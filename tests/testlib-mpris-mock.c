/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib-mpris-mock.h"

#define OBJECT_PATH "/org/mpris/MediaPlayer2"
#define BUS_NAME "org.mpris.MediaPlayer2.PhoshMock"

static void
on_bus_acquired (GDBusConnection *connection,
                 const char      *name,
                 gpointer         user_data)
{
  PhoshTestMprisMock *self = user_data;
  g_autoptr (GError) err = NULL;

  phosh_mpris_dbus_media_player2_player_set_can_go_previous (self->skel, TRUE);
  phosh_mpris_dbus_media_player2_player_set_can_go_next (self->skel, TRUE);
  phosh_mpris_dbus_media_player2_player_set_can_play (self->skel, TRUE);
  phosh_mpris_dbus_media_player2_player_set_playback_status (self->skel, "Playing");
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self->skel),
                                    connection,
                                    OBJECT_PATH,
                                    &err);
  g_assert_no_error (err);
}


PhoshTestMprisMock *
phosh_test_mpris_mock_new (void)
{
  PhoshTestMprisMock *self;

  self = g_new0 (PhoshTestMprisMock, 1);
  self->skel = phosh_mpris_dbus_media_player2_player_skeleton_new ();
  return self;
}


void
phosh_test_mpris_mock_dispose (PhoshTestMprisMock *self)
{
  g_clear_handle_id (&self->bus_id,  g_bus_unown_name);
  g_clear_object (&self->skel);
  g_free (self);
}


void
phosh_mpris_mock_export (PhoshTestMprisMock *self)
{
  self->bus_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                 BUS_NAME,
                                 G_BUS_NAME_OWNER_FLAGS_NONE,
                                 on_bus_acquired,
                                 NULL,
                                 NULL,
                                 self,
                                 NULL);
  g_assert_true (self->bus_id > 0);
}
