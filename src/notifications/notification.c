/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-notification"

#include "phosh-config.h"
#include "notification.h"
#include "phosh-enums.h"
#include "app-grid-button.h"

#include <glib/gi18n-lib.h>

/**
 * PhoshNotification:
 *
 * A notification displayed to the user
 *
 * A notification usually consists of a summary, a body (the content)
 * and an icon.
 */

enum {
  PROP_0,
  PROP_ID,
  PROP_APP_NAME,
  PROP_SUMMARY,
  PROP_BODY,
  PROP_APP_ICON,
  PROP_APP_INFO,
  PROP_IMAGE,
  PROP_URGENCY,
  PROP_ACTIONS,
  PROP_TRANSIENT,
  PROP_RESIDENT,
  PROP_CATEGORY,
  PROP_PROFILE,
  PROP_TIMESTAMP,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];


enum {
  SIGNAL_ACTIONED,
  SIGNAL_EXPIRED,
  SIGNAL_CLOSED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


typedef struct _PhoshNotificationPrivate {
  guint                     id;
  char                     *app_name;
  GDateTime                *updated;
  char                     *summary;
  char                     *body;
  GIcon                    *icon;
  GIcon                    *image;
  GAppInfo                 *info;
  PhoshNotificationUrgency  urgency;
  GStrv                     actions;
  gboolean                  transient;
  gboolean                  resident;
  char                     *category;
  char                     *profile;

  gulong                    timeout;
} PhoshNotificationPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshNotification, phosh_notification, G_TYPE_OBJECT)


