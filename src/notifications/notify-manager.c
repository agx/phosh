/*
 * Copyright (C) 2019 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-notify-manager"

#include "../config.h"

#include <gio/gdesktopappinfo.h>

#include "notification-banner.h"
#include "notification-list.h"
#include "notify-manager.h"
#include "shell.h"
#include "phosh-enums.h"

#define NOTIFICATION_DEFAULT_TIMEOUT 5000 /* ms */
#define NOTIFICATIONS_SPEC_VERSION "1.2"

#define NOTIFICATIONS_SCHEMA_ID "org.gnome.desktop.notifications"
#define NOTIFICATIONS_KEY_SHOW_BANNERS "show-banners"

/**
 * SECTION:notify-manager
 * @short_description: Provides the org.freedesktop.Notification DBus interface
 * @Title: PhoshNotifyManager
 *
 * See https://developer.gnome.org/notification-spec/
 */

#define NOTIFY_DBUS_NAME "org.freedesktop.Notifications"

static void phosh_notify_manager_notify_iface_init (
  PhoshNotifyDbusNotificationsIface *iface);

typedef struct _PhoshNotifyManager
{
  PhoshNotifyDbusNotificationsSkeleton parent;

  int dbus_name_id;
  guint next_id;
  guint unknown_source;
  gboolean show_banners;

  GSettings *settings;

  PhoshNotificationList *list;
} PhoshNotifyManager;

G_DEFINE_TYPE_WITH_CODE (PhoshNotifyManager,
                         phosh_notify_manager,
                         PHOSH_NOTIFY_DBUS_TYPE_NOTIFICATIONS_SKELETON,
                         G_IMPLEMENT_INTERFACE (
                           PHOSH_NOTIFY_DBUS_TYPE_NOTIFICATIONS,
                           phosh_notify_manager_notify_iface_init));


