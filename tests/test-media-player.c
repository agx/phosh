/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "media-player.h"

static void
test_phosh_media_player_new (void)
{
  PhoshMediaPlayer *player;

  player = PHOSH_MEDIA_PLAYER (g_object_ref_sink (phosh_media_player_new ()));
  g_assert_true (PHOSH_IS_MEDIA_PLAYER (player));

  g_assert_false (phosh_media_player_get_is_playable (player));
  g_assert_cmpint (phosh_media_player_get_status (player), ==, PHOSH_MEDIA_PLAYER_STATUS_STOPPED);

  g_assert_finalize_object (player);
}

int
main (int argc, char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/media-player/new", test_phosh_media_player_new);

  return g_test_run ();
}
