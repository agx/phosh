/*
 * Copyright (C) 2021-2023 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-notify-feedback"

#include "phosh-config.h"

#include "shell.h"
#include "notify-feedback.h"
#include "notification-source.h"
#include "util.h"

#include <libfeedback.h>

#include <gmobile.h>

/**
 * PhoshNotifyFeedback:
 *
 * Provide feedback on notifications
 *
 * #PhoshNotifyFeedback is the manager object responsible to provide
 * proper feedback on new notifications or when notifications are
 * being closed.
 */

enum {
  PROP_0,
  PROP_NOTIFICATION_LIST,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshNotifyFeedback {
  GObject                      parent;

  LfbEvent                     *event;
  PhoshNotificationList        *list;

  GSettings                    *settings;
  PhoshNotifyScreenWakeupFlags  wakeup_flags;
  GStrv                         wakeup_categories;
  PhoshNotificationUrgency      wakeup_min_urgency;
};
G_DEFINE_TYPE (PhoshNotifyFeedback, phosh_notify_feedback, G_TYPE_OBJECT)

static void
end_notify_feedback (PhoshNotifyFeedback *self)
{
  g_return_if_fail (lfb_is_initted ());

  if (self->event == NULL)
    return;

  if (lfb_event_get_state (self->event) == LFB_EVENT_STATE_RUNNING)
    lfb_event_end_feedback_async (self->event, NULL, NULL, NULL);
}


static const char *
find_event (const char *category)
{
  PhoshShell *shell = phosh_shell_get_default ();
  gboolean inactive = phosh_shell_get_blanked (shell) || phosh_shell_get_locked (shell);
  const char *ret = NULL;

  if (inactive) {
    if (g_strcmp0 (category, "email.arrived") == 0)
      ret = "message-missed-email";
    else if (g_strcmp0 (category, "im.received") == 0)
      ret = "message-missed-instant";
    else
      ret = "message-missed-notification";
  } else {
    if (g_strcmp0 (category, "email.arrived") == 0)
      ret = "message-new-email";
    else if (g_strcmp0 (category, "im.received") == 0)
      ret = "message-new-instant";
    /* no feedback when not locked as to not distract the user */
  }

  if (g_strcmp0 (category, "x-gnome.call.unanswered") == 0)
    ret = "phone-missed-call";
  else if (g_strcmp0 (category, "call.ended") == 0)
    ret = "phone-hangup";
  else if (g_strcmp0 (category, "call.incoming") == 0)
    ret = "phone-incoming-call";
  else if (g_strcmp0 (category, "call.unanswered") == 0)
    ret = "phone-missed-call";

  return ret;
}


static void
maybe_wakeup_screen (PhoshNotifyFeedback *self, PhoshNotificationSource *source, guint position, guint num)
{
  g_return_if_fail (num > 0);

  if (self->wakeup_flags & PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_ANY) {
    phosh_shell_activate_action (phosh_shell_get_default (), "screensaver.wakeup-screen", NULL);
    return;
  }

  for (int i = 0; i < num; i++) {
    g_autoptr (PhoshNotification) noti = g_list_model_get_item (G_LIST_MODEL (source), position + i);
    gboolean wakeup;

    g_return_if_fail (PHOSH_IS_NOTIFICATION (noti));

    wakeup = phosh_notify_feedback_check_screen_wakeup (self, noti);
    if (wakeup) {
      phosh_shell_activate_action (phosh_shell_get_default (), "screensaver.wakeup-screen", NULL);
      break;
    }
  }
}


static gboolean
maybe_trigger_feedback (PhoshNotifyFeedback *self, PhoshNotificationSource *source, guint position, guint num)
{
  for (int i = 0; i < num; i++) {
    g_autoptr (PhoshNotification) noti = g_list_model_get_item (G_LIST_MODEL (source), position + i);
    g_autoptr (LfbEvent) event = NULL;
    const char *category, *event_name, *profile;
    g_autofree char *app_id = NULL;
    GAppInfo *info = NULL;

    g_return_val_if_fail (PHOSH_IS_NOTIFICATION (noti), FALSE);
    g_return_val_if_fail (lfb_is_initted (), FALSE);

    category = phosh_notification_get_category (noti);
    event_name = find_event (category);
    if (event_name == NULL)
      continue;

    info = phosh_notification_get_app_info (noti);
    if (info)
      app_id = phosh_strip_suffix_from_app_id (g_app_info_get_id (info));

    profile = phosh_notification_get_profile (noti);
    if (g_strcmp0 (profile, "none") == 0)
      continue;

    g_debug ("Emitting event %s for %s, profile: %s",
             event_name, app_id ?: "unknown", profile);
    event = lfb_event_new (event_name);
    lfb_event_set_feedback_profile (event, profile);
    g_set_object (&self->event, event);

    if (app_id)
      lfb_event_set_app_id (event, app_id);

    lfb_event_trigger_feedback_async (self->event, NULL, NULL, NULL);
    return TRUE;
  }

  return FALSE;
}


static void
on_notification_source_items_changed (PhoshNotifyFeedback *self,
                                      guint                position,
                                      guint                removed,
                                      guint                added,
                                      GListModel          *list)
{
  if (!added)
    return;

  maybe_wakeup_screen (self, PHOSH_NOTIFICATION_SOURCE (list), position, added);

  /* TODO: add pending events to queue instead of just skipping them. */
  if (self->event && lfb_event_get_state (self->event) == LFB_EVENT_STATE_RUNNING)
    return;

  maybe_trigger_feedback (self, PHOSH_NOTIFICATION_SOURCE (list), position, added);
}


static void
on_settings_changed (PhoshNotifyFeedback *self, char *key, GSettings *settings)
{
  self->wakeup_flags = g_settings_get_flags (settings, "wakeup-screen-triggers");
  self->wakeup_min_urgency = g_settings_get_enum (settings, "wakeup-screen-urgency");
  g_strfreev (self->wakeup_categories);
  self->wakeup_categories = g_settings_get_strv (settings, "wakeup-screen-categories");
}


static void
on_notification_list_items_changed (PhoshNotifyFeedback *self,
                                    guint                position,
                                    guint                removed,
                                    guint                added,
                                    GListModel          *list)
{
  g_autoptr (PhoshNotificationSource) first = g_list_model_get_item (list, 0);

  if (!first) {
    g_debug ("Notification list empty, ending feedback");
    end_notify_feedback (self);
  }

  /* No need to worry about removed sources, signals get detached due
   * to g_signal_connect_object () */
  for (int i = position; i < position + added; i++) {
    g_autoptr (PhoshNotificationSource) source = g_list_model_get_item (list, position);
    /* Listen to new notification on the store for feedback triggering */
    g_signal_connect_object (source,
                             "items-changed",
                             G_CALLBACK (on_notification_source_items_changed),
                             self,
                             G_CONNECT_SWAPPED);
    on_notification_source_items_changed (self, 0, 0,
                                         g_list_model_get_n_items (G_LIST_MODEL (source)),
                                         G_LIST_MODEL (source));
  }
}


static void
on_shell_state_changed (PhoshNotifyFeedback *self, GParamSpec *pspec, PhoshShell *shell)
{
  g_return_if_fail (PHOSH_IS_NOTIFY_FEEDBACK (self));

  /* Feedback ongoing, nothing to do */
  if (self->event && lfb_event_get_state (self->event) == LFB_EVENT_STATE_RUNNING)
    return;

  if (!phosh_shell_get_blanked (shell))
    return;

  for (guint i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (self->list)); i++) {
    g_autoptr (PhoshNotificationSource) source = g_list_model_get_item (G_LIST_MODEL (self->list), i);

    if (maybe_trigger_feedback (self, source, 0, g_list_model_get_n_items (G_LIST_MODEL (source))))
      break;
  }
}


