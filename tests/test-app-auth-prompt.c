/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib.h"

#include "app-auth-prompt.h"

typedef struct _Fixture {
  PhoshTestCompositorState *state;
} Fixture;


static void
compositor_setup (Fixture *fixture, gconstpointer unused)
{
  fixture->state = phosh_test_compositor_new (TRUE);
  g_assert_nonnull (fixture->state);
}

static void
compositor_teardown (Fixture *fixture, gconstpointer unused)
{
  phosh_test_compositor_free (fixture->state);
}

static void
test_app_auth_prompt_new (Fixture *fixture, gconstpointer unused)
{
  GtkWidget *prompt = g_object_new (PHOSH_TYPE_APP_AUTH_PROMPT,
                                    "monitor", phosh_test_get_monitor (),
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

  g_test_add ("/phosh/app_auth_prompt_new/new", Fixture, NULL,
              compositor_setup, test_app_auth_prompt_new, compositor_teardown);

  return g_test_run ();
}
