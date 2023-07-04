/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-call-notification"

#include "phosh-config.h"

#include "call-notification.h"
#include "calls-manager.h"
#include "util.h"

#include "cui-call.h"
#include "handy.h"
#include <glib/gi18n.h>

/**
 * PhoshCallNotification:
 *
 * The notifictaion shown when a call is ongoing.  The call is set at
 * construction time and can't be changed.
 */

enum {
  PROP_0,
  PROP_CALL,
  PROP_ACTIVE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshCallNotification {
  GtkListBoxRow parent;

  HdyAvatar    *avatar;
  GtkLabel     *call_duration;
  GtkLabel     *call_state;
  GtkLabel     *caller;
  GtkLabel     *caller_detail;

  PhoshCall    *call;
  gboolean      active;
};
G_DEFINE_TYPE (PhoshCallNotification, phosh_call_notification, GTK_TYPE_LIST_BOX_ROW)


static gboolean
transform_display_name (GBinding     *binding,
                        const GValue *from_value,
                        GValue       *to_value,
                        gpointer      user_data)
{
  const char *display_name = g_value_get_string (from_value);

  if (STR_IS_NULL_OR_EMPTY (display_name))
    g_value_set_static_string (to_value, _("Unknown caller"));
  else
    g_value_set_string (to_value, display_name);

  return TRUE;
}


static gboolean
transform_display_name_detail (GBinding     *binding,
                               const GValue *from_value,
                               GValue       *to_value,
                               gpointer      user_data)
{
  const char *display_name = g_value_get_string (from_value);
  gboolean visible = TRUE;

  /* Hide details for unknown callers so the display name is centered */
  if (STR_IS_NULL_OR_EMPTY (display_name))
    visible = FALSE;

  g_value_set_boolean (to_value, visible);
  return TRUE;
}


static gboolean
transform_display_name_initals (GBinding     *binding,
                                const GValue *from_value,
                                GValue       *to_value,
                                gpointer      user_data)
{
  const char *display_name = g_value_get_string (from_value);
  gboolean show_initials = TRUE;

  /* Don't show initials for unknown callers as it would always be 'UC' */
  if (STR_IS_NULL_OR_EMPTY (display_name))
    show_initials = FALSE;

  g_value_set_boolean (to_value, show_initials);
  return TRUE;
}


static gboolean
transform_active_time (GBinding     *binding,
                       const GValue *from_value,
                       GValue       *to_value,
                       gpointer      user_data)
{
#define MINUTE 60
#define HOUR   (60 * MINUTE)

  /* TODO: use https://gitlab.gnome.org/World/Phosh/libcall-ui/-/merge_requests/73 */
  double elapsed = g_value_get_double (from_value);
  guint seconds, minutes;
  GString *str = g_string_new ("");

  if (elapsed > HOUR) {
    int hours = (int) (elapsed / HOUR);
    g_string_append_printf (str, "%u:", hours);
    elapsed -= (hours * HOUR);
  }

  minutes = (int) (elapsed / MINUTE);
  seconds = elapsed - (minutes * MINUTE);
  g_string_append_printf (str, "%02u:%02u", minutes, seconds);

  g_value_take_string (to_value, g_string_free (str, FALSE));
  return TRUE;

#undef HOUR
#undef MINUTE
}


static gboolean
transform_call_state (GBinding     *binding,
                      const GValue *from_value,
                      GValue       *to_value,
                      gpointer      user_data)
{
  PhoshCallState state = g_value_get_enum (from_value);

  g_value_set_string (to_value, cui_call_state_to_string (state));
  return TRUE;
}


static void
phosh_call_notification_set_call (PhoshCallNotification *self, PhoshCall *call)
{
  g_return_if_fail (PHOSH_IS_CALL_NOTIFICATION (self));
  g_return_if_fail (PHOSH_IS_CALL (call));
  g_return_if_fail (self->call == NULL);

  g_set_object (&self->call, call);
  g_object_bind_property_full (call, "display-name",
                               self->caller, "label",
                               G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE,
                               transform_display_name,
                               NULL,
                               NULL,
                               NULL);
  g_object_bind_property_full (call, "display-name",
                               self->caller_detail, "visible",
                               G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE,
                               transform_display_name_detail,
                               NULL,
                               NULL,
                               NULL);
  g_object_bind_property (call, "display-name",
                          self->avatar, "text",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);
  g_object_bind_property_full (call, "display-name",
                               self->avatar, "show-initials",
                               G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE,
                               transform_display_name_initals,
                               NULL,
                               NULL,
                               NULL);
  g_object_bind_property (call, "id",
                          self->caller_detail, "label",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);
  g_object_bind_property (call, "avatar-icon",
                          self->avatar, "loadable-icon",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);
  g_object_bind_property_full (call, "active-time",
                               self->call_duration, "label",
                               G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE,
                               transform_active_time,
                               NULL,
                               NULL,
                               NULL);
  g_object_bind_property_full (call, "state",
                               self->call_state, "label",
                               G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE,
                               transform_call_state,
                               NULL,
                               NULL,
                               NULL);
}


static void
phosh_call_notification_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  PhoshCallNotification *self = PHOSH_CALL_NOTIFICATION (object);

  switch (property_id) {
  case PROP_CALL:
    phosh_call_notification_set_call (self, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_call_notification_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  PhoshCallNotification *self = PHOSH_CALL_NOTIFICATION (object);

  switch (property_id) {
  case PROP_ACTIVE:
    g_value_set_boolean (value, self->active);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_call_notification_dispose (GObject *object)
{
  PhoshCallNotification *self = PHOSH_CALL_NOTIFICATION (object);

  g_clear_object (&self->call);

  G_OBJECT_CLASS (phosh_call_notification_parent_class)->dispose (object);
}


static void
phosh_call_notification_class_init (PhoshCallNotificationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_call_notification_get_property;
  object_class->set_property = phosh_call_notification_set_property;
  object_class->dispose = phosh_call_notification_dispose;

  /**
   * PhoshCallNotification:call:
   *
   * The call tracked by this notifiation
   */
  props[PROP_CALL] =
    g_param_spec_object ("call", "", "",
                         PHOSH_TYPE_CALL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * PhoshCallNotification:active:
   *
   * %TRUE when the notification has an associated call and it is active.
   */
  props[PROP_ACTIVE] =
    g_param_spec_boolean ("active", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/call-notification.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshCallNotification, avatar);
  gtk_widget_class_bind_template_child (widget_class, PhoshCallNotification, call_duration);
  gtk_widget_class_bind_template_child (widget_class, PhoshCallNotification, call_state);
  gtk_widget_class_bind_template_child (widget_class, PhoshCallNotification, caller);
  gtk_widget_class_bind_template_child (widget_class, PhoshCallNotification, caller_detail);

  gtk_widget_class_set_css_name (widget_class, "phosh-call-notification");
}


static void
phosh_call_notification_init (PhoshCallNotification *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


PhoshCallNotification *
phosh_call_notification_new (PhoshCall *call)
{
  return g_object_new (PHOSH_TYPE_CALL_NOTIFICATION,
                       "call", call,
                       NULL);
}
