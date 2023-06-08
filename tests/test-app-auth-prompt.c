/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib-compositor.h"

#include "app-auth-prompt.h"

static void
test_app_auth_prompt_new (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  GtkWidget *prompt = g_object_new (PHOSH_TYPE_APP_AUTH_PROMPT,
                                    "monitor", phosh_test_get_monitor (fixture->state),
                                    "title", "title",
                                    "subtitle", "subtitle",
                                    "body", "body",
                                    "grant-label", "ok",
                                    "deny-label", "cancel",
                                    NULL);

  g_assert_true (PHOSH_IS_APP_AUTH_PROMPT (prompt));

  gtk_widget_show (prompt);
  gtk_widget_destroy (prompt);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  PHOSH_COMPOSITOR_TEST_ADD ("/phosh/app_auth_prompt_new/new", test_app_auth_prompt_new);

  return g_test_run ();
}
