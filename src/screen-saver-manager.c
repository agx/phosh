/*
 * Copyright (C) 2019 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-screen-saver-manager"

#include "screen-saver-manager.h"

/**
 * SECTION:phosh-screen-saver-manager
 * @short_description: Provides the org.gnome.ScreenSaver DBus interface
 * @Title: PhoshScreenSaverManager
 *
 * See https://people.gnome.org/~mccann/gnome-screensaver/docs/gnome-screensaver.html
 * for a (a bit outdated) interface description.
 */

#define SCREEN_SAVER_DBUS_NAME "org.gnome.ScreenSaver"

static void phosh_screen_saver_manager_screen_saver_iface_init (
  PhoshScreenSaverDbusScreenSaverIface *iface);

typedef struct _PhoshScreenSaverManager
{
  PhoshScreenSaverDbusScreenSaverSkeleton parent;

  int dbus_name_id;
} PhoshScreenSaverManager;

G_DEFINE_TYPE_WITH_CODE (PhoshScreenSaverManager,
                         phosh_screen_saver_manager,
                         PHOSH_SCREEN_SAVER_DBUS_TYPE_SCREEN_SAVER_SKELETON,
                         G_IMPLEMENT_INTERFACE (
                           PHOSH_SCREEN_SAVER_DBUS_TYPE_SCREEN_SAVER,
                           phosh_screen_saver_manager_screen_saver_iface_init));

static gboolean
handle_get_active (PhoshScreenSaverDbusScreenSaver *skeleton,
                   GDBusMethodInvocation           *invocation)
{
  g_warning ("%s not implemented", __func__);
  return FALSE;
}

static gboolean
handle_get_active_time (PhoshScreenSaverDbusScreenSaver *skeleton,
                        GDBusMethodInvocation           *invocation)
{
  g_warning ("%s not implemented", __func__);
  return FALSE;

}

static gboolean
handle_lock (PhoshScreenSaverDbusScreenSaver *skeleton,
             GDBusMethodInvocation           *invocation)
{
  g_warning ("%s not implemented", __func__);
  return FALSE;
}

static gboolean
handle_set_active (PhoshScreenSaverDbusScreenSaver *skeleton,
                   GDBusMethodInvocation           *invocation,
                   gboolean                         arg_value)
{
  g_warning ("%s not implemented", __func__);
  return FALSE;
}


static void
phosh_screen_saver_manager_screen_saver_iface_init (PhoshScreenSaverDbusScreenSaverIface *iface)
{
  iface->handle_get_active = handle_get_active;
  iface->handle_get_active_time = handle_get_active_time;
  iface->handle_lock = handle_lock;
  iface->handle_set_active = handle_set_active;
}


static void
on_name_acquired (GDBusConnection *connection,
                  const char      *name,
                  gpointer         user_data)
{
  g_debug ("Acquired name %s", name);
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
  PhoshScreenSaverManager *self = user_data;

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                    connection,
                                    "/org/gnome/ScreenSaver",
                                    NULL);
}


static void
phosh_screen_saver_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (phosh_screen_saver_manager_parent_class)->finalize (object);
}


static void
phosh_screen_saver_manager_constructed (GObject *object)
{
  PhoshScreenSaverManager *self = PHOSH_SCREEN_SAVER_MANAGER (object);

  G_OBJECT_CLASS (phosh_screen_saver_manager_parent_class)->constructed (object);
  self->dbus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                       SCREEN_SAVER_DBUS_NAME,
                                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                                       G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                       on_bus_acquired,
                                       on_name_acquired,
                                       on_name_lost,
                                       g_object_ref (self),
                                       g_object_unref);
}


static void
phosh_screen_saver_manager_class_init (PhoshScreenSaverManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_screen_saver_manager_constructed;
  object_class->finalize = phosh_screen_saver_manager_finalize;
}


static void
phosh_screen_saver_manager_init (PhoshScreenSaverManager *self)
{
}

PhoshScreenSaverManager *
phosh_screen_saver_manager_get_default (void)
{
  static PhoshScreenSaverManager *instance;

  if (instance == NULL) {
    instance = g_object_new (PHOSH_TYPE_SCREEN_SAVER_MANAGER, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
  }
  return instance;
}
