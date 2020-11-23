/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-mount-notification"

#include "config.h"
#include "mount-notification.h"
#include "shell.h"

#include <gio/gio.h>
#include <glib/gi18n.h>

/**
 * SECTION:mount-notification
 * @short_description: A notifiction shown when a device got mounted
 * @Title: PhoshMountNotification
 *
 * The #PhoshMountNotification is responsible for showing the necessary
 * information when a device got mounted.
 */

typedef struct _PhoshMountNotification {
  PhoshNotification parent;

  gboolean dummy;
} PhoshMountNotification;


G_DEFINE_TYPE (PhoshMountNotification, phosh_mount_notification, PHOSH_TYPE_NOTIFICATION)


static void
phosh_mount_notification_do_action (PhoshNotification *notification, guint id, const char *action)
{
  g_autoptr (GAppInfo) info = g_app_info_get_default_for_type ("inode/directory", FALSE);
  g_autoptr (GError) err = NULL;
  g_autoptr (GList) l = NULL;

  g_debug ("Action %s for %d", action, id);

  /* FIXME: async, do the heavy lifting in mount-manager */
  l = g_list_append (l, (gpointer)action);
  if (!g_app_info_launch_uris (info, l, NULL, &err)) {
    g_warning ("Failed to launch %s %s", g_app_info_get_name (info),
               action);
  }
}


static void
phosh_mount_notification_class_init (PhoshMountNotificationClass *klass)
{
  PhoshNotificationClass *notification_class = PHOSH_NOTIFICATION_CLASS (klass);

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
  g_autoptr (PhoshMountNotification) notification = NULL;
  g_autoptr (GFile) root = NULL;
  gchar *actions[] = { NULL, _("Open"), NULL };

  name = g_mount_get_name (mount);
  g_debug ("Mount '%s' added", name);
  icon = g_mount_get_symbolic_icon (mount);
  root = g_mount_get_root (mount);
  uri = g_file_get_uri (root);
  if (uri)
    actions[0] = uri;
  return g_object_new (PHOSH_TYPE_MOUNT_NOTIFICATION,
                       "id", id,
                       "summary", name,
                       "app-name", PHOSH_APP_ID,
                       "image", icon,
                       "urgency", PHOSH_NOTIFICATION_URGENCY_NORMAL,
                       "actions", uri ? actions : NULL,
                       "transient", FALSE,
                       "resident", TRUE,
                       NULL);
}