static void
phosh_notification_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  PhoshNotification *self = PHOSH_NOTIFICATION (object);

  switch (property_id) {
    case PROP_ID:
      phosh_notification_set_id (self, g_value_get_uint (value));
      break;
    case PROP_APP_NAME:
      phosh_notification_set_app_name (self, g_value_get_string (value));
      break;
    case PROP_TIMESTAMP:
      phosh_notification_set_timestamp (self, g_value_get_boxed (value));
      break;
    case PROP_SUMMARY:
      phosh_notification_set_summary (self, g_value_get_string (value));
      break;
    case PROP_BODY:
      phosh_notification_set_body (self, g_value_get_string (value));
      break;
    case PROP_APP_ICON:
      phosh_notification_set_app_icon (self, g_value_get_object (value));
      break;
    case PROP_APP_INFO:
      phosh_notification_set_app_info (self, g_value_get_object (value));
      break;
    case PROP_IMAGE:
      phosh_notification_set_image (self, g_value_get_object (value));
      break;
    case PROP_URGENCY:
      phosh_notification_set_urgency (self, g_value_get_enum (value));
      break;
    case PROP_ACTIONS:
      phosh_notification_set_actions (self, g_value_get_boxed (value));
      break;
    case PROP_TRANSIENT:
      phosh_notification_set_transient (self, g_value_get_boolean (value));
      break;
    case PROP_RESIDENT:
      phosh_notification_set_resident (self, g_value_get_boolean (value));
      break;
    case PROP_CATEGORY:
      phosh_notification_set_category (self, g_value_get_string (value));
      break;
    case PROP_PROFILE:
      phosh_notification_set_profile (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_notification_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  PhoshNotification *self = PHOSH_NOTIFICATION (object);

  switch (property_id) {
    case PROP_ID:
      g_value_set_uint (value, phosh_notification_get_id (self));
      break;
    case PROP_APP_NAME:
      g_value_set_string (value, phosh_notification_get_app_name (self));
      break;
    case PROP_TIMESTAMP:
      g_value_set_boxed (value, phosh_notification_get_timestamp (self));
      break;
    case PROP_SUMMARY:
      g_value_set_string (value, phosh_notification_get_summary (self));
      break;
    case PROP_BODY:
      g_value_set_string (value, phosh_notification_get_body (self));
      break;
    case PROP_APP_ICON:
      g_value_set_object (value, phosh_notification_get_app_icon (self));
      break;
    case PROP_APP_INFO:
      g_value_set_object (value, phosh_notification_get_app_info (self));
      break;
    case PROP_IMAGE:
      g_value_set_object (value, phosh_notification_get_image (self));
      break;
    case PROP_URGENCY:
      g_value_set_enum (value, phosh_notification_get_urgency (self));
      break;
    case PROP_ACTIONS:
      g_value_set_boxed (value, phosh_notification_get_actions (self));
      break;
    case PROP_TRANSIENT:
      g_value_set_boolean (value, phosh_notification_get_transient (self));
      break;
    case PROP_RESIDENT:
      g_value_set_boolean (value, phosh_notification_get_resident (self));
      break;
    case PROP_CATEGORY:
      g_value_set_string (value, phosh_notification_get_category (self));
      break;
    case PROP_PROFILE:
      g_value_set_string (value, phosh_notification_get_profile (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_notification_finalize (GObject *object)
{
  PhoshNotification *self = PHOSH_NOTIFICATION (object);
  PhoshNotificationPrivate *priv = phosh_notification_get_instance_private (self);

  /* If we've been dismissed cancel the auto timeout */
  if (priv->timeout != 0) {
    g_source_remove (priv->timeout);
  }

  g_clear_pointer (&priv->app_name, g_free);
  g_clear_pointer (&priv->updated, g_date_time_unref);
  g_clear_pointer (&priv->summary, g_free);
  g_clear_pointer (&priv->body, g_free);
  g_clear_object (&priv->icon);
  g_clear_object (&priv->image);
  g_clear_object (&priv->info);
  g_clear_pointer (&priv->actions, g_strfreev);
  g_clear_pointer (&priv->category, g_free);
  g_clear_pointer (&priv->profile, g_free);

  G_OBJECT_CLASS (phosh_notification_parent_class)->finalize (object);
}


static void
phosh_notification_class_init (PhoshNotificationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phosh_notification_finalize;
  object_class->set_property = phosh_notification_set_property;
  object_class->get_property = phosh_notification_get_property;

  props[PROP_ID] =
    g_param_spec_uint (
      "id",
      "ID",
      "Notification ID",
      0,
      G_MAXUINT,
      0,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_APP_NAME] =
    g_param_spec_string (
      "app-name",
      "App Name",
      "The applications's name",
      "",
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_TIMESTAMP] =
    g_param_spec_boxed (
      "timestamp",
      "Timestamp",
      "The time that notification came in.",
      G_TYPE_DATE_TIME,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_SUMMARY] =
    g_param_spec_string (
      "summary",
      "Summary",
      "The notification's summary",
      "",
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_BODY] =
    g_param_spec_string (
      "body",
      "Body",
      "The notification's body",
      "",
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_APP_ICON] =
    g_param_spec_object (
      "app-icon",
      "App Icon",
      "Application icon",
      G_TYPE_ICON,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * PhoshNotification:app-info:
   *
   * When non-%NULL this overrides the values of [property@Notification:app-name]
   * and [property@Notification:app-icon] with those from the `GAppInfo`.
   */
  props[PROP_APP_INFO] =
    g_param_spec_object ("app-info", "", "",
      G_TYPE_APP_INFO,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_IMAGE] =
    g_param_spec_object (
      "image",
      "Image",
      "Notification image",
      G_TYPE_ICON,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_URGENCY] =
    g_param_spec_enum (
      "urgency",
      "Urgency",
      "Notification urgency",
      PHOSH_TYPE_NOTIFICATION_URGENCY,
      PHOSH_NOTIFICATION_URGENCY_NORMAL,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_ACTIONS] =
    g_param_spec_boxed (
      "actions",
      "Actions",
      "Notification actions",
      G_TYPE_STRV,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_TRANSIENT] =
    g_param_spec_boolean (
      "transient",
      "Transient",
      "The notification is transient",
      FALSE,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_RESIDENT] =
    g_param_spec_boolean (
      "resident",
      "Resident",
      "The notification is resident",
      FALSE,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_CATEGORY] =
    g_param_spec_string (
      "category",
      "Category",
      "The notification's category",
      "",
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * PhoshNotification:profile:
   *
   * The feedback profile to use for the event triggered by this
   * notification. Valid values from Feedbackd's Feedback Theme
   * Specification as well as the value `none` meaning: don't trigger
   * any feedback for this event. If `NULL` (the default) the decision
   * is left to feedbackd.
   */
  props[PROP_PROFILE] =
    g_param_spec_string ("profile", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  /**
   * PhoshNotification::actioned:
   * @self: The notification
   * @action: The name of the activated action
   *
   * Emitted when the user activates one of the provided actions
   *
   * This includes the default action.
   */
  signals[SIGNAL_ACTIONED] = g_signal_new ("actioned",
                                           G_TYPE_FROM_CLASS (klass),
                                           G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                           NULL,
                                           G_TYPE_NONE,
                                           1,
                                           G_TYPE_STRING);
  /**
   * PhoshNotification::expired:
   * @self: The notification
   *
   * Emitted when the timeout set by [method@Notification.expires] has expired
   */
  signals[SIGNAL_EXPIRED] = g_signal_new ("expired",
                                          G_TYPE_FROM_CLASS (klass),
                                          G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                          NULL,
                                          G_TYPE_NONE,
                                          0);
  /**
   * PhoshNotification::closed:
   * @self: The #PhoshNotifiation
   * @reason: Why @self was closed
   *
   * Emitted when the notification has been closed
   */
  signals[SIGNAL_CLOSED] = g_signal_new ("closed",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                         NULL,
                                         G_TYPE_NONE,
                                         1,
                                         PHOSH_TYPE_NOTIFICATION_REASON);
}


static void
phosh_notification_init (PhoshNotification *self)
{
  PhoshNotificationPrivate *priv = phosh_notification_get_instance_private (self);

  priv->app_name = g_strdup (_("Notification"));
}


PhoshNotification *
phosh_notification_new (guint                     id,
                        const char               *app_name,
                        GAppInfo                 *info,
                        const char               *summary,
                        const char               *body,
                        GIcon                    *icon,
                        GIcon                    *image,
                        PhoshNotificationUrgency  urgency,
                        GStrv                     actions,
                        gboolean                  transient,
                        gboolean                  resident,
                        const char               *category,
                        const char               *profile,
                        GDateTime                *timestamp)
{

  return g_object_new (PHOSH_TYPE_NOTIFICATION,
                       "id", id,
                       "summary", summary,
                       "body", body,
                       "app-name", app_name,
                       "app-icon", icon,
                       /* Set info after fallback name and icon */
                       "app-info", info,
                       "image", image,
                       "urgency", urgency,
                       "actions", actions,
                       "transient", transient,
                       "resident", resident,
                       "category", category,
                       "profile", profile,
                       "timestamp", timestamp,
                       NULL);
}


void
phosh_notification_set_id (PhoshNotification *self,
                           guint              id)
{
  PhoshNotificationPrivate *priv;
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  priv = phosh_notification_get_instance_private (self);

  if (priv->id == id) {
    return;
  }

  priv->id = id;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ID]);
}


guint
phosh_notification_get_id (PhoshNotification *self)
{
  PhoshNotificationPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), 0);
  priv = phosh_notification_get_instance_private (self);

  return priv->id;
}


void
phosh_notification_set_app_icon (PhoshNotification *self,
                                 GIcon             *icon)
{
  PhoshNotificationPrivate *priv;

  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  priv = phosh_notification_get_instance_private (self);

  g_clear_object (&priv->icon);
  if (icon != NULL) {
    priv->icon = g_object_ref (icon);
  } else {
    priv->icon = g_themed_icon_new (PHOSH_APP_UNKNOWN_ICON);
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_APP_ICON]);
}

/**
 * phosh_notification_get_app_icon:
 * @self: The notification:
 *
 * Returns:(transfer none): The notification's icon
 */
GIcon *
phosh_notification_get_app_icon (PhoshNotification *self)
{
  PhoshNotificationPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), 0);
  priv = phosh_notification_get_instance_private (self);

  if (priv->info && g_app_info_get_icon (priv->info)) {
    return g_app_info_get_icon (priv->info);
  }

  return priv->icon;
}


void
phosh_notification_set_app_info (PhoshNotification *self,
                                 GAppInfo          *info)
{
  PhoshNotificationPrivate *priv;

  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  priv = phosh_notification_get_instance_private (self);

  g_clear_object (&priv->info);

  if (info != NULL) {
    GIcon *icon;
    const char *name;

    priv->info = g_object_ref (info);

    icon = g_app_info_get_icon (info);
    name = g_app_info_get_name (info);

    phosh_notification_set_app_icon (self, icon);
    phosh_notification_set_app_name (self, name);
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_APP_INFO]);
}

