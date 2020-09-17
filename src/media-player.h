/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_MEDIA_PLAYER (phosh_media_player_get_type ())

G_DECLARE_FINAL_TYPE (PhoshMediaPlayer, phosh_media_player, PHOSH, MEDIA_PLAYER, GtkGrid)

GtkWidget * phosh_media_player_new (void);

G_END_DECLS
