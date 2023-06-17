/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "media-player.h"


typedef struct {
  GMainLoop *mainloop;
} TestFixture;

static void
fixture_setup (TestFixture *fixture, gconstpointer unused)
{
  g_assert_null (fixture->mainloop);
  fixture->mainloop = g_main_loop_new (NULL, FALSE);
}

static void
fixture_teardown (TestFixture *fixture, gconstpointer unused)
{
  g_clear_pointer (&fixture->mainloop, g_main_loop_unref);
}

static gboolean
on_timeout (GMainLoop *mainloop)
{
  g_main_loop_quit (mainloop);
  return G_SOURCE_REMOVE;
}

static void
test_phosh_media_player_new (TestFixture *fixture, gconstpointer data)
{
  GtkWidget *widget;

  widget = g_object_ref_sink (phosh_media_player_new ());
  g_assert_true (PHOSH_IS_MEDIA_PLAYER (widget));

  g_timeout_add_seconds (1, (GSourceFunc)on_timeout, fixture->mainloop);
  g_main_loop_run (fixture->mainloop);

  g_assert_finalize_object (widget);
}

int
main (int argc, char *argv[])
{
  int ret = -1;
  g_autoptr(GTestDBus) bus = NULL;
  const char *display;

  bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  /* g_test_dbus_up clears $DISPLAY, preserve that for gtk_init */
  display = g_getenv ("DISPLAY");
  g_test_dbus_up (bus);
  g_setenv ("DISPLAY", display, TRUE);

  gtk_test_init (&argc, &argv, NULL);

  g_test_add("/phosh/media-player/new",
             TestFixture,
             NULL,
             fixture_setup,
             test_phosh_media_player_new,
             fixture_teardown);

  ret = g_test_run ();
  g_test_dbus_down (bus);

  return ret;
}
