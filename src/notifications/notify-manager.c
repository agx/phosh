/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-notify-manager"

#include "../phosh-config.h"

#include <gio/gdesktopappinfo.h>

#include "dbus-notification.h"
#include "notification-banner.h"
#include "notification-list.h"
#include "notify-manager.h"
#include "notify-feedback.h"
#include "shell.h"
#include "phosh-enums.h"
#include "util.h"

#define NOTIFICATIONS_KEY_SHOW_BANNERS "show-banners"
#define NOTIFICATIONS_KEY_APP_CHILDREN "application-children"

#define NOTIFICATIONS_APP_SCHEMA_ID PHOSH_NOTIFICATIONS_SCHEMA_ID ".application"
#define NOTIFICATIONS_APP_PREFIX "/org/gnome/desktop/notifications/application"
#define NOTIFICATIONS_APP_KEY_SHOW_BANNERS "show-banners"
#define NOTIFICATIONS_APP_KEY_APP_ID "application-id"

#define NOTIFICATION_DEFAULT_TIMEOUT 5000 /* ms */
#define NOTIFICATIONS_SPEC_VERSION "1.2"

/**
 * PhoshNotifyManager:
 *
 * Manages notifications
 *
 * #PhoshNotifyManager manages notifications sent from the shell
 * itself and via the org.freedesktop.Notification DBus interface.
 * See https://developer.gnome.org/notification-spec/
 *
 * It maintains a list of notifications via a #PhoshNotificationList.
 */

#define NOTIFY_DBUS_NAME "org.freedesktop.Notifications"

