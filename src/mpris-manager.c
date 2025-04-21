/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-mpris-manager"

#include "phosh-config.h"

#include "mpris-dbus.h"
#include "mpris-manager.h"
#include "util.h"

#include <gmobile.h>

#define MPRIS_OBJECT_PATH "/org/mpris/MediaPlayer2"
#define MPRIS_PREFIX "org.mpris.MediaPlayer2."

/**
 * PhoshMprisManager:
 *
 * The #PhoshMprisManger interfaces with
 * [org.mpris.MediaPlayer2](https://specifications.freedesktop.org/mpris-spec/latest/)
 * based players allowing widgets to get an actual media player object.
 */

enum {
  PROP_0,
  PROP_CAN_RAISE,
  PROP_PLAYER,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshMprisManager {
  GObject                           parent;

  GCancellable                     *cancel;
  /* Base interface to raise player */
  PhoshMprisDBusMediaPlayer2       *mpris;
  /* Actual player controls */
  PhoshMprisDBusMediaPlayer2Player *player;
  GDBusConnection                  *session_bus;

  guint                             dbus_id;

  gboolean                          can_raise;
};
G_DEFINE_TYPE (PhoshMprisManager, phosh_mpris_manager, G_TYPE_OBJECT)


static void
phosh_mpris_manager_set_player (PhoshMprisManager *self, PhoshMprisDBusMediaPlayer2Player *player)
{
  if (self->player == player)
    return;

  g_set_object (&self->player, player);
  g_debug ("New player: %p", self->player);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PLAYER]);
}


