/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * PhoshMediaPlayerStatus:
 * @PHOSH_MEDIA_PLAYER_STATUS_STOPPED: The player is stopped.
 * @PHOSH_MEDIA_PLAYER_STATUS_PAUSED: The player is paused.
 * @PHOSH_MEDIA_PLAYER_STATUS_PLAYING: The player is playing.
 *
 * The status of the media player attached to the wigget
 */
typedef enum {
  PHOSH_MEDIA_PLAYER_STATUS_STOPPED,
  PHOSH_MEDIA_PLAYER_STATUS_PAUSED,
  PHOSH_MEDIA_PLAYER_STATUS_PLAYING,
} PhoshMediaPlayerStatus;

#define PHOSH_TYPE_MEDIA_PLAYER (phosh_media_player_get_type ())

G_DECLARE_FINAL_TYPE (PhoshMediaPlayer, phosh_media_player, PHOSH, MEDIA_PLAYER, GtkGrid)

GtkWidget              *phosh_media_player_new                   (void);
gboolean                phosh_media_player_get_is_playable       (PhoshMediaPlayer *self);
PhoshMediaPlayerStatus  phosh_media_player_get_status            (PhoshMediaPlayer *self);
void                    phosh_media_player_toggle_play_pause     (PhoshMediaPlayer *self);

G_END_DECLS
