/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib.h"

#include "background.h"

#include <gdesktop-enums.h>

#define BG_SCHEMA "org.gnome.desktop.background"

typedef struct _Fixture {
  PhoshTestCompositorState *state;
  GSettings                *settings;
} Fixture;


#define BG_KEY_PRIMARY_COLOR      "primary-color"
#define BG_KEY_SECONDARY_COLOR    "secondary-color"
#define BG_KEY_COLOR_TYPE         "color-shading-type"
#define BG_KEY_PICTURE_OPTIONS    "picture-options"
#define BG_KEY_PICTURE_OPACITY    "picture-opacity"
#define BG_KEY_PICTURE_URI        "picture-uri"


static void
compositor_setup (Fixture *fixture, gconstpointer unused)
{
  fixture->state = phosh_test_compositor_new ();
  g_assert_nonnull (fixture->state);

  fixture->settings = g_settings_new (BG_SCHEMA);
  g_settings_set_enum (fixture->settings, BG_KEY_PICTURE_OPTIONS,
                       G_DESKTOP_BACKGROUND_STYLE_NONE);
}

static void
compositor_teardown (Fixture *fixture, gconstpointer unused)
{
  g_clear_object (&fixture->settings);
  phosh_test_compositor_free (fixture->state);
}

static void
test_background_new (Fixture *fixture, gconstpointer unused)
{
  gboolean primary;
  GtkWidget *background;

  background = phosh_background_new (phosh_wayland_get_zwlr_layer_shell_v1(fixture->state->wl),
                                     fixture->state->output,
                                     1,
                                     TRUE);
  g_assert_true (PHOSH_IS_BACKGROUND (background));
  g_object_get (background, "primary", &primary, NULL);
  g_assert_true (primary);

  phosh_background_set_primary (PHOSH_BACKGROUND (background), FALSE);
  g_object_get (background, "primary", &primary, NULL);
  g_assert_false (primary);

  gtk_widget_show (background);
  gtk_widget_destroy (background);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/phosh/background/new", Fixture, NULL,
              compositor_setup, test_background_new, compositor_teardown);

  return g_test_run ();
}