static void
phosh_mpris_manager_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  PhoshMprisManager *self = PHOSH_MPRIS_MANAGER (object);

  switch (property_id) {
  case PROP_CAN_RAISE:
    g_value_set_boolean (value, self->can_raise);
    break;
  case PROP_PLAYER:
    g_value_set_object (value, self->player);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_mpris_manager_dispose (GObject *object)
{
  PhoshMprisManager *self = PHOSH_MPRIS_MANAGER (object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  if (self->dbus_id) {
    g_dbus_connection_signal_unsubscribe (self->session_bus, self->dbus_id);
    self->dbus_id = 0;
  }

  g_clear_object (&self->session_bus);
  g_clear_object (&self->mpris);
  g_clear_object (&self->player);

  G_OBJECT_CLASS (phosh_mpris_manager_parent_class)->dispose (object);
}


static void
phosh_mpris_manager_class_init (PhoshMprisManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phosh_mpris_manager_get_property;
  object_class->dispose = phosh_mpris_manager_dispose;

  /**
   * PhoshMprisManager:can-raise
   *
   * Whether the player app can be raised
   */
  props[PROP_CAN_RAISE] =
    g_param_spec_boolean ("can-raise", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * PhoshMprisManager:player
   *
   * The current player.
   */
  props[PROP_PLAYER] =
    g_param_spec_object ("player", "", "",
                         PHOSH_MPRIS_DBUS_TYPE_MEDIA_PLAYER2_PLAYER_PROXY,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
on_attach_player_ready (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  PhoshMprisManager *self = PHOSH_MPRIS_MANAGER (user_data);
  g_autoptr (PhoshMprisDBusMediaPlayer2Player) player = NULL;
  g_autoptr (GError) err = NULL;

  player = phosh_mpris_dbus_media_player2_player_proxy_new_for_bus_finish (res, &err);
  if (player == NULL) {
    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      return;

    phosh_async_error_warn (err, "Failed to get player");
    phosh_mpris_manager_set_player (self, NULL);
    return;
  }

  g_return_if_fail (PHOSH_IS_MPRIS_MANAGER (self));

  phosh_mpris_manager_set_player (self, player);
}


static void
on_attach_mpris_ready (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  PhoshMprisManager *self = PHOSH_MPRIS_MANAGER (user_data);
  PhoshMprisDBusMediaPlayer2 *mpris;
  g_autoptr (GError) err = NULL;
  gboolean can_raise;

  mpris = phosh_mpris_dbus_media_player2_proxy_new_for_bus_finish (res, &err);
  /* Missing mpris interface is not fatal */
  if (mpris == NULL) {
    phosh_async_error_warn (err, "Failed to get player");
    return;
  }

  g_return_if_fail (PHOSH_IS_MPRIS_MANAGER (self));
  self->mpris = g_steal_pointer (&mpris);

  can_raise = phosh_mpris_dbus_media_player2_get_can_raise (self->mpris);
  if (self->can_raise == can_raise)
    return;

  self->can_raise = can_raise;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CAN_RAISE]);
}


static void
attach_player (PhoshMprisManager *self, const char *name)
{
  g_clear_object (&self->mpris);
  phosh_mpris_manager_set_player (self, NULL);

  g_debug ("Trying to attach player for %s", name);

  /* The player interface with the controls */
  phosh_mpris_dbus_media_player2_player_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                                           G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                                           name,
                                                           MPRIS_OBJECT_PATH,
                                                           self->cancel,
                                                           on_attach_player_ready,
                                                           self);

  /* The player base interface to e.g. raise the player */
  phosh_mpris_dbus_media_player2_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                                    G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                                    name,
                                                    MPRIS_OBJECT_PATH,
                                                    self->cancel,
                                                    on_attach_mpris_ready,
                                                    self);
}


static gboolean
is_valid_player (const char *bus_name)
{
  if (!g_str_has_prefix (bus_name, MPRIS_PREFIX))
    return FALSE;

  if (strlen (bus_name) < G_N_ELEMENTS (MPRIS_PREFIX))
    return FALSE;

  return TRUE;
}


static void
find_player_done (GObject *source_object, GAsyncResult *res, PhoshMprisManager *self)
{
  g_autoptr (GVariant) result = NULL;
  g_autoptr (GVariant) names = NULL;
  g_autoptr (GError) err = NULL;
  const char *name;
  GVariantIter iter;
  gboolean found = FALSE;

  result = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source_object), res, &err);
  if (!result) {
    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      return;

    phosh_async_error_warn (err, "Failed to list bus names to find mpris player");

    phosh_mpris_manager_set_player (self, NULL);
    return;
  }
  g_return_if_fail (PHOSH_IS_MPRIS_MANAGER (self));
  g_return_if_fail (G_IS_DBUS_CONNECTION (self->session_bus));

  names = g_variant_get_child_value (result, 0);
  g_variant_iter_init (&iter, names);
  while (g_variant_iter_loop (&iter, "&s", &name)) {

    if (!is_valid_player (name))
      continue;

    g_debug ("Found player: %s", name);
    attach_player (self, name);
    found = TRUE;
    break;
  }

  if (!found) {
    g_debug ("No player found");
    phosh_mpris_manager_set_player (self, NULL);
  }
}


static void
find_player (PhoshMprisManager *self)
{
  g_return_if_fail (G_IS_DBUS_CONNECTION (self->session_bus));

  g_dbus_connection_call (self->session_bus,
                          "org.freedesktop.DBus",
                          "/org/freedesktop/DBus",
                          "org.freedesktop.DBus",
                          "ListNames",
                          NULL,
                          G_VARIANT_TYPE ("(as)"),
                          G_DBUS_CALL_FLAGS_NO_AUTO_START,
                          1000,
                          self->cancel,
                          (GAsyncReadyCallback)find_player_done,
                          self);
}


static void
on_dbus_name_owner_changed (GDBusConnection *connection,
                            const char      *sender_name,
                            const char      *object_path,
                            const char      *interface_name,
                            const char      *signal_name,
                            GVariant        *parameters,
                            gpointer         user_data)
{
  PhoshMprisManager *self = PHOSH_MPRIS_MANAGER (user_data);
  g_autofree char *name, *from, *to;

  g_return_if_fail (PHOSH_IS_MPRIS_MANAGER (self));

  g_variant_get (parameters, "(sss)", &name, &from, &to);
  g_debug ("mpris player name owner change: '%s' '%s' '%s'", name, from, to);

  if (!is_valid_player (name))
    return;

  /* Current player vanished, look for another one, already running */
  if (gm_str_is_null_or_empty (to)) {
    phosh_mpris_manager_set_player (self, NULL);
    find_player (self);
    return;
  }

  /* New player showed up, pick up */
  attach_player (self, name);
}


