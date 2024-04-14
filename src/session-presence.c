/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-session-presence"

#include "phosh-config.h"
#include "session-presence.h"

#define GNOME_SESSION_DBUS_NAME      "org.gnome.SessionManager"
#define GNOME_SESSION_DBUS_OBJECT    "/org/gnome/SessionManager/Presence"

/**
 * PhoshSessionPresence:
 *
 * Interface with gnome-session's Presence interface
 *
 * The #PhoshSessionPresence is responsible for getting status updated
 * from gnome-session's org.gnome.SessionManager.Presence interface.
 *
 * This is just a minimal wrapper so we don't have to provide the
 * object path, names and bus names in several places.
 */


typedef struct _PhoshSessionPresence
{
  PhoshSessionPresenceDBusPresenceProxy parent;
} PhoshSessionPresence;


G_DEFINE_TYPE (PhoshSessionPresence, phosh_session_presence, PHOSH_SESSION_PRESENCE_DBUS_TYPE_PRESENCE_PROXY)


static void
phosh_session_presence_class_init (PhoshSessionPresenceClass *klass)
{
}


static void
phosh_session_presence_init (PhoshSessionPresence *self)
{
}

/**
 * phosh_session_presence_get_default_failable:
 *
 * Get the session presence singleton
 *
 * Returns:(transfer none): The session presence singleton
 */
PhoshSessionPresence *
phosh_session_presence_get_default_failable (void)
{
  static PhoshSessionPresence *instance;
  GError *err = NULL;
  GInitable *ret;

  if (instance == NULL) {
    ret = g_initable_new (PHOSH_TYPE_SESSION_PRESENCE, NULL, &err,
                          "g-flags", G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                          "g-name", GNOME_SESSION_DBUS_NAME,
                          "g-bus-type", G_BUS_TYPE_SESSION,
                          "g-object-path", GNOME_SESSION_DBUS_OBJECT,
                          "g-interface-name", "org.gnome.SessionManager.Presence",
                          NULL);
    if (ret != NULL) {
      instance = PHOSH_SESSION_PRESENCE (ret);
    } else {
      g_warning ("Can't connect to session at %s: %s", GNOME_SESSION_DBUS_OBJECT, err->message);
      return NULL;
    }
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
  }
  return instance;
}
