/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-default-media-player"

#include "phosh-config.h"

#include "default-media-player.h"
#include "mpris-manager.h"
#include "shell-priv.h"

/**
 * PhoshDefaultMediaPlayer:
 *
 * The media player widget tracking the default player
 *
 * The default player is the last recently one added (so likely the one the user
 * cares the most about).
 */

typedef struct _PhoshDefaultMediaPlayer {
  PhoshMediaPlayer          parent_instance;

  PhoshMprisManager        *manager;
} PhoshDefaultMediaPlayer;

G_DEFINE_FINAL_TYPE (PhoshDefaultMediaPlayer, phosh_default_media_player, PHOSH_TYPE_MEDIA_PLAYER);


static void
phosh_default_media_player_dispose (GObject *object)
{
  PhoshDefaultMediaPlayer *self = PHOSH_DEFAULT_MEDIA_PLAYER (object);

  g_clear_object (&self->manager);

  G_OBJECT_CLASS (phosh_default_media_player_parent_class)->dispose (object);
}


static void
phosh_default_media_player_class_init (PhoshDefaultMediaPlayerClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->dispose = phosh_default_media_player_dispose;
}


static void
on_player_changed (PhoshDefaultMediaPlayer *self, GParamSpec *pspec, PhoshMprisManager *manager)
{
  g_assert (PHOSH_IS_MEDIA_PLAYER (self));
  g_assert (PHOSH_IS_MPRIS_MANAGER (self->manager));

  phosh_media_player_set_player (PHOSH_MEDIA_PLAYER (self),
                                 phosh_mpris_manager_get_player (self->manager));
}


static void
phosh_default_media_player_init (PhoshDefaultMediaPlayer *self)
{
  PhoshMprisManager *manager = phosh_shell_get_mpris_manager (phosh_shell_get_default ());

  if (manager) {
    self->manager = g_object_ref (manager);

    g_signal_connect_object (self->manager,
                             "notify::player",
                             G_CALLBACK (on_player_changed),
                             self,
                             G_CONNECT_SWAPPED);
    on_player_changed (self, NULL, self->manager);
  }
}


GtkWidget *
phosh_default_media_player_new (void)
{
  return g_object_new (PHOSH_TYPE_MEDIA_PLAYER, NULL);
}
