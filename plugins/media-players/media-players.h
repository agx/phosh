/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */


#include <gtk/gtk.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_MEDIA_PLAYERS (phosh_media_players_get_type ())
G_DECLARE_FINAL_TYPE (PhoshMediaPlayers, phosh_media_players, PHOSH, MEDIA_PLAYERS, GtkBox)

G_END_DECLS
