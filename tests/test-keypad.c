/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * based on LGPL-2.1+ HdyKeypad which is
 * Copyright (C) 2019 Purism SPC
 */

#include "keypad.h"

gint notified;


static void
notify_cb (GtkWidget *widget,
           gpointer   data)
{
  notified++;
}


static void
test_keypad_entry (void)
{
  g_autoptr (PhoshKeypad) keypad = NULL;
  g_autoptr (GtkEntry) entry = NULL;

  keypad = g_object_ref_sink (PHOSH_KEYPAD (phosh_keypad_new ()));
  entry = g_object_ref_sink (GTK_ENTRY (gtk_entry_new ()));

  notified = 0;
  g_signal_connect (keypad, "notify::entry", G_CALLBACK (notify_cb), NULL);

  g_assert_null (phosh_keypad_get_entry (keypad));

  phosh_keypad_set_entry (keypad, entry);
  g_assert_cmpint (notified, ==, 1);

  g_assert_true (phosh_keypad_get_entry (keypad) == entry);

  g_object_set (keypad, "entry", NULL, NULL);
  g_assert_cmpint (notified, ==, 2);

  g_assert_null (phosh_keypad_get_entry (keypad));
}


static void
test_keypad_start_action (void)
{
  g_autoptr (PhoshKeypad) keypad = NULL;
  g_autoptr (GtkWidget) button = NULL;

  keypad = g_object_ref_sink (PHOSH_KEYPAD (phosh_keypad_new ()));
  button = g_object_ref_sink (gtk_button_new ());

  notified = 0;
  g_signal_connect (keypad, "notify::start-action", G_CALLBACK (notify_cb), NULL);

  g_assert_null (phosh_keypad_get_start_action (keypad));

  phosh_keypad_set_start_action (keypad, button);
  g_assert_cmpint (notified, ==, 1);

  g_assert_true (phosh_keypad_get_start_action (keypad) == button);

  g_object_set (keypad, "start-action", NULL, NULL);
  g_assert_cmpint (notified, ==, 2);

  g_assert_null (phosh_keypad_get_start_action (keypad));
}


static void
test_keypad_end_action (void)
{
  g_autoptr (PhoshKeypad) keypad = NULL;
  g_autoptr (GtkWidget) button = NULL;

  keypad = g_object_ref_sink (PHOSH_KEYPAD (phosh_keypad_new ()));
  button = g_object_ref_sink (gtk_button_new ());

  notified = 0;
  g_signal_connect (keypad, "notify::end-action", G_CALLBACK (notify_cb), NULL);

  g_assert_null (phosh_keypad_get_end_action (keypad));

  phosh_keypad_set_end_action (keypad, button);
  g_assert_cmpint (notified, ==, 1);

  g_assert_true (phosh_keypad_get_end_action (keypad) == button);

  g_object_set (keypad, "end-action", NULL, NULL);
  g_assert_cmpint (notified, ==, 2);

  g_assert_null (phosh_keypad_get_end_action (keypad));
}


static void
test_keypad_shuffle (void)
{
  g_autoptr (PhoshKeypad) keypad = NULL;

  keypad = g_object_ref_sink (g_object_new (PHOSH_TYPE_KEYPAD,
                                            "shuffle", TRUE,
                                            NULL));
  g_assert_true (phosh_keypad_get_shuffle (keypad));
}


gint
main (gint argc, char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/Keypad/entry", test_keypad_entry);
  g_test_add_func ("/phosh/Keypad/start_action", test_keypad_start_action);
  g_test_add_func ("/phosh/Keypad/end_action", test_keypad_end_action);
  g_test_add_func ("/phosh/Keypad/shuffle", test_keypad_shuffle);

  return g_test_run ();
}