/**
 * phosh_notification_get_app_info:
 * @self: The notification:
 *
 * Returns:(transfer none): The notification's app info
 */
GAppInfo *
phosh_notification_get_app_info (PhoshNotification *self)
{
  PhoshNotificationPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), 0);
  priv = phosh_notification_get_instance_private (self);

  return priv->info;
}


void
phosh_notification_set_image (PhoshNotification *self,
                              GIcon             *image)
{
  PhoshNotificationPrivate *priv;

  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  priv = phosh_notification_get_instance_private (self);

  if (g_set_object (&priv->image, image)) {
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_IMAGE]);
  }
}

/**
 * phosh_notification_get_image:
 * @self: The notification:
 *
 * Returns:(transfer none): The notification's image
 */
GIcon *
phosh_notification_get_image (PhoshNotification *self)
{
  PhoshNotificationPrivate *priv;
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), 0);
  priv = phosh_notification_get_instance_private (self);

  return priv->image;
}


void
phosh_notification_set_summary (PhoshNotification *self,
                                const char        *summary)
{
  PhoshNotificationPrivate *priv;

  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  priv = phosh_notification_get_instance_private (self);

  if (g_strcmp0 (priv->summary, summary) == 0) {
    return;
  }

  g_clear_pointer (&priv->summary, g_free);
  priv->summary = g_strdup (summary);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SUMMARY]);
}