static void
on_bus_get_finished (GObject *source_object, GAsyncResult *res, PhoshMprisManager *self)
{
  g_autoptr (GError) err = NULL;
  GDBusConnection *session_bus;

  session_bus = g_bus_get_finish (res, &err);
  if (session_bus == NULL) {
    phosh_async_error_warn (err, "Failed to attach to session bus");
    return;
  }

  g_return_if_fail (PHOSH_IS_MPRIS_MANAGER (self));
  self->session_bus = session_bus;
  /* Listen for name owner changes to detect new mpris players */
  /* We don't need to hold a ref since the callback won't be invoked after unsubscribe
   * from the same thread */
  self->dbus_id = g_dbus_connection_signal_subscribe (self->session_bus,
                                                      "org.freedesktop.DBus",
                                                      "org.freedesktop.DBus",
                                                      "NameOwnerChanged",
                                                      "/org/freedesktop/DBus",
                                                      NULL,
                                                      G_DBUS_SIGNAL_FLAGS_NONE,
                                                      on_dbus_name_owner_changed,
                                                      self, NULL);
  /* Find player initially */
  find_player (self);
}


static void
phosh_mpris_manager_init (PhoshMprisManager *self)
{
  self->cancel = g_cancellable_new ();

  g_bus_get (G_BUS_TYPE_SESSION,
             self->cancel,
             (GAsyncReadyCallback)on_bus_get_finished,
             self);
}


PhoshMprisManager *
phosh_mpris_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_MPRIS_MANAGER, NULL);
}


PhoshMprisDBusMediaPlayer2Player *
phosh_mpris_manager_get_player (PhoshMprisManager *self)
{
  g_return_val_if_fail (PHOSH_IS_MPRIS_MANAGER (self), NULL);

  return self->player;
}


gboolean
phosh_mpris_manager_get_can_raise (PhoshMprisManager *self)
{
  g_return_val_if_fail (PHOSH_IS_MPRIS_MANAGER (self), FALSE);

  return self->can_raise;
}


gboolean
phosh_mpris_manager_raise_finish (PhoshMprisManager *self, GAsyncResult *res, GError **err)
{
  g_assert (G_IS_TASK (res));
  g_assert (!err || !*err);
  g_assert (g_async_result_is_tagged (res, phosh_mpris_manager_raise_async));

  g_return_val_if_fail (PHOSH_IS_MPRIS_MANAGER (self), FALSE);

  return g_task_propagate_boolean (G_TASK (res), err);
}


static void
on_raise_done (GObject *object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GTask) task = G_TASK (user_data);
  PhoshMprisDBusMediaPlayer2 *mpris = PHOSH_MPRIS_DBUS_MEDIA_PLAYER2 (object);
  GError *err = NULL;

  if (!phosh_mpris_dbus_media_player2_call_raise_finish (mpris, res, &err)) {
    g_task_return_error (task, err);
    return;
  }

  g_task_return_boolean (task, TRUE);
}


void
phosh_mpris_manager_raise_async (PhoshMprisManager  *self,
                                 GCancellable       *cancel,
                                 GAsyncReadyCallback callback,
                                 gpointer            user_data)
{
  g_autoptr (GTask) task = NULL;

  g_return_if_fail (PHOSH_IS_MPRIS_MANAGER (self));
  g_return_if_fail (cancel == NULL || G_IS_CANCELLABLE (cancel));

  task = g_task_new (self, cancel, callback, user_data);
  g_task_set_source_tag (task, phosh_mpris_manager_raise_async);

  phosh_mpris_dbus_media_player2_call_raise (self->mpris,
                                             self->cancel,
                                             on_raise_done,
                                             g_steal_pointer (&task));
}
