/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "media-player.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_DEFAULT_MEDIA_PLAYER (phosh_default_media_player_get_type ())

G_DECLARE_FINAL_TYPE (PhoshDefaultMediaPlayer, phosh_default_media_player, PHOSH,
                      DEFAULT_MEDIA_PLAYER, PhoshMediaPlayer)

GtkWidget              *phosh_default_media_player_new           (void);

G_END_DECLS