const char *
phosh_notification_get_summary (PhoshNotification *self)
{
  PhoshNotificationPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), 0);
  priv = phosh_notification_get_instance_private (self);

  return priv->summary;
}


void
phosh_notification_set_body (PhoshNotification *self,
                             const char        *body)
{
  PhoshNotificationPrivate *priv;

  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  priv = phosh_notification_get_instance_private (self);

  if (g_strcmp0 (priv->body, body) == 0) {
    return;
  }

  g_clear_pointer (&priv->body, g_free);
  priv->body = g_strdup (body);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_BODY]);
}


const char *
phosh_notification_get_body (PhoshNotification *self)
{
  PhoshNotificationPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), 0);
  priv = phosh_notification_get_instance_private (self);

  return priv->body;
}



void
phosh_notification_set_app_name (PhoshNotification *self,
                                 const char        *app_name)
{
  PhoshNotificationPrivate *priv;

  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  priv = phosh_notification_get_instance_private (self);

  if (g_strcmp0 (priv->app_name, app_name) == 0) {
    return;
  }

  g_clear_pointer (&priv->app_name, g_free);

  if (app_name &&
      strlen (app_name) > 0 &&
      g_strcmp0 (app_name, "notify-send") != 0) {
    priv->app_name = g_strdup (app_name);
  } else {
    priv->app_name = g_strdup (_("Notification"));
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_APP_NAME]);
}


const char *
phosh_notification_get_app_name (PhoshNotification *self)
{
  PhoshNotificationPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), 0);
  priv = phosh_notification_get_instance_private (self);

  if (priv->info && g_app_info_get_name (priv->info)) {
    return g_app_info_get_name (priv->info);
  }

  return priv->app_name;
}

/**
 * phosh_notification_set_timestamp:
 * @self: A #PhoshNotification
 * @timestamp: (nullable): A timestamp or %NULL
 *
 * Sets the timestamp of a notification. If %NULL is passed it's set
 * to the current date and time.
 */
