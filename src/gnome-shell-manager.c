/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-gnome-shell-manager"

#include "../config.h"

#include "gnome-shell-manager.h"
#include "shell.h"

/**
 * SECTION:gnome-shell-manager
 * @short_description: Provides the org.gnome.Shell DBus interface
 * @Title: PhoshGnomeShellManager
 *
 */

#define NOTIFY_DBUS_NAME "org.gnome.Shell"

static void phosh_gnome_shell_manager_shell_iface_init (PhoshGnomeShellDBusShellIface *iface);

typedef struct _PhoshGnomeShellManager {
  PhoshGnomeShellDBusShellSkeleton parent;

  int                              dbus_name_id;
} PhoshGnomeShellManager;

G_DEFINE_TYPE_WITH_CODE (PhoshGnomeShellManager,
                         phosh_gnome_shell_manager,
                         PHOSH_GNOME_SHELL_DBUS_TYPE_SHELL_SKELETON,
                         G_IMPLEMENT_INTERFACE (
                           PHOSH_GNOME_SHELL_DBUS_TYPE_SHELL,
                           phosh_gnome_shell_manager_shell_iface_init));

static gboolean
handle_show_monitor_labels (PhoshGnomeShellDBusShell *skeleton,
                            GDBusMethodInvocation    *invocation,
                            GVariant                 *arg_params)
{
  PhoshGnomeShellManager *self = PHOSH_GNOME_SHELL_MANAGER (skeleton);

  g_return_val_if_fail (PHOSH_IS_GNOME_SHELL_MANAGER (self), FALSE);
  g_debug ("DBus show monitor labels");

  phosh_gnome_shell_dbus_shell_complete_show_monitor_labels (
    skeleton, invocation);

  return TRUE;
}

static gboolean
handle_hide_monitor_labels (PhoshGnomeShellDBusShell *skeleton,
                            GDBusMethodInvocation    *invocation)
{
  PhoshGnomeShellManager *self = PHOSH_GNOME_SHELL_MANAGER (skeleton);

  g_return_val_if_fail (PHOSH_IS_GNOME_SHELL_MANAGER (self), FALSE);
  g_debug ("DBus hide monitor labels");

  phosh_gnome_shell_dbus_shell_complete_hide_monitor_labels (
    skeleton, invocation);

  return TRUE;
}

static void
phosh_gnome_shell_manager_shell_iface_init (PhoshGnomeShellDBusShellIface *iface)
{
  iface->handle_show_monitor_labels = handle_show_monitor_labels;
  iface->handle_hide_monitor_labels = handle_hide_monitor_labels;
}


static void
on_name_acquired (GDBusConnection *connection,
                  const char      *name,
                  gpointer         user_data)
{
  PhoshGnomeShellManager *self = PHOSH_GNOME_SHELL_MANAGER (user_data);

  g_debug ("Acquired name %s", name);
  g_return_if_fail (PHOSH_IS_GNOME_SHELL_MANAGER (self));
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
  PhoshGnomeShellManager *self = user_data;

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                    connection,
                                    "/org/gnome/Shell",
                                    NULL);
}


static void
phosh_gnome_shell_manager_dispose (GObject *object)
{
  PhoshGnomeShellManager *self = PHOSH_GNOME_SHELL_MANAGER (object);

  G_OBJECT_CLASS (phosh_gnome_shell_manager_parent_class)->dispose (object);
}


static void
phosh_gnome_shell_manager_constructed (GObject *object)
{
  PhoshGnomeShellManager *self = PHOSH_GNOME_SHELL_MANAGER (object);

  G_OBJECT_CLASS (phosh_gnome_shell_manager_parent_class)->constructed (object);
  self->dbus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                       NOTIFY_DBUS_NAME,
                                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                                       G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                       on_bus_acquired,
                                       on_name_acquired,
                                       on_name_lost,
                                       g_object_ref (self),
                                       g_object_unref);
}


static void
phosh_gnome_shell_manager_class_init (PhoshGnomeShellManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_gnome_shell_manager_constructed;
  object_class->dispose = phosh_gnome_shell_manager_dispose;
}


static void
phosh_gnome_shell_manager_init (PhoshGnomeShellManager *self)
{
}


PhoshGnomeShellManager *
phosh_gnome_shell_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_GNOME_SHELL_MANAGER, NULL);
}
