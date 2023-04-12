/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-mount-notification"

#include "phosh-config.h"
#include "mount-notification.h"
#include "shell.h"

#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

/**
 * PhoshMountNotification:
 *
 * A notifiction shown when a device got mounted
 *
 * The #PhoshMountNotification is responsible for showing the necessary
 * information when a device got mounted and providing the open action.
 */

typedef struct _PhoshMountNotification {
  PhoshNotification parent;

  GCancellable *cancellable;
} PhoshMountNotification;


G_DEFINE_TYPE (PhoshMountNotification, phosh_mount_notification, PHOSH_TYPE_NOTIFICATION)


static void
on_launch_finished (GAppInfo               *source,
                    GAsyncResult           *result,
                    PhoshMountNotification *self)
{
  g_autoptr (GError) err = NULL;

  if (!g_app_info_launch_uris_finish (source, result, &err)) {
    g_warning ("Failed to open %s: %s",
               phosh_notification_get_summary (PHOSH_NOTIFICATION (self)),
               err->message);
  }

  g_object_unref (self);
}


static void
phosh_mount_notification_do_action (PhoshNotification *notification, guint id, const char *action)
{
  PhoshMountNotification *self = PHOSH_MOUNT_NOTIFICATION (notification);
  g_autoptr (GAppInfo) info = g_app_info_get_default_for_type ("inode/directory", FALSE);
  g_autoptr (GList) l = NULL;
  g_autoptr (GdkAppLaunchContext) context = NULL;

  g_debug ("Action %s for %d", action, id);

  if (!G_IS_APP_INFO (info)) {
    g_warning ("No handler for inode/directory");
    return;
  }

  l = g_list_append (l, (gpointer)action);
  context = phosh_shell_get_app_launch_context (phosh_shell_get_default ());
  self->cancellable = g_cancellable_new ();
  g_app_info_launch_uris_async (info,
                                l,
                                G_APP_LAUNCH_CONTEXT (context),
                                self->cancellable,
                                (GAsyncReadyCallback)on_launch_finished,
                                g_object_ref (self));
}


static void
phosh_mount_notification_dispose (GObject *object)
{
  PhoshMountNotification *self = PHOSH_MOUNT_NOTIFICATION (object);

  g_clear_pointer (&self->cancellable, g_cancellable_cancel);

  G_OBJECT_CLASS (phosh_mount_notification_parent_class)->dispose (object);
}


static void
phosh_mount_notification_class_init (PhoshMountNotificationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhoshNotificationClass *notification_class = PHOSH_NOTIFICATION_CLASS (klass);

  object_class->dispose = phosh_mount_notification_dispose;
  notification_class->do_action = phosh_mount_notification_do_action;
}


static void
phosh_mount_notification_init (PhoshMountNotification *self)
{
}


PhoshMountNotification *
phosh_mount_notification_new_from_mount (guint id, GMount *mount)
{
  g_autofree gchar *name = NULL;
  g_autofree gchar *uri = NULL;
  g_autoptr (GIcon) icon = NULL;
  g_autoptr (GIcon) app_icon = NULL;
  g_autoptr (GFile) root = NULL;
  g_autoptr (GAppInfo) info = NULL;
  g_autoptr (GAppInfo) handler_info = NULL;
  GDesktopAppInfo *desktop_info;
  gchar *actions[] = { NULL, _("Open"), NULL };

  name = g_mount_get_name (mount);
  g_debug ("Mount '%s' added", name);
  icon = g_mount_get_symbolic_icon (mount);
  root = g_mount_get_root (mount);

  handler_info = g_app_info_get_default_for_type ("inode/directory", FALSE);
  /* Only add an action if we have a handler */
  if (handler_info) {
    uri = g_file_get_uri (root);
    if (uri)
      actions[0] = uri;
  }

  app_icon = g_themed_icon_new ("applications-system-symbolic");
  desktop_info = g_desktop_app_info_new (PHOSH_APP_ID ".desktop");
  if (desktop_info) {
    info = G_APP_INFO (desktop_info);
  }

  return g_object_new (PHOSH_TYPE_MOUNT_NOTIFICATION,
                       "id", id,
                       "summary", name,
                       "app-info", info,
                       "app-icon", app_icon,
                       "image", icon,
                       "urgency", PHOSH_NOTIFICATION_URGENCY_NORMAL,
                       "actions", uri ? actions : NULL,
                       "transient", FALSE,
                       "resident", FALSE,
                       "category", "device.added",
                       NULL);
}