void
phosh_notification_set_timestamp (PhoshNotification *self,
                                  GDateTime         *timestamp)
{
  PhoshNotificationPrivate *priv;
  g_autoptr (GDateTime) now = NULL;

  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  priv = phosh_notification_get_instance_private (self);

  if (timestamp == NULL) {
    now = g_date_time_new_now_local ();
    timestamp = now;
  }

  if (priv->updated != NULL && g_date_time_compare (priv->updated, timestamp) == 0)
     return;

  g_clear_pointer (&priv->updated, g_date_time_unref);
  priv->updated = g_date_time_ref (timestamp);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TIMESTAMP]);
}


GDateTime *
phosh_notification_get_timestamp (PhoshNotification *self)
{
  PhoshNotificationPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), NULL);
  priv = phosh_notification_get_instance_private (self);

  return priv->updated;
}


void
phosh_notification_set_actions (PhoshNotification *self,
                                GStrv              actions)
{
  PhoshNotificationPrivate *priv;

  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  priv = phosh_notification_get_instance_private (self);

  g_clear_pointer (&priv->actions, g_strfreev);
  priv->actions = g_strdupv (actions);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ACTIONS]);
}

/**
 * phosh_notification_get_actions:
 * @self: The #PhoshNotification
 *
 * Returns: (transfer none): The notification's actions
 */
GStrv
phosh_notification_get_actions (PhoshNotification *self)
{
  PhoshNotificationPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), 0);
  priv = phosh_notification_get_instance_private (self);

  return priv->actions;
}


void
phosh_notification_set_urgency (PhoshNotification        *self,
                                PhoshNotificationUrgency  urgency)
{
  PhoshNotificationPrivate *priv;

  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  priv = phosh_notification_get_instance_private (self);

  if (priv->urgency == urgency)
    return;

  priv->urgency = urgency;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_URGENCY]);
}


PhoshNotificationUrgency
phosh_notification_get_urgency (PhoshNotification *self)
{
  PhoshNotificationPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self),
                        PHOSH_NOTIFICATION_URGENCY_NORMAL);
  priv = phosh_notification_get_instance_private (self);

  return priv->urgency;
}

/**
 * phosh_notification_set_transient:
 * @self: the #PhoshNotification
 * @transient: if @self is transient
 *
 * Set if @self should go to the message tray
 */
void
phosh_notification_set_transient (PhoshNotification *self,
                                  gboolean           transient)
{
  PhoshNotificationPrivate *priv;

  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  priv = phosh_notification_get_instance_private (self);

  if (priv->transient == transient)
    return;

  priv->transient = transient;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TRANSIENT]);
}

/**
 * phosh_notification_get_transient:
 * @self: the #PhoshNotification
 *
 * Transient notifications don't go to the message tray
 *
 * Returns: %TRUE when transient, otherwise %FALSE
 */
gboolean
phosh_notification_get_transient (PhoshNotification *self)
{
  PhoshNotificationPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), TRUE);
  priv = phosh_notification_get_instance_private (self);

  return priv->transient;
}

/**
 * phosh_notification_set_resident:
 * @self: the #PhoshNotification
 * @resident: is the notification resident
 *
 * Set whether or not invoking actions dismiss @self
 */
void
phosh_notification_set_resident (PhoshNotification *self,
                                 gboolean           resident)
{
  PhoshNotificationPrivate *priv;

  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  priv = phosh_notification_get_instance_private (self);

  if (priv->resident == resident)
    return;

  priv->resident = resident;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_RESIDENT]);
}

/**
 * phosh_notification_get_resident:
 * @self: the #PhoshNotification
 *
 * When %TRUE invoking an action _doesn't_ dismiss the notification
 *
 * Returns: %TRUE when resident, otherwise %FALSE
 */
gboolean
phosh_notification_get_resident (PhoshNotification *self)
{
  PhoshNotificationPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), FALSE);
  priv = phosh_notification_get_instance_private (self);

  return priv->resident;
}

/**
 * phosh_notification_set_category:
 * @self: the #PhoshNotification
 * @category: the new category
 *
 * Set the category of the notification, such as `email.arrived`
 */