static void
phosh_notify_feedback_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PhoshNotifyFeedback *self = PHOSH_NOTIFY_FEEDBACK (object);

  switch (property_id) {
  case PROP_NOTIFICATION_LIST:
    self->list = g_value_dup_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_notify_feedback_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  PhoshNotifyFeedback *self = PHOSH_NOTIFY_FEEDBACK (object);

  switch (property_id) {
  case PROP_NOTIFICATION_LIST:
    g_value_set_object (value, self->list);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_notify_feedback_constructed (GObject *object)
{
  PhoshNotifyFeedback *self = PHOSH_NOTIFY_FEEDBACK (object);

  G_OBJECT_CLASS (phosh_notify_feedback_parent_class)->constructed (object);

  g_signal_connect_swapped (self->list,
                            "items-changed",
                            G_CALLBACK (on_notification_list_items_changed),
                            self);

  g_signal_connect_object (phosh_shell_get_default (),
                           "notify::shell-state",
                           G_CALLBACK (on_shell_state_changed),
                           self,
                           G_CONNECT_SWAPPED);
}


static void
phosh_notify_feedback_dispose (GObject *object)
{
  PhoshNotifyFeedback *self = PHOSH_NOTIFY_FEEDBACK (object);

  g_clear_object (&self->settings);

  if (self->event) {
    end_notify_feedback (self);
    g_clear_object (&self->event);
  }

  g_signal_handlers_disconnect_by_data (self->list, self);
  g_clear_object (&self->list);

  g_clear_pointer (&self->wakeup_categories, g_strfreev);

  G_OBJECT_CLASS (phosh_notify_feedback_parent_class)->dispose (object);
}


static void
phosh_notify_feedback_class_init (PhoshNotifyFeedbackClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phosh_notify_feedback_get_property;
  object_class->set_property = phosh_notify_feedback_set_property;
  object_class->constructed = phosh_notify_feedback_constructed;
  object_class->dispose = phosh_notify_feedback_dispose;

  /**
   * PhoshNotifyFeedback:notification-list
   *
   * The list of notifications that drives the feedback emission
   */
  props[PROP_NOTIFICATION_LIST] =
    g_param_spec_object ("notification-list",
                         "",
                         "",
                         PHOSH_TYPE_NOTIFICATION_LIST,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_notify_feedback_init (PhoshNotifyFeedback *self)
{
  self->settings = g_settings_new ("sm.puri.phosh.notifications");

  g_signal_connect_swapped (self->settings, "changed", G_CALLBACK (on_settings_changed), self);
  on_settings_changed (self, NULL, self->settings);
}


PhoshNotifyFeedback *
phosh_notify_feedback_new (PhoshNotificationList *list)
{
  return PHOSH_NOTIFY_FEEDBACK (g_object_new (PHOSH_TYPE_NOTIFY_FEEDBACK,
                                              "notification-list", list,
                                              NULL));
}


/**
 * phosh_notify_feedback_check_screen_wakeup:
 * @self: The notification feedback manager
 * @notification: The notification to check
 *
 * Checks if the given notification should trigger screeen wakeup
 *
 * Returns: %TRUE if the notification should trigger the screen wakeup.
 */
gboolean
phosh_notify_feedback_check_screen_wakeup (PhoshNotifyFeedback *self,
                                           PhoshNotification   *notification)
{
  const char *category;
  PhoshNotificationUrgency urgency;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (notification), FALSE);

  urgency = phosh_notification_get_urgency (notification);
  if (self->wakeup_flags & PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_URGENCY &&
        urgency >= self->wakeup_min_urgency) {
    return TRUE;
  }

  category = phosh_notification_get_category (notification);
  if (!gm_str_is_null_or_empty (category) &&
      self->wakeup_flags & PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_CATEGORY) {
    /* exact match of setting to notification's category (e.g. `im` == `im` or `im.foo == `im.foo` */
    if (g_strv_contains ((const char * const *)self->wakeup_categories, category))
      return TRUE;

    /* setting (`im`) matches the class of the notification (`im.received`) */
    for (int j = 0; j < g_strv_length (self->wakeup_categories); j++) {
      if (strchr (self->wakeup_categories[j], '.'))
        continue;

      if (g_str_has_prefix (category, self->wakeup_categories[j]) &&
          category [strlen (self->wakeup_categories[j])] == '.') {
        return TRUE;
      }
    }
  }
  return FALSE;
}
