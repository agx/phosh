/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib-compositor.h"

#include "end-session-dialog.h"

#include <glib.h>


static void
test_end_session_dialog_new (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  GtkWidget *dialog;

  dialog = g_object_new (PHOSH_TYPE_END_SESSION_DIALOG,
                         "monitor", phosh_test_get_monitor (fixture->state),
                         "action", PHOSH_END_SESSION_ACTION_LOGOUT,
                         "timeout", 0,
                         "inhibitor-paths", NULL,
                         NULL);

  g_assert_true (PHOSH_IS_END_SESSION_DIALOG (dialog));

  gtk_widget_show (dialog);

  g_assert_true (gtk_widget_get_visible (dialog));
  g_assert_true (gtk_widget_get_mapped (dialog));
  gtk_widget_hide (dialog);
  g_assert_false (gtk_widget_get_visible (dialog));
  g_assert_false (gtk_widget_get_mapped (dialog));
  gtk_widget_destroy (dialog);
}


static gboolean
on_expired (GMainLoop *mainloop)
{
  /* Make sure we don't wait forever */
  g_assert_not_reached ();
}


static void
on_closed (GMainLoop *mainloop)
{
  g_main_loop_quit (mainloop);
}


static void
test_end_session_dialog_timeout (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  PhoshEndSessionDialog *dialog = NULL;
  g_autoptr (GMainLoop) mainloop = NULL;

  dialog = g_object_new (PHOSH_TYPE_END_SESSION_DIALOG,
                         "monitor", phosh_test_get_monitor (fixture->state),
                         "action", PHOSH_END_SESSION_ACTION_LOGOUT,
                         "timeout", 1,
                         "inhibitor-paths", NULL,
                         NULL);
  g_assert_true (PHOSH_IS_END_SESSION_DIALOG (dialog));

  mainloop = g_main_loop_new (NULL, FALSE);

  g_signal_connect_swapped (dialog, "closed", G_CALLBACK (on_closed), mainloop);
  g_timeout_add_seconds (3, (GSourceFunc)on_expired, mainloop);
  gtk_widget_show (GTK_WIDGET (dialog));

  g_main_loop_run (mainloop);
  /* Letting the dialog timeout means confirmation */
  g_assert_true (phosh_end_session_dialog_get_action_confirmed (dialog));

  gtk_widget_destroy (GTK_WIDGET (dialog));
}



int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  PHOSH_COMPOSITOR_TEST_ADD ("/phosh/end-session-dialog/new", test_end_session_dialog_new);
  PHOSH_COMPOSITOR_TEST_ADD ("/phosh/end-session-dialog/timeout", test_end_session_dialog_timeout);

  return g_test_run ();
}