void
phosh_notification_set_category (PhoshNotification *self,
                                 const char        *category)
{
  PhoshNotificationPrivate *priv;

  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  priv = phosh_notification_get_instance_private (self);

  if (g_strcmp0 (priv->category, category) == 0)
    return;

  g_clear_pointer (&priv->category, g_free);
  priv->category = g_strdup (category);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CATEGORY]);
}

/**
 * phosh_notification_get_category:
 * @self: the #PhoshNotification
 *
 * Get the category hint the notification was sent with
 *
 * See <https://specifications.freedesktop.org/notification-spec/notification-spec-latest.html>
 *
 * Returns: the category or %NULL
 */
const char *
phosh_notification_get_category (PhoshNotification *self)
{
  PhoshNotificationPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), NULL);
  priv = phosh_notification_get_instance_private (self);

  return priv->category;
}

/**
 * phosh_notification_set_profile:
 * @self: the #PhoshNotification
 * @profile: the feedback profile to use
 *
 * Set the feedback profile (constrained by feedbackd's
 * global policy) for the event related to this notification.
 */
void
phosh_notification_set_profile (PhoshNotification *self,
                                 const char        *profile)
{
  PhoshNotificationPrivate *priv;

  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  priv = phosh_notification_get_instance_private (self);

  if (g_strcmp0 (priv->profile, profile) == 0)
    return;

  g_clear_pointer (&priv->profile, g_free);
  priv->profile = g_strdup (profile);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PROFILE]);
}

/**
 * phosh_notification_get_profile:
 * @self: the #PhoshNotification
 *
 * Get the intended feedback profile for the event related to this
 * notification
 *
 * See the Feedback Theme Specification for details.
 *
 * Returns: the profile or %NULL
 */
const char *
phosh_notification_get_profile (PhoshNotification *self)
{
  PhoshNotificationPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), NULL);
  priv = phosh_notification_get_instance_private (self);

  return priv->profile;
}


void
phosh_notification_activate (PhoshNotification *self,
                             const char        *action)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  g_signal_emit (self, signals[SIGNAL_ACTIONED], 0, action);
}


static gboolean
expired (gpointer data)
{
  PhoshNotification *self = data;
  PhoshNotificationPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), G_SOURCE_REMOVE);
  priv = phosh_notification_get_instance_private (self);

  g_debug ("%i expired", priv->id);

  priv->timeout = 0;

  g_signal_emit (self, signals[SIGNAL_EXPIRED], 0);

  return G_SOURCE_REMOVE;
}

/**
 * phosh_notification_expires:
 * @self: the #PhoshNotification
 * @timeout: delay (in milliseconds)
 *
 * Set @self to expire after @timeout (from this call)
 *
 * Note: doesn't close the notification, for that call
 * [method@Notification.close] in response to [signal@Notification::expired].
 */
void
phosh_notification_expires (PhoshNotification *self,
                            int                timeout)
{
  PhoshNotificationPrivate *priv;

  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  g_return_if_fail (timeout > 0);
  priv = phosh_notification_get_instance_private (self);

  priv->timeout = g_timeout_add (timeout, expired, self);
  g_source_set_name_by_id (priv->timeout, "[phosh] notification_expires_id");
}

/**
 * phosh_notification_close:
 * @self: the #PhoshNotification
 * @reason: Why the notification is closing
 */
void
phosh_notification_close (PhoshNotification       *self,
                          PhoshNotificationReason  reason)
{
  PhoshNotificationPrivate *priv;

  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  priv = phosh_notification_get_instance_private (self);

  /* No point running the timeout, we're already closing */
  if (priv->timeout != 0) {
    g_source_remove (priv->timeout);
    priv->timeout = 0;
  }

  g_signal_emit (self, signals[SIGNAL_CLOSED], 0, reason);
}

void
phosh_notification_do_action (PhoshNotification *self, guint id, const char *action)
{
  g_return_if_fail (PHOSH_NOTIFICATION_GET_CLASS (self)->do_action);

  PHOSH_NOTIFICATION_GET_CLASS (self)->do_action (self, id, action);
}
