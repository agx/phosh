/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-screen-saver-manager"

#include "screen-saver-manager.h"
#include "shell.h"
#include "lockscreen-manager.h"

/**
 * SECTION:screen-saver-manager
 * @short_description: Provides the org.gnome.ScreenSaver DBus interface
 * @Title: PhoshScreenSaverManager
 *
 * See https://people.gnome.org/~mccann/gnome-screensaver/docs/gnome-screensaver.html
 * for a (a bit outdated) interface description.
 */

#define SCREEN_SAVER_DBUS_NAME "org.gnome.ScreenSaver"

enum {
  PROP_0,
  PROP_LOCKSCREEN_MANAGER,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

static void phosh_screen_saver_manager_screen_saver_iface_init (
  PhoshScreenSaverDbusScreenSaverIface *iface);

typedef struct _PhoshScreenSaverManager
{
  PhoshScreenSaverDbusScreenSaverSkeleton parent;

  int dbus_name_id;
  PhoshLockscreenManager *lockscreen_manager;

} PhoshScreenSaverManager;

G_DEFINE_TYPE_WITH_CODE (PhoshScreenSaverManager,
                         phosh_screen_saver_manager,
                         PHOSH_SCREEN_SAVER_DBUS_TYPE_SCREEN_SAVER_SKELETON,
                         G_IMPLEMENT_INTERFACE (
                           PHOSH_SCREEN_SAVER_DBUS_TYPE_SCREEN_SAVER,
                           phosh_screen_saver_manager_screen_saver_iface_init));

static void
phosh_screen_saver_manager_set_property (GObject *object,
                                         guint property_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
  PhoshScreenSaverManager *self = PHOSH_SCREEN_SAVER_MANAGER (object);

  switch (property_id) {
  case PROP_LOCKSCREEN_MANAGER:
    self->lockscreen_manager = g_value_get_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_screen_saver_manager_get_property (GObject *object,
                                         guint property_id,
                                         GValue *value,
                                         GParamSpec *pspec)
{
  PhoshScreenSaverManager *self = PHOSH_SCREEN_SAVER_MANAGER (object);

  switch (property_id) {
  case PROP_LOCKSCREEN_MANAGER:
    g_value_set_object (value, self->lockscreen_manager);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static gboolean
handle_get_active (PhoshScreenSaverDbusScreenSaver *skeleton,
                   GDBusMethodInvocation           *invocation)
{
  PhoshScreenSaverManager *self = PHOSH_SCREEN_SAVER_MANAGER (skeleton);
  gboolean locked;

  g_return_val_if_fail (PHOSH_IS_SCREEN_SAVER_MANAGER (self), FALSE);
  g_return_val_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self->lockscreen_manager), FALSE);

  locked = phosh_lockscreen_manager_get_locked (self->lockscreen_manager);
  g_debug ("DBus call GetActive: %d", locked);

  phosh_screen_saver_dbus_screen_saver_complete_get_active (
    skeleton, invocation, locked);

  return TRUE;
}


static gboolean
handle_get_active_time (PhoshScreenSaverDbusScreenSaver *skeleton,
                        GDBusMethodInvocation           *invocation)
{
  PhoshScreenSaverManager *self = PHOSH_SCREEN_SAVER_MANAGER (skeleton);
  guint delta = 0; /* in seconds */
  guint64 active;

  g_return_val_if_fail (PHOSH_IS_SCREEN_SAVER_MANAGER (self), FALSE);
  g_return_val_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self->lockscreen_manager), FALSE);

  active = phosh_lockscreen_manager_get_active_time (self->lockscreen_manager);
  if (active)
    delta = (g_get_monotonic_time () - active) / 1000000;

  g_debug ("DBus GetActiveTime: %u", delta);
  phosh_screen_saver_dbus_screen_saver_complete_get_active_time (
    skeleton, invocation, delta);

  return TRUE;
}

static gboolean
handle_lock (PhoshScreenSaverDbusScreenSaver *skeleton,
             GDBusMethodInvocation           *invocation)
{
  PhoshScreenSaverManager *self = PHOSH_SCREEN_SAVER_MANAGER (skeleton);

  g_return_val_if_fail (PHOSH_IS_SCREEN_SAVER_MANAGER (self), FALSE);
  g_return_val_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self->lockscreen_manager), FALSE);

  g_debug ("DBus call lock");
  phosh_lockscreen_manager_set_locked (self->lockscreen_manager, TRUE);

  phosh_screen_saver_dbus_screen_saver_complete_lock (
    skeleton, invocation);

  return TRUE;
}