enum {
  NEW_NOTIFICATION,
  NOTIFICATION_ACTIVATED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

typedef struct _PhoshNotifyManager
{
  PhoshNotifyDBusNotificationsSkeleton parent;

  int dbus_name_id;
  guint next_id;
  guint unknown_source;
  gboolean show_banners;
  GStrv app_children;

  GSettings *settings;

  /* Notification to be handled on unlock */
  struct {
    PhoshNotification *notification;
    char              *action;
  } unlock_notify;

  PhoshNotificationList *list;
  PhoshNotifyFeedback *feedback;
} PhoshNotifyManager;

static void phosh_notify_manager_notify_iface_init (PhoshNotifyDBusNotificationsIface *iface);
G_DEFINE_TYPE_WITH_CODE (PhoshNotifyManager,
                         phosh_notify_manager,
                         PHOSH_NOTIFY_DBUS_TYPE_NOTIFICATIONS_SKELETON,
                         G_IMPLEMENT_INTERFACE (
                           PHOSH_NOTIFY_DBUS_TYPE_NOTIFICATIONS,
                           phosh_notify_manager_notify_iface_init));

static gboolean
handle_close_notification (PhoshNotifyDBusNotifications *skeleton,
                           GDBusMethodInvocation        *invocation,
                           guint                         arg_id)
{
  PhoshNotification *notification = NULL;
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (skeleton);

  g_return_val_if_fail (PHOSH_IS_NOTIFY_MANAGER (self), FALSE);
  g_debug ("DBus call CloseNotification %u", arg_id);

  notification = phosh_notification_list_get_by_id (self->list, arg_id);

  /*
   * ignore errors when closing non-existent notification, at least qt 5.11 is not
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
handle_get_capabilities (PhoshNotifyDBusNotifications *skeleton,
                         GDBusMethodInvocation        *invocation)
{
  const char *const capabilities[] = {
    "body", "body-markup", "actions", "icon-static", NULL,
  };

  g_debug ("DBus call GetCapabilities");
  phosh_notify_dbus_notifications_complete_get_capabilities (
    skeleton, invocation, capabilities);
  return TRUE;
}


static gboolean
handle_get_server_information (PhoshNotifyDBusNotifications *skeleton,
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
on_unlock_notify_ref_gone (gpointer data, GObject *gone)
{
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (data);

  g_return_if_fail (PHOSH_IS_NOTIFY_MANAGER (self));

  self->unlock_notify.notification = NULL;
  g_clear_pointer (&self->unlock_notify.action, g_free);
}


static void
forget_unlock_notify (PhoshNotifyManager *self)
{
  if (!self->unlock_notify.notification)
    return;

  g_object_weak_unref (G_OBJECT (self->unlock_notify.notification),
                       on_unlock_notify_ref_gone,
                       self);
  self->unlock_notify.notification = NULL;
  g_clear_pointer (&self->unlock_notify.action, g_free);
}


static void
invoke_action (PhoshNotifyManager *self, PhoshNotification *notification, const gchar *action)
{
  guint id;

  id = phosh_notification_get_id (notification);

  g_return_if_fail (id);
  g_debug ("Emitting ActionInvoked: %d, %s", id, action);

  phosh_notification_do_action (notification, id, action);

  /* Resident notifications stay after being actioned */
  if (!phosh_notification_get_resident (notification)) {
    phosh_notification_close (notification,
                              PHOSH_NOTIFICATION_REASON_DISMISSED);
  }
  g_signal_emit (self, signals[NOTIFICATION_ACTIVATED], 0, notification);
}



static void
on_shell_lock_changed (PhoshNotifyManager* self, GParamSpec *pspec, PhoshShell *shell)
{
  gboolean locked;

  g_return_if_fail (PHOSH_IS_NOTIFY_MANAGER (self));

  locked = phosh_shell_get_locked (shell);
  if (!locked && self->unlock_notify.notification) {
    invoke_action (self, self->unlock_notify.notification, self->unlock_notify.action);
    forget_unlock_notify (self);
  }
}


static void
on_notification_actioned (PhoshNotifyManager *self,
                          const char         *action,
                          PhoshNotification  *notification)
{
  PhoshShell *shell = phosh_shell_get_default();

  g_return_if_fail (PHOSH_IS_NOTIFY_MANAGER (self));
  g_return_if_fail (PHOSH_IS_NOTIFICATION (notification));

  /*
   * Postpone action when shell is locked.
   * Only do this for the default action atm as other actions on the lock screen
   * must be a result of action filters and hence don't need unlock
   * TODO: Ensure this is really a result of a filtered action
   * Eventually we want to allow other actions that need unlock:
   * https://gitlab.freedesktop.org/xdg/xdg-specs/-/issues/103
   */
  if (phosh_shell_get_locked (shell) && g_strcmp0 (action, "default") == 0) {
    PhoshLockscreenManager *lm;

    /* Forget any pending actions */
    forget_unlock_notify (self);

    /* Clear out notification if it goes away in the meantime */
    g_object_weak_ref (G_OBJECT (notification),
                       on_unlock_notify_ref_gone,
                       self);
    self->unlock_notify.notification = notification;
    self->unlock_notify.action = g_strdup (action);

    /* Scroll to unlock page */
    lm = phosh_shell_get_lockscreen_manager (shell);
    phosh_lockscreen_manager_set_page (lm, PHOSH_LOCKSCREEN_PAGE_UNLOCK);
  } else {
    invoke_action (self, notification, action);
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

  if (g_variant_is_of_type (variant, G_VARIANT_TYPE ("(iiibiiay)"))) {
    gsize size_should_be;

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

    /* Extract a copy of the raw data */
    data = (guchar *) g_memdup2 (g_variant_get_data (wrapped_data), size_should_be);

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


static void
phosh_notify_manager_add_application (PhoshNotifyManager *self, GAppInfo *info)
{
  g_autofree char *munged_id = NULL;
  g_autofree char *path = NULL;
  g_autoptr (GSettings) settings = NULL;
  g_autoptr(GPtrArray) new_apps = NULL;
  const gchar *id;

  id = g_app_info_get_id(info);
  munged_id = phosh_munge_app_id (id);
  if (g_strv_contains ((const gchar * const *)self->app_children, munged_id))
    return;

  g_debug ("Adding new application: %s/%s", id, munged_id);
  new_apps = g_ptr_array_sized_new (g_strv_length (self->app_children) + 1);
  for (int i = 0; i < g_strv_length (self->app_children); i++)
    g_ptr_array_add (new_apps, self->app_children[i]);

  g_ptr_array_add (new_apps, munged_id);
  g_ptr_array_add (new_apps, NULL);

  path = g_strconcat (NOTIFICATIONS_APP_PREFIX, "/", munged_id, "/", NULL);
  settings = g_settings_new_with_path (NOTIFICATIONS_APP_SCHEMA_ID, path);
  g_settings_set_string (settings, NOTIFICATIONS_APP_KEY_APP_ID, id);
  g_settings_set_strv (self->settings, NOTIFICATIONS_KEY_APP_CHILDREN,
                       (const gchar * const *)new_apps->pdata);
}


static gboolean
handle_notify (PhoshNotifyDBusNotifications *skeleton,
               GDBusMethodInvocation        *invocation,
               const char                   *app_name,
               guint                         replaces_id,
               const char                   *app_icon,
               const char                   *summary,
               const char                   *body,
               const char *const            *actions,
               GVariant                     *hints,
               int                           expire_timeout)
{
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (skeleton);
  PhoshNotification *notification = NULL;
  GVariant *item;
  GVariantIter iter;
  guint id;
  g_autofree char *desktop_id = NULL;
  g_autofree char *source_id = NULL;
  g_autofree char *escaped_body = NULL;
  g_autoptr (GAppInfo) info = NULL;
  PhoshNotificationUrgency urgency = PHOSH_NOTIFICATION_URGENCY_NORMAL;
  g_autoptr (GIcon) data_gicon = NULL;
  g_autoptr (GIcon) path_gicon = NULL;
  g_autoptr (GIcon) app_gicon = NULL;
  g_autoptr (GIcon) old_data_gicon = NULL;
  gboolean transient = FALSE;
  gboolean resident = FALSE;
  g_autofree char *category = NULL;
  g_autofree char *profile = NULL;
  GIcon *icon = NULL;
  GIcon *image = NULL;

  g_return_val_if_fail (PHOSH_IS_NOTIFY_MANAGER (self), FALSE);

  g_debug ("DBus call Notify: %s (%u): %s (%s), %s, %d", app_name, replaces_id, summary, body, app_icon, expire_timeout);

  app_gicon = parse_icon_string (app_icon);

  g_variant_iter_init (&iter, hints);
  while ((item = g_variant_iter_next_value (&iter))) {
    g_autofree char *key = NULL;
    g_autoptr(GVariant) value = NULL;

    g_variant_get (item, "{sv}", &key, &value);

    if (g_strcmp0 (key, "urgency") == 0) {
      if (g_variant_is_of_type (value, G_VARIANT_TYPE_BYTE) &&
          (g_variant_get_byte(value) == PHOSH_NOTIFICATION_URGENCY_CRITICAL)) {
        expire_timeout = 0;
        urgency = PHOSH_NOTIFICATION_URGENCY_CRITICAL;
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
    } else if ((g_strcmp0 (key, "x-phosh-fb-profile") == 0)) {
      if (g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
        profile = g_variant_dup_string (value, NULL);
    }

    g_variant_unref (item);
  }

  if (data_gicon) {
    image = data_gicon;
  } else if (path_gicon) {
    image = path_gicon;
  } else if (old_data_gicon) {
    image = old_data_gicon;
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

  if (replaces_id)
    notification = phosh_notification_list_get_by_id (self->list, replaces_id);

  escaped_body = phosh_util_escape_markup (body, TRUE);

  if (notification) {
    id = replaces_id;

    g_object_set (notification,
                  "app_name", app_name,
                  "summary", summary,
                  "body", escaped_body,
                  "app-icon", icon,
                  "app-info", info,
                  "image", image,
                  "urgency", urgency,
                  "actions", actions,
                  "timestamp", NULL,
                  NULL);
  } else {
    g_autoptr(PhoshDBusNotification) dbus_notification = NULL;

    if (info)
      phosh_notify_manager_add_application (self, info);

    id = phosh_notify_manager_get_notification_id (self);
    dbus_notification = phosh_dbus_notification_new (id,
                                                     app_name,
                                                     info,
                                                     summary,
                                                     escaped_body,
                                                     icon,
                                                     image,
                                                     urgency,
                                                     (GStrv) actions,
                                                     transient,
                                                     resident,
                                                     category,
                                                     profile,
                                                     NULL);

    phosh_notify_manager_add_notification (self,
                                           source_id,
                                           expire_timeout,
                                           PHOSH_NOTIFICATION (dbus_notification));
  }

  phosh_notify_dbus_notifications_complete_notify (
    skeleton, invocation, id);

  return TRUE;
}


static void
phosh_notify_manager_notify_iface_init (PhoshNotifyDBusNotificationsIface *iface)
{
  iface->handle_close_notification = handle_close_notification;
  iface->handle_get_capabilities = handle_get_capabilities;
  iface->handle_get_server_information = handle_get_server_information;
  iface->handle_notify = handle_notify;
}


static void
on_notifications_setting_changed (PhoshNotifyManager *self,
                                  const char         *key,
                                  GSettings          *settings)
{
  g_return_if_fail (PHOSH_IS_NOTIFY_MANAGER (self));
  g_return_if_fail (G_IS_SETTINGS (settings));

  self->show_banners = g_settings_get_boolean (settings, NOTIFICATIONS_KEY_SHOW_BANNERS);
}


static void
on_notification_apps_setting_changed (PhoshNotifyManager *self,
                                      const char         *key,
                                      GSettings          *settings)
{
  g_return_if_fail (PHOSH_IS_NOTIFY_MANAGER (self));
  g_return_if_fail (G_IS_SETTINGS (settings));

  g_strfreev (self->app_children);
  self->app_children = g_settings_get_strv (settings, NOTIFICATIONS_KEY_APP_CHILDREN);
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

  g_clear_handle_id (&self->dbus_name_id, g_bus_unown_name);

  if (g_dbus_interface_skeleton_get_object_path (G_DBUS_INTERFACE_SKELETON (self)))
    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (self));

  g_clear_object (&self->settings);
  g_clear_object (&self->feedback);
  g_clear_object (&self->list);

  G_OBJECT_CLASS (phosh_notify_manager_parent_class)->dispose (object);
}


static void
phosh_notify_manager_finalize (GObject *object)
{
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (object);

  g_strfreev (self->app_children);

  G_OBJECT_CLASS (phosh_notify_manager_parent_class)->finalize (object);
}



static void
phosh_notify_manager_constructed (GObject *object)
{
  PhoshNotifyManager *self = PHOSH_NOTIFY_MANAGER (object);
  PhoshShell *shell = phosh_shell_get_default();

  G_OBJECT_CLASS (phosh_notify_manager_parent_class)->constructed (object);
  self->dbus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                       NOTIFY_DBUS_NAME,
                                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                                       G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                       on_bus_acquired,
                                       on_name_acquired,
                                       on_name_lost,
                                       self,
                                       NULL);

  self->settings = g_settings_new (PHOSH_NOTIFICATIONS_SCHEMA_ID);
  g_signal_connect_swapped (self->settings, "changed::" NOTIFICATIONS_KEY_SHOW_BANNERS,
                            G_CALLBACK (on_notifications_setting_changed), self);
  on_notifications_setting_changed (self, NULL, self->settings);

  g_signal_connect_swapped (self->settings, "changed::" NOTIFICATIONS_KEY_APP_CHILDREN,
                            G_CALLBACK (on_notification_apps_setting_changed), self);
  on_notification_apps_setting_changed (self, NULL, self->settings);

  g_signal_connect_swapped (shell, "notify::locked", G_CALLBACK (on_shell_lock_changed), self);

  self->feedback = phosh_notify_feedback_new (self->list);
}


static void
phosh_notify_manager_class_init (PhoshNotifyManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_notify_manager_constructed;
  object_class->dispose = phosh_notify_manager_dispose;
  object_class->finalize = phosh_notify_manager_finalize;

  /**
   * PhoshNotifyManager::new-notification:
   * @self: the #PhoshNotifyManager
   * @notification: the new #PhoshNotification
   *
   * Emitted when a new notification is received and a banner should (possibly)
   * be shown
   */
  signals[NEW_NOTIFICATION] = g_signal_new ("new-notification",
                                            G_TYPE_FROM_CLASS (klass),
                                            G_SIGNAL_RUN_LAST,
                                            0,
                                            NULL,
                                            NULL,
                                            g_cclosure_marshal_VOID__OBJECT,
                                            G_TYPE_NONE,
                                            1,
                                            PHOSH_TYPE_NOTIFICATION);

  /**
   * PhoshNotifyManager::notification-activated:
   * @self: the #PhoshNotifyManager
   * @notification: the #PhoshNotification
   *
   * Emitted when the action on a notification gets activated.
   */
  signals[NOTIFICATION_ACTIVATED] = g_signal_new ("notification-activated",
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

/**
 * phosh_notify_manager_get_default:
 *
 * Get the notify manager singleton
 *
 * Returns:(transfer none): The notify manager singleton
 */
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
 * Returns:(transfer none): The #PhoshNotificationList
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

/**
 * phosh_notify_manager_get_notification_id:
 * @self: the #PhoshNotifyManager
 *
 * Get a notification id
 *
 * Returns: a notification id that can be used to create new
 * notifications.
 */
guint
phosh_notify_manager_get_notification_id (PhoshNotifyManager *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFY_MANAGER (self), FALSE);

  return self->next_id++;
}

/**
 * phosh_notify_manager_add_notification
 * @self: the #PhoshNotifyManager
 * @source_id: The notification source's app_id
 * @expire_timeout: When the notification should expire
 * @notification: The notification
 *
 * Adds @notification to the current list of notifications.
 */
void
phosh_notify_manager_add_notification (PhoshNotifyManager *self,
                                       const gchar *source_id,
                                       int expire_timeout,
                                       PhoshNotification *notification)
{
  g_return_if_fail (PHOSH_IS_NOTIFY_MANAGER (self));
  g_return_if_fail (PHOSH_IS_NOTIFICATION (notification));
  g_return_if_fail (source_id);

  if (expire_timeout == -1)
    expire_timeout = NOTIFICATION_DEFAULT_TIMEOUT;

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

  g_signal_emit (self, signals[NEW_NOTIFICATION], 0, notification);
}

gboolean
phosh_notify_manager_close_notification_by_id (PhoshNotifyManager *self,
                                               int id,
                                               PhoshNotificationReason reason)
{
  PhoshNotification *notification = NULL;

  g_return_val_if_fail (PHOSH_IS_NOTIFY_MANAGER (self), FALSE);

  notification = phosh_notification_list_get_by_id (self->list, id);
  if (!notification)
    return FALSE;

  phosh_notification_close (notification, reason);
  return TRUE;
}

/**
 * phosh_notify_manager_get_show_notfication_banner:
 * @self: the #PhoshNotifyManager
 * @notification: the #PhoshNotification in question
 *
 * Checks whether a #PhoshNotificationBanner should be displayed
 * for the given #PhoshNotification according to current policy.
 *
 * Returns: %TRUE if the banner should be shown, otherwise %FALSE
 */
gboolean
phosh_notify_manager_get_show_notification_banner (PhoshNotifyManager *self,
                                                   PhoshNotification  *notification)
{
  g_autoptr (GSettings) settings = NULL;
  g_autofree char *path = NULL;
  g_autofree char *munged_id = NULL;
  GAppInfo *app_info;
  gboolean show;

  g_return_val_if_fail (PHOSH_IS_NOTIFY_MANAGER (self), FALSE);

  if (!self->show_banners)
    return FALSE;

  if (phosh_notification_get_urgency (notification) == PHOSH_NOTIFICATION_URGENCY_CRITICAL)
    return TRUE;

  app_info = phosh_notification_get_app_info (notification);
  if (!app_info)
    return TRUE;

  munged_id = phosh_munge_app_id (g_app_info_get_id(app_info));
  path = g_strconcat (NOTIFICATIONS_APP_PREFIX, "/", munged_id, "/", NULL);
  settings = g_settings_new_with_path (NOTIFICATIONS_APP_SCHEMA_ID, path);
  show = g_settings_get_boolean (settings, NOTIFICATIONS_APP_KEY_SHOW_BANNERS);

  g_debug ("Show banners for %s: %d", munged_id, show);
  return show;
}

/**
 * phosh_notify_manager_close_all:
 * @self: the #PhoshNotifyManager
 * @rease: the #PhoshNotificationReason
 *
 * Closes all notifications using #PhoshNotificationReason as reason.
 */
void
phosh_notify_manager_close_all_notifications (PhoshNotifyManager      *self,
                                              PhoshNotificationReason  reason)
{
  GListModel *source_list;

  source_list = G_LIST_MODEL (phosh_notify_manager_get_list (self));

  for (int i = g_list_model_get_n_items(G_LIST_MODEL (source_list)); i > 0; i--) {
    g_autoptr (GListModel) notif_list = G_LIST_MODEL (g_list_model_get_object (source_list, i-1));

    for (int j = g_list_model_get_n_items(G_LIST_MODEL (notif_list)); j > 0; j--) {
      g_autoptr (PhoshNotification) notification = PHOSH_NOTIFICATION (g_list_model_get_object (notif_list, j-1));
      phosh_notification_close (notification, PHOSH_NOTIFICATION_REASON_DISMISSED);
    }
  }
}

/**
 * phosh_notify_manager_shell_notification_new:
 * @self: The #PhoshNotifyManager
 * @summary: The notification summary
 * @body:(nullable): The notification body
 * @icon_name:(nullable): The icon to be used
 *
 * Creates a new notification and adds it to the list of notifications
 * (filling in then notification id) and displaying it (if
 * notifications aren't disabled by other means). The returned id
 * can be used to retract the notification.
 *
 * Returns: The id of the notification
 */
guint
phosh_notify_manager_add_shell_notification (PhoshNotifyManager *self,
                                             const char         *summary,
                                             const char         *body,
                                             const char         *icon_name,
                                             int                 expire_timeout)
{
  g_autoptr (GIcon) app_icon = NULL;
  g_autoptr (GIcon) icon = NULL;
  g_autoptr (PhoshNotification) notification = NULL;
  GDesktopAppInfo *desktop_info;
  g_autoptr (GAppInfo) info = NULL;
  guint id;

  id = phosh_notify_manager_get_notification_id (self);

  app_icon = g_themed_icon_new ("applications-system-symbolic");
  desktop_info = g_desktop_app_info_new (PHOSH_APP_ID ".desktop");
  if (desktop_info)
    info = G_APP_INFO (desktop_info);

  if (icon_name)
    icon = g_themed_icon_new (icon_name);

  notification =  g_object_new (PHOSH_TYPE_NOTIFICATION,
                                "id", id,
                                "summary", summary,
                                "body", body,
                                "app-info", info,
                                "app-icon", app_icon,
                                "image", icon,
                                "urgency", PHOSH_NOTIFICATION_URGENCY_NORMAL,
                                "actions", NULL,
                                "transient", !!expire_timeout,
                                "resident", FALSE,
                                NULL);

  phosh_notify_manager_add_notification (self, PHOSH_APP_ID ".desktop",
                                         expire_timeout,
                                         PHOSH_NOTIFICATION (notification));
  return id;
}
