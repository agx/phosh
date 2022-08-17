/*
 * Copyright (C) 2019-2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-screen-saver-manager"

#include "screen-saver-manager.h"
#include "shell.h"
#include "idle-manager.h"
#include "login1-manager-dbus.h"
#include "login1-session-dbus.h"
#include "lockscreen-manager.h"
#include "util.h"

/**
 * SECTION:screen-saver-manager
 * @short_description: Provides the org.gnome.ScreenSaver DBus interface and handles logind's Session
 * @Title: PhoshScreenSaverManager
 *
 * See https://people.gnome.org/~mccann/gnome-screensaver/docs/gnome-screensaver.html
 * for a (a bit outdated) interface description. It also handles the login1 session
 * parts since those are closely related and this keeps #PhoshLockscreenManager free
 * of the DBus handling.
 */

#define SCREEN_SAVER_DBUS_NAME "org.gnome.ScreenSaver"

#define LOGIN_BUS_NAME "org.freedesktop.login1"
#define LOGIN_OBJECT_PATH "/org/freedesktop/login1"

enum {
  PROP_0,
  PROP_LOCKSCREEN_MANAGER,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

static void phosh_screen_saver_manager_screen_saver_iface_init (
  PhoshScreenSaverDBusScreenSaverIface *iface);

typedef struct _PhoshScreenSaverManager
{
  PhoshScreenSaverDBusScreenSaverSkeleton parent;

  int idle_id;
  int dbus_name_id;
  PhoshLockscreenManager *lockscreen_manager;

  PhoshDBusLoginSession *logind_session_proxy;
  PhoshDBusLoginManager *logind_manager_proxy;

  GCancellable          *cancel;
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
    self->lockscreen_manager = g_value_dup_object (value);
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
handle_get_active (PhoshScreenSaverDBusScreenSaver *skeleton,
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
handle_get_active_time (PhoshScreenSaverDBusScreenSaver *skeleton,
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
handle_lock (PhoshScreenSaverDBusScreenSaver *skeleton,
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
handle_set_active (PhoshScreenSaverDBusScreenSaver *skeleton,
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
phosh_screen_saver_manager_screen_saver_iface_init (PhoshScreenSaverDBusScreenSaverIface *iface)
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

  g_debug ("Signaling ActiveChanged: %d", locked);
  g_dbus_connection_emit_signal (g_dbus_interface_skeleton_get_connection (skeleton),
                                 NULL,
                                 g_dbus_interface_skeleton_get_object_path (skeleton),
                                 SCREEN_SAVER_DBUS_NAME,
                                 "ActiveChanged",
                                 g_variant_new ("(b)", locked),
                                 NULL);
}

static void
phosh_screen_saver_manager_wakeup_screen (PhoshScreenSaverManager *self)
{
  GDBusInterfaceSkeleton *skeleton;

  g_return_if_fail (PHOSH_IS_SCREEN_SAVER_MANAGER (self));

  skeleton = G_DBUS_INTERFACE_SKELETON (self);
  g_debug ("Signaling WakeUpScreen");
  g_dbus_connection_emit_signal (g_dbus_interface_skeleton_get_connection (skeleton),
                                 NULL,
                                 g_dbus_interface_skeleton_get_object_path (skeleton),
                                 SCREEN_SAVER_DBUS_NAME,
                                 "WakeUpScreen",
                                 NULL,
                                 NULL);
}

static void
on_lockscreen_manager_wakeup_outputs (PhoshScreenSaverManager *self,
                                      PhoshLockscreenManager *lockscreen_manager)
{
  g_return_if_fail (PHOSH_IS_SCREEN_SAVER_MANAGER (self));
  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (lockscreen_manager));

  phosh_screen_saver_manager_wakeup_screen (self);
}


static void
on_logind_lock (PhoshScreenSaverManager *self, PhoshDBusLoginSession *proxy)
{
  g_debug ("Locking request via logind1");
  phosh_lockscreen_manager_set_locked  (self->lockscreen_manager, TRUE);
}


static void
on_logind_unlock (PhoshScreenSaverManager *self, PhoshDBusLoginSession *proxy)
{
  g_debug ("Unlocking request via logind1");
  phosh_lockscreen_manager_set_locked  (self->lockscreen_manager, FALSE);
}

static void
on_logind_prepare_for_sleep (PhoshScreenSaverManager *self,
                             gboolean                 suspending,
                             PhoshDBusLoginManager   *proxy)
{
  g_return_if_fail (PHOSH_IS_SCREEN_SAVER_MANAGER (self));

  if (suspending) {
    phosh_lockscreen_manager_set_locked (self->lockscreen_manager, TRUE);
  } else {
    PhoshIdleManager *idle_manager;

    g_debug ("Got PrepareForSleep signal (resume case)");
    phosh_screen_saver_manager_wakeup_screen (self);

    idle_manager = phosh_idle_manager_get_default ();
    phosh_idle_manager_reset_timers (idle_manager);
  }
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
  PhoshScreenSaverManager *self = PHOSH_SCREEN_SAVER_MANAGER (user_data);

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                    connection,
                                    "/org/gnome/ScreenSaver",
                                    NULL);
}


static void
phosh_screen_saver_manager_dispose (GObject *object)
{
  PhoshScreenSaverManager *self = PHOSH_SCREEN_SAVER_MANAGER (object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  g_clear_handle_id (&self->idle_id, g_source_remove);
  g_clear_handle_id (&self->dbus_name_id, g_bus_unown_name);

  if (g_dbus_interface_skeleton_get_object_path (G_DBUS_INTERFACE_SKELETON (self)))
    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (self));

  g_clear_object (&self->lockscreen_manager);
  g_clear_object (&self->logind_session_proxy);
  g_clear_object (&self->logind_manager_proxy);

  G_OBJECT_CLASS (phosh_screen_saver_manager_parent_class)->dispose (object);
}


static void
on_logind_get_session_proxy_finish  (GObject                 *object,
                                     GAsyncResult            *res,
                                     PhoshScreenSaverManager *self)
{
  g_autoptr (GError) err = NULL;

  self->logind_session_proxy = phosh_dbus_login_session_proxy_new_for_bus_finish (
    res, &err);
  if (!self->logind_session_proxy) {
    phosh_dbus_service_error_warn (err, "Failed to get login1 session proxy");
    return;
  }

  g_return_if_fail (PHOSH_IS_SCREEN_SAVER_MANAGER (self));

  /* finally register signals */
  g_object_connect (
    self->logind_session_proxy,
    "swapped-object-signal::lock", G_CALLBACK (on_logind_lock), self,
    "swapped-object-signal::unlock", G_CALLBACK (on_logind_unlock), self,
    NULL);

  g_signal_connect_swapped (
    self->logind_manager_proxy,
    "prepare-for-sleep", G_CALLBACK (on_logind_prepare_for_sleep), self);
}


static void
on_logind_manager_get_session_finished (PhoshDBusLoginManager   *object,
                                        GAsyncResult            *res,
                                        PhoshScreenSaverManager *self)
{
  g_autofree char *object_path = NULL;
  g_autoptr (GError) err = NULL;

  if (!phosh_dbus_login_manager_call_get_session_finish (
        object, &object_path, res, &err)) {
    g_warning ("Failed to get session: %s", err->message);
    return;
  }

  g_return_if_fail (PHOSH_IS_SCREEN_SAVER_MANAGER (self));

  /* Register a proxy for this session */
  phosh_dbus_login_session_proxy_new_for_bus (
    G_BUS_TYPE_SYSTEM,
    G_DBUS_PROXY_FLAGS_NONE,
    LOGIN_BUS_NAME,
    object_path,
    self->cancel,
    (GAsyncReadyCallback)on_logind_get_session_proxy_finish,
    self);
}


static void
on_logind_manager_proxy_new_for_bus_finish (GObject                 *source_object,
                                            GAsyncResult            *res,
                                            PhoshScreenSaverManager *self)
{
  g_autoptr (GError) err = NULL;
  g_autofree char *session_id = NULL;

  self->logind_manager_proxy =
    phosh_dbus_login_manager_proxy_new_for_bus_finish (res, &err);

  if (!self->logind_manager_proxy) {
    phosh_dbus_service_error_warn (err, "Failed to get login1 manager proxy");
    return;
  }

  g_return_if_fail (PHOSH_IS_SCREEN_SAVER_MANAGER (self));

  /* If we find a session get it object path */
  if (phosh_find_systemd_session (&session_id)) {
    g_debug ("Logind session %s", session_id);

    phosh_dbus_login_manager_call_get_session (
      self->logind_manager_proxy,
      session_id,
      self->cancel,
      (GAsyncReadyCallback)on_logind_manager_get_session_finished,
      self);
  }

  g_debug ("Connected to logind's session interface");
}


static gboolean
on_idle (PhoshScreenSaverManager *self)
{
  /* Connect to logind's session manager */
  phosh_dbus_login_manager_proxy_new_for_bus (
    G_BUS_TYPE_SYSTEM,
    G_DBUS_PROXY_FLAGS_NONE,
    LOGIN_BUS_NAME,
    LOGIN_OBJECT_PATH,
    self->cancel,
    (GAsyncReadyCallback) on_logind_manager_proxy_new_for_bus_finish,
    self);

  self->idle_id = 0;
  return G_SOURCE_REMOVE;
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
                                       self,
                                       NULL);

  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self->lockscreen_manager));

  /* Perform login1 setup when idle */
  self->idle_id = g_idle_add ((GSourceFunc)on_idle, self);
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
  self->cancel = g_cancellable_new ();
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
