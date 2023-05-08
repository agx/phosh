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

#define LIBFEEDBACK_USE_UNSTABLE_API
#include <libfeedback.h>

/**
 * PhoshNotifyFeedback:
 *
 * Provider feedback on notifications
 *
 * #PhoshNotifyFeedback is responsible to provider proper feedback
 * on new notifications or when notifications are being closed.
 */

enum {
  PROP_0,
  PROP_NOTIFICATION_LIST,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshNotifyFeedback {
  GObject                parent;

  LfbEvent              *event;
  PhoshNotificationList *list;
};
G_DEFINE_TYPE (PhoshNotifyFeedback, phosh_notify_feedback, G_TYPE_OBJECT)

static void
end_notify_feedback (PhoshNotifyFeedback *self)
{
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
  /* If shell is unlocked and we don't have a specific category don't
     trigger any event to not distract the user*/
  const char *ret = NULL;

  if (inactive) {
    if (g_strcmp0 (category, "email.arrived") == 0)
      ret = "message-missed-email";
    else if (g_strcmp0 (category, "im.received") == 0)
      ret = "message-missed-instant";
    else if (g_strcmp0 (category, "x-gnome.call.unanswered") == 0)
      ret = "phone-missed-call";
    else
      ret = "message-missed-notification";
  } else {
    if (g_strcmp0 (category, "email.arrived") == 0)
      ret = "message-new-email";
    else if (g_strcmp0 (category, "im.received") == 0)
      ret = "message-new-instant";
    else if (g_strcmp0 (category, "x-gnome.call.unanswered") == 0)
      ret = "phone-missed-call";
    /* no additional feedback when not locked */
  }

  return ret;
}


static void
on_notifcation_source_items_changed (PhoshNotifyFeedback *self,
                                     guint                position,
                                     guint                removed,
                                     guint                added,
                                     GListModel          *list)
{
  if (!added)
    return;

  /* TODO: add pending events to queue instead of just skipping them. */
  if (self->event && lfb_event_get_state (self->event) == LFB_EVENT_STATE_RUNNING)
    return;

  for (int i = 0; i < added; i++) {
    g_autoptr (PhoshNotification) new = g_list_model_get_item (list, position + i);
    g_autoptr (LfbEvent) event = NULL;
    const char *category, *event_name;
    g_autofree char *app_id = NULL;
    GAppInfo *info = NULL;

    g_return_if_fail (PHOSH_IS_NOTIFICATION (new));

    category = phosh_notification_get_category (new);
    event_name = find_event (category);
    if (event_name == NULL)
      continue;

    info = phosh_notification_get_app_info (new);
    if (info == NULL)
        continue;
    app_id = phosh_strip_suffix_from_app_id (g_app_info_get_id (info));

    g_debug ("Emitting event %s for %s", event_name, app_id ?: "unknown");
    event = lfb_event_new (event_name);
    g_set_object (&self->event, event);

    if (app_id)
      lfb_event_set_app_id (event, app_id);

    lfb_event_trigger_feedback_async (self->event, NULL, NULL, NULL);
    /* TODO: add additional events to queue instead of just skipping them */
    break;
  }
}


static void
on_notifcation_list_items_changed (PhoshNotifyFeedback *self,
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
                             G_CALLBACK (on_notifcation_source_items_changed),
                             self,
                             G_CONNECT_SWAPPED);
    on_notifcation_source_items_changed (self, 0, 0,
                                         g_list_model_get_n_items (G_LIST_MODEL (source)),
                                         G_LIST_MODEL (source));
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
                            G_CALLBACK (on_notifcation_list_items_changed),
                            self);
}


static void
phosh_notify_feedback_dispose (GObject *object)
{
  PhoshNotifyFeedback *self = PHOSH_NOTIFY_FEEDBACK (object);

  if (self->event) {
    end_notify_feedback (self);
    g_clear_object (&self->event);
  }

  g_signal_handlers_disconnect_by_data (self->list, self);
  g_clear_object (&self->list);

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
}


PhoshNotifyFeedback *
phosh_notify_feedback_new (PhoshNotificationList *list)
{
  return PHOSH_NOTIFY_FEEDBACK (g_object_new (PHOSH_TYPE_NOTIFY_FEEDBACK,
                                              "notification-list", list,
                                              NULL));
}
