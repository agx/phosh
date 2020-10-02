/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-session-manager"

#include "config.h"
#include "session-manager.h"

#include "dbus/gnome-session-dbus.h"

#define BUS_NAME "org.gnome.SessionManager"
#define OBJECT_PATH "/org/gnome/SessionManager"


/**
 * SECTION:session-manager
 * @short_description: Session interaction
 * @Title: PhoshSessionManager
 *
 * The #PhoshSessionManager is responsible for
 * managing attributes of the session.
 */

enum {
  PHOSH_SESSION_MANAGER_PROP_0,
  PHOSH_SESSION_MANAGER_PROP_ACTIVE,
  PHOSH_SESSION_MANAGER_PROP_LAST_PROP,
};
static GParamSpec *props[PHOSH_SESSION_MANAGER_PROP_LAST_PROP];


typedef struct _PhoshSessionManager {
  GObject                         parent;
  gboolean                        active;

  PhoshSessionDBusSessionManager *proxy;
} PhoshSessionManager;


G_DEFINE_TYPE (PhoshSessionManager, phosh_session_manager, G_TYPE_OBJECT)


static void
phosh_session_manager_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  PhoshSessionManager *self = PHOSH_SESSION_MANAGER (object);

  switch (property_id) {
  case PHOSH_SESSION_MANAGER_PROP_ACTIVE:
    g_value_set_boolean (value, self->active);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
on_session_active_changed (PhoshSessionManager            *self,
                           GParamSpec                     *pspec,
                           PhoshSessionDBusSessionManager *proxy)
{
  gboolean active;

  g_return_if_fail (PHOSH_IS_SESSION_MANAGER (self));
  g_return_if_fail (PHOSH_SESSION_DBUS_IS_SESSION_MANAGER_PROXY (proxy));

  active = phosh_session_dbus_session_manager_get_session_is_active (proxy);

  if (self->active == active)
    return;

  self->active = active;
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_SESSION_MANAGER_PROP_ACTIVE]);
  g_debug ("Session is now %sactive", self->active ? "" : "in");
}


static void
phosh_session_manager_constructed (GObject *object)
{
  PhoshSessionManager *self = PHOSH_SESSION_MANAGER (object);

  g_autoptr (GError) err = NULL;

  /* Sync call since this happens early in startup and we need it right away */
  self->proxy = phosh_session_dbus_session_manager_proxy_new_for_bus_sync (
    G_BUS_TYPE_SESSION,
    G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION,
    BUS_NAME,
    OBJECT_PATH,
    NULL,
    &err);

  if (!self->proxy) {
    g_warning ("Failed to get session proxy %s", err->message);
  } else {
    /* Don't use a property binding so 'active' can be a r/o property */
    g_signal_connect_swapped (self->proxy,
                              "notify::session-is-active",
                              G_CALLBACK (on_session_active_changed),
                              self);
    on_session_active_changed (self, NULL, self->proxy);
  }

  G_OBJECT_CLASS (phosh_session_manager_parent_class)->constructed (object);
}


static void
phosh_session_manager_dispose (GObject *object)
{
  PhoshSessionManager *self = PHOSH_SESSION_MANAGER (object);

  g_clear_object (&self->proxy);

  G_OBJECT_CLASS (phosh_session_manager_parent_class)->dispose (object);
}


static void
phosh_session_manager_class_init (PhoshSessionManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_session_manager_constructed;
  object_class->dispose = phosh_session_manager_dispose;
  object_class->get_property = phosh_session_manager_get_property;

  /**
   * PhoshSessionManager:active:
   *
   * Whether this phosh instance runs in the currently active session.
   */
  props[PHOSH_SESSION_MANAGER_PROP_ACTIVE] =
    g_param_spec_boolean ("active",
                          "Active",
                          "Active session",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PHOSH_SESSION_MANAGER_PROP_LAST_PROP, props);
}


static void
phosh_session_manager_init (PhoshSessionManager *self)
{
}


PhoshSessionManager *
phosh_session_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_SESSION_MANAGER, NULL);
}


gboolean
phosh_session_manager_is_active (PhoshSessionManager *self)
{
  g_return_val_if_fail (PHOSH_IS_SESSION_MANAGER (self), FALSE);

  return self->active;
}