enum {
  SIGNAL_NEW_NOTIFICATION,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


static gboolean
handle_close_notification (PhoshNotifyDbusNotifications *skeleton,
                           GDBusMethodInvocation        *invocation,
                           guint                         arg_id)
{
  PhoshNotification *notification = NULL;
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (skeleton);

  g_return_val_if_fail (PHOSH_IS_NOTIFY_MANAGER (self), FALSE);
  g_debug ("DBus call CloseNotification %u", arg_id);

  notification = phosh_notification_list_get_by_id (self->list, arg_id);

  /*
   * ignore errors when closing non-existent notifcation, at least qt 5.11 is not
   * happy about it.
   */
  if (notification && PHOSH_IS_NOTIFICATION (notification)) {
    phosh_notification_close (notification, PHOSH_NOTIFICATION_REASON_CLOSED);
  } else {
    phosh_notify_dbus_notifications_emit_notification_closed (
      PHOSH_NOTIFY_DBUS_NOTIFICATIONS (self), arg_id,
      PHOSH_NOTIFICATION_REASON_CLOSED);
  }

  phosh_notify_dbus_notifications_complete_close_notification (
    skeleton, invocation);

  return TRUE;
}


static gboolean
handle_get_capabilities (PhoshNotifyDbusNotifications *skeleton,
                         GDBusMethodInvocation        *invocation)
{
  const gchar *const capabilities[] = {
    "body", "body-markup", "actions", "icon-static", NULL,
  };

  g_debug ("DBus call GetCapabilities");
  phosh_notify_dbus_notifications_complete_get_capabilities (
    skeleton, invocation, capabilities);
  return TRUE;
}


static gboolean
handle_get_server_information (PhoshNotifyDbusNotifications *skeleton,
                               GDBusMethodInvocation        *invocation)
{
  g_debug ("DBus call GetServerInformation");
  phosh_notify_dbus_notifications_complete_get_server_information (
    skeleton, invocation, "Phosh Notify Daemon", "Phosh", PHOSH_VERSION,
    NOTIFICATIONS_SPEC_VERSION);
  return TRUE;
}


static void
on_notification_expired (PhoshNotifyManager *self,
                         PhoshNotification  *notification)
{
  guint id = 0;

  g_return_if_fail (PHOSH_IS_NOTIFY_MANAGER (self));
  g_return_if_fail (PHOSH_IS_NOTIFICATION (notification));

  id = phosh_notification_get_id (notification);

  g_debug ("Notification %u expired", id);

  /* Transient notifications are closed rather than staying in the tray */
  if (phosh_notification_get_transient (notification)) {
    phosh_notification_close (notification,
                              PHOSH_NOTIFICATION_REASON_EXPIRED);
  }
}


static void
on_notification_actioned (PhoshNotifyManager *self,
                          const char         *action,
                          PhoshNotification  *notification)
{
  guint id;

  g_return_if_fail (PHOSH_IS_NOTIFY_MANAGER (self));
  g_return_if_fail (PHOSH_IS_NOTIFICATION (notification));

  id = phosh_notification_get_id (notification);

  g_return_if_fail (id);

  g_debug ("Emitting ActionInvoked: %d, %s", id, action);

  phosh_notify_dbus_notifications_emit_action_invoked (
    PHOSH_NOTIFY_DBUS_NOTIFICATIONS (self), id, action);

  /* Resident notifications stay after being actioned */
  if (!phosh_notification_get_resident (notification)) {
    phosh_notification_close (notification,
                              PHOSH_NOTIFICATION_REASON_DISMISSED);
  }
}


static void
on_notification_closed (PhoshNotifyManager      *self,
                        PhoshNotificationReason  reason,
                        PhoshNotification       *notification)
{
  guint id;

  g_return_if_fail (PHOSH_IS_NOTIFY_MANAGER (self));
  g_return_if_fail (PHOSH_IS_NOTIFICATION (notification));

  id = phosh_notification_get_id (notification);

  g_debug ("Emitting NotificationClosed: %d, %d", id, reason);

  phosh_notify_dbus_notifications_emit_notification_closed (
    PHOSH_NOTIFY_DBUS_NOTIFICATIONS (self), id, reason);
}


static GIcon *
parse_icon_data (GVariant *variant)
{
  GVariant *wrapped_data = NULL;
  guchar *data = NULL;
  GIcon *icon = NULL;
  int width = 0;
  int height = 0;
  int row_stride = 0;
  int has_alpha = 0;
  int sample_size = 0;
  int channels = 0;
  gsize size_should_be = 0;

  if (g_variant_is_of_type (variant, G_VARIANT_TYPE ("(iiibiiay)"))) {
    g_variant_get (variant,
                   "(iiibii@ay)",
                   &width,
                   &height,
                   &row_stride,
                   &has_alpha,
                   &sample_size,
                   &channels,
                   &wrapped_data);

    size_should_be = (height - 1) * row_stride + width * ((channels * sample_size + 7) / 8);

    if (size_should_be != g_variant_get_size (wrapped_data)) {
      g_warning ("Rejecting image, %" G_GSIZE_FORMAT
                 " (expected) != %" G_GSIZE_FORMAT,
                 size_should_be, g_variant_get_size (wrapped_data));

      return NULL;
    }

    // Extract a copy of the raw data
    data = (guchar *) g_memdup (g_variant_get_data (wrapped_data),
                                size_should_be);

    icon = G_ICON (gdk_pixbuf_new_from_data (data,
                                             GDK_COLORSPACE_RGB,
                                             has_alpha,
                                             sample_size,
                                             width,
                                             height,
                                             row_stride,
                                             (GdkPixbufDestroyNotify) g_free,
                                             NULL));
  }

  return icon;
}

static GIcon *
parse_icon_string (const char *string)
{
  g_autoptr (GFile) file = NULL;
  GIcon *icon = NULL;

  if (string == NULL || strlen (string) < 1) {
    return NULL;
  }

  if (g_str_has_prefix (string, "file://")) {
    file = g_file_new_for_uri (string);
    icon = g_file_icon_new (file);
  } else if (g_str_has_prefix (string, "/")) {
    file = g_file_new_for_path (string);
    icon = g_file_icon_new (file);
  } else {
    icon = g_themed_icon_new (string);
  }

  return icon;
}

static gboolean
handle_notify (PhoshNotifyDbusNotifications *skeleton,
               GDBusMethodInvocation        *invocation,
               const char                   *app_name,
               guint                         replaces_id,
               const gchar                  *app_icon,
               const gchar                  *summary,
               const gchar                  *body,
               const gchar * const          *actions,
               GVariant                     *hints,
               gint                          expire_timeout)
{
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (skeleton);
  PhoshNotification *notification = NULL;
  GVariant *item;
  GVariantIter iter;
  guint id;
  g_autofree gchar *desktop_id = NULL;
  g_autofree gchar *source_id = NULL;
  g_autoptr (GAppInfo) info = NULL;
  PhoshNotificationUrgency urgency = PHOSH_NOTIFICATION_URGENCY_NORMAL;
  g_autoptr (GIcon) data_gicon = NULL;
  g_autoptr (GIcon) path_gicon = NULL;
  g_autoptr (GIcon) app_gicon = NULL;
  g_autoptr (GIcon) old_data_gicon = NULL;
  g_autoptr (GIcon) fallback_gicon = NULL;
  gboolean transient = FALSE;
  gboolean resident = FALSE;
  g_autofree char *category = NULL;
  GIcon *icon = NULL;
  GIcon *image = NULL;
  g_autoptr (GDateTime) timestamp = g_date_time_new_now_local ();

  if (timestamp == NULL)
    g_warning ("Could not get local time");
  g_return_val_if_fail (PHOSH_IS_NOTIFY_MANAGER (self), FALSE);

  g_debug ("DBus call Notify: %s (%u): %s (%s), %s, %d", app_name, replaces_id, summary, body, app_icon, expire_timeout);

  app_gicon = parse_icon_string (app_icon);

  g_variant_iter_init (&iter, hints);
  while ((item = g_variant_iter_next_value (&iter))) {
    g_autofree gchar *key = NULL;
    g_autoptr(GVariant) value = NULL;

    g_variant_get (item, "{sv}", &key, &value);

    if (g_strcmp0 (key, "urgency") == 0) {
      if (g_variant_is_of_type (value, G_VARIANT_TYPE_BYTE) &&
          (g_variant_get_byte(value) == PHOSH_NOTIFICATION_URGENCY_CRITICAL)) {
        expire_timeout = 0;
      }
    } else if ((g_strcmp0 (key, "image-data") == 0) ||
               (g_strcmp0 (key, "image_data") == 0)) {
      data_gicon = parse_icon_data (value);
    } else if ((g_strcmp0 (key, "image-path") == 0) ||
               (g_strcmp0 (key, "image_path") == 0)) {
      if (g_variant_is_of_type (value, G_VARIANT_TYPE_STRING)) {
        path_gicon = parse_icon_string (g_variant_get_string (value, NULL));
      }
    } else if (g_strcmp0 (key, "icon_data") == 0) {
      old_data_gicon = parse_icon_data (value);
    } else if ((g_strcmp0 (key, "desktop_entry") == 0) ||
               (g_strcmp0 (key, "desktop-entry") == 0)) {
      if (g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
        desktop_id = g_variant_dup_string (value, NULL);
    } else if ((g_strcmp0 (key, "transient") == 0)) {
      if (g_variant_is_of_type (value, G_VARIANT_TYPE_BOOLEAN))
        transient = g_variant_get_boolean (value);
    } else if ((g_strcmp0 (key, "resident") == 0)) {
      if (g_variant_is_of_type (value, G_VARIANT_TYPE_BOOLEAN))
        resident = g_variant_get_boolean (value);
    } else if ((g_strcmp0 (key, "category") == 0)) {
      if (g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
        category = g_variant_dup_string (value, NULL);
    }

    g_variant_unref (item);
  }

  if (data_gicon) {
    image = data_gicon;
  } else if (path_gicon) {
    image = path_gicon;
  } else if (old_data_gicon) {
    image = old_data_gicon;
  } else if (urgency == PHOSH_NOTIFICATION_URGENCY_CRITICAL) {
    fallback_gicon = g_themed_icon_new ("dialog-error");
    image = fallback_gicon;
  } else {
    image = NULL;
  }

  icon = app_gicon;

  if (desktop_id) {
    GDesktopAppInfo *desktop_info;
    source_id = g_strdup_printf ("%s.desktop", desktop_id);

    desktop_info = g_desktop_app_info_new (source_id);

    if (desktop_info) {
      info = G_APP_INFO (desktop_info);
    }
  } else if (app_name && g_strcmp0 (app_name, "notify-send")) {
    /* When the app name is set (and isn't notify-send) use that
       as it's better than nothing  */
    source_id = g_strdup_printf ("legacy-app-%s", app_name);
  } else {
    /* Worse case: The notification gets it's own group, we don't know
       where it came from */
    source_id = g_strdup_printf ("unknown-app-%i", self->unknown_source++);
  }

  if (expire_timeout == -1)
    expire_timeout = NOTIFICATION_DEFAULT_TIMEOUT;

  if (replaces_id)
    notification = phosh_notification_list_get_by_id (self->list, replaces_id);

  if (notification) {
    id = replaces_id;

    g_object_set (notification,
                  "app_name", app_name,
                  "summary", summary,
                  "body", body,
                  "app-icon", icon,
                  "app-info", info,
                  "image", image,
                  "urgency", urgency,
                  "actions", actions,
                  "timestamp", timestamp,
                  NULL);
  } else {
    id = self->next_id++;

    notification = phosh_notification_new (id,
                                           app_name,
                                           info,
                                           summary,
                                           body,
                                           icon,
                                           image,
                                           urgency,
                                           (GStrv) actions,
                                           transient,
                                           resident,
                                           category,
                                           timestamp);

    phosh_notification_list_add (self->list, source_id, notification);

    g_signal_connect_object (notification,
                             "expired",
                             G_CALLBACK (on_notification_expired),
                             self,
                             G_CONNECT_SWAPPED);
    g_signal_connect_object (notification,
                             "actioned",
                             G_CALLBACK (on_notification_actioned),
                             self,
                             G_CONNECT_SWAPPED);
    g_signal_connect_object (notification,
                             "closed",
                             G_CALLBACK (on_notification_closed),
                             self,
                             G_CONNECT_SWAPPED);

    if (expire_timeout) {
      phosh_notification_expires (notification, expire_timeout);
    }

    g_signal_emit (self, signals[SIGNAL_NEW_NOTIFICATION], 0, notification);
  }

  phosh_notify_dbus_notifications_complete_notify (
    skeleton, invocation, id);

  return TRUE;
}


static void
phosh_notify_manager_notify_iface_init (PhoshNotifyDbusNotificationsIface *iface)
{
  iface->handle_close_notification = handle_close_notification;
  iface->handle_get_capabilities = handle_get_capabilities;
  iface->handle_get_server_information = handle_get_server_information;
  iface->handle_notify = handle_notify;
}


static void
on_notifications_setting_changed (PhoshNotifyManager *self,
                                  const gchar        *key,
                                  GSettings          *settings)
{
  g_return_if_fail (PHOSH_IS_NOTIFY_MANAGER (self));
  g_return_if_fail (G_IS_SETTINGS (settings));

  self->show_banners = g_settings_get_boolean (settings, NOTIFICATIONS_KEY_SHOW_BANNERS);
}


static void
on_name_acquired (GDBusConnection *connection,
                  const char      *name,
                  gpointer         user_data)
{
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (user_data);

  g_debug ("Acquired name %s", name);
  g_return_if_fail (PHOSH_IS_NOTIFY_MANAGER (self));
}


static void
on_name_lost (GDBusConnection *connection,
              const char      *name,
              gpointer         user_data)
{
  g_debug ("Lost or failed to acquire name %s", name);
}


static void
on_bus_acquired (GDBusConnection *connection,
                 const char      *name,
                 gpointer         user_data)
{
  PhoshNotifyManager *self = user_data;

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                    connection,
                                    "/org/freedesktop/Notifications",
                                    NULL);
}


static void
phosh_notify_manager_dispose (GObject *object)
{
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (object);

  g_clear_object (&self->settings);

  g_clear_object (&self->list);

  G_OBJECT_CLASS (phosh_notify_manager_parent_class)->dispose (object);
}


static void
phosh_notify_manager_constructed (GObject *object)
{
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (object);

  G_OBJECT_CLASS (phosh_notify_manager_parent_class)->constructed (object);
  self->dbus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                       NOTIFY_DBUS_NAME,
                                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                                       G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                       on_bus_acquired,
                                       on_name_acquired,
                                       on_name_lost,
                                       g_object_ref (self),
                                       g_object_unref);

  self->settings = g_settings_new (NOTIFICATIONS_SCHEMA_ID);
  g_signal_connect_swapped (self->settings, "changed::" NOTIFICATIONS_KEY_SHOW_BANNERS,
                            G_CALLBACK (on_notifications_setting_changed), self);
  on_notifications_setting_changed (self, NULL, self->settings);
}


static void
phosh_notify_manager_class_init (PhoshNotifyManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_notify_manager_constructed;
  object_class->dispose = phosh_notify_manager_dispose;


  /**
   * PhoshNotifyManager::new-notification:
   * @self: the #PhoshNotifyManager
   * @notification: the new #PhoshNotification
   *
   * Emitted when a new notification is received and a banner should (possibly)
   * be shown
   */
  signals[SIGNAL_NEW_NOTIFICATION] = g_signal_new ("new-notification",
                                                   G_TYPE_FROM_CLASS (klass),
                                                   G_SIGNAL_RUN_LAST,
                                                   0,
                                                   NULL,
                                                   NULL,
                                                   g_cclosure_marshal_VOID__OBJECT,
                                                   G_TYPE_NONE,
                                                   1,
                                                   PHOSH_TYPE_NOTIFICATION);
}


static void
phosh_notify_manager_init (PhoshNotifyManager *self)
{
  self->next_id = 1;

  self->list = phosh_notification_list_new ();
}


PhoshNotifyManager *
phosh_notify_manager_get_default (void)
{
  static PhoshNotifyManager *instance;

  if (instance == NULL) {
    instance = g_object_new (PHOSH_TYPE_NOTIFY_MANAGER, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
  }

  return instance;
}


/**
 * phosh_notify_manager_get_list:
 * @self: the #PhoshNotifyManager
 *
 * Get the #PhoshNotificationList of current notifications
 *
 * Returns: the #PhoshNotificationList
 */
PhoshNotificationList *
phosh_notify_manager_get_list (PhoshNotifyManager *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFY_MANAGER (self), NULL);

  return self->list;
}


/**
 * phosh_notify_manager_get_show_banners:
 * @self: the #PhoshNotifyManager
 *
 * Are notififcation banners enabled
 *
 * Returns: %TRUE if banners should be shown, otherwise %FALSE
 */
gboolean
phosh_notify_manager_get_show_banners (PhoshNotifyManager *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFY_MANAGER (self), FALSE);

  return self->show_banners;
}