static gboolean
handle_set_active (PhoshScreenSaverDbusScreenSaver *skeleton,
                   GDBusMethodInvocation           *invocation,
                   gboolean                         lock)
{
  PhoshScreenSaverManager *self = PHOSH_SCREEN_SAVER_MANAGER (skeleton);

  g_return_val_if_fail (PHOSH_IS_SCREEN_SAVER_MANAGER (self), FALSE);
  g_return_val_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self->lockscreen_manager), FALSE);

  g_debug ("DBus call SetActive: %d", lock);
  if (lock) {
    phosh_lockscreen_manager_set_locked (self->lockscreen_manager, lock);
  } else {
    /* Screensaver active and screeen locked is the same so just
       discard requests to de-activate the screensaver */
    g_debug ("Ignoring request to deactivate screen saver");
  }

  phosh_screen_saver_dbus_screen_saver_complete_set_active (
    skeleton, invocation);

  return TRUE;
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
on_lockscreen_manager_notify_locked (PhoshScreenSaverManager *self,
                                     GParamSpec *pspec,
                                     PhoshLockscreenManager *lockscreen_manager)
{
  gboolean locked;
  GDBusInterfaceSkeleton *skeleton;

  g_return_if_fail (PHOSH_IS_SCREEN_SAVER_MANAGER (self));
  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (lockscreen_manager));

  skeleton = G_DBUS_INTERFACE_SKELETON (self);
  locked = phosh_lockscreen_manager_get_locked(self->lockscreen_manager);

  g_debug ("Signal ActiveChanged: %d", locked);
  g_dbus_connection_emit_signal (g_dbus_interface_skeleton_get_connection (skeleton),
                                 NULL,
                                 g_dbus_interface_skeleton_get_object_path (skeleton),
                                 SCREEN_SAVER_DBUS_NAME,
                                 "ActiveChanged",
                                 g_variant_new ("(b)", locked),
                                 NULL);
}



static void
on_lockscreen_manager_wakeup_outputs (PhoshScreenSaverManager *self,
                                      PhoshLockscreenManager *lockscreen_manager)
{
  GDBusInterfaceSkeleton *skeleton;

  g_return_if_fail (PHOSH_IS_SCREEN_SAVER_MANAGER (self));
  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (lockscreen_manager));

  skeleton = G_DBUS_INTERFACE_SKELETON (self);
  g_debug ("Signal WakeUpScreen");
  g_dbus_connection_emit_signal (g_dbus_interface_skeleton_get_connection (skeleton),
                                 NULL,
                                 g_dbus_interface_skeleton_get_object_path (skeleton),
                                 SCREEN_SAVER_DBUS_NAME,
                                 "WakeUpScreen",
                                 NULL,
                                 NULL);
}


static void
on_name_acquired (GDBusConnection *connection,
                  const char      *name,
                  gpointer         user_data)
{
  PhoshScreenSaverManager *self = PHOSH_SCREEN_SAVER_MANAGER (user_data);

  g_debug ("Acquired name %s", name);
  g_return_if_fail (PHOSH_IS_SCREEN_SAVER_MANAGER (self));

  /* Connect to lockscreen manager once we have a valid connection so we don't
     end up sending out signals early */
  g_object_connect (
    self->lockscreen_manager,
    "swapped-object-signal::notify::locked", G_CALLBACK (on_lockscreen_manager_notify_locked), self,
    "swapped-object-signal::wakeup-outputs", G_CALLBACK (on_lockscreen_manager_wakeup_outputs), self,
    NULL);

  on_lockscreen_manager_notify_locked (self, NULL, self->lockscreen_manager);
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
phosh_screen_saver_manager_dispose (GObject *object)
{
  PhoshScreenSaverManager *self = PHOSH_SCREEN_SAVER_MANAGER (object);

  g_clear_object (&self->lockscreen_manager);

  G_OBJECT_CLASS (phosh_screen_saver_manager_parent_class)->dispose (object);
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

  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self->lockscreen_manager));
}


static void
phosh_screen_saver_manager_class_init (PhoshScreenSaverManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_screen_saver_manager_constructed;
  object_class->dispose = phosh_screen_saver_manager_dispose;
  object_class->set_property = phosh_screen_saver_manager_set_property;
  object_class->get_property = phosh_screen_saver_manager_get_property;

  props[PROP_LOCKSCREEN_MANAGER] =
      g_param_spec_object ("lockscreen-manager",
                           "LockscreenManager",
                           "The lockscreen manager",
                           PHOSH_TYPE_LOCKSCREEN_MANAGER,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_screen_saver_manager_init (PhoshScreenSaverManager *self)
{
}


PhoshScreenSaverManager *
phosh_screen_saver_manager_get_default (PhoshLockscreenManager *lockscreen_manager)
{
  static PhoshScreenSaverManager *instance;

  if (instance == NULL) {
    g_return_val_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (lockscreen_manager), NULL);
    instance = g_object_new (PHOSH_TYPE_SCREEN_SAVER_MANAGER,
                             "lockscreen-manager", lockscreen_manager,
                             NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
  } else {
    /* switching lockscreen manager is not allowed */
    g_return_val_if_fail (instance->lockscreen_manager == lockscreen_manager, instance);
  }

  return instance;
}
