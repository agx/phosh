/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib.h"

#include "layersurface.h"
#include <gdk/gdkwayland.h>

#include <glib.h>


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
on_layer_surface_notify (PhoshLayerSurface *surface,
                         GParamSpec        *pspec,
                         guint             *count)
{
  (*count)++;
}

static void
test_layer_surface_new (Fixture *fixture, gconstpointer unused)
{
  GtkWidget *surface = phosh_layer_surface_new (phosh_wayland_get_zwlr_layer_shell_v1(fixture->state->wl),
                                                fixture->state->output);

  g_assert_true (PHOSH_IS_LAYER_SURFACE (surface));
  gtk_widget_destroy (surface);
}


static void
test_layer_surface_g_object_new (Fixture *fixture, gconstpointer unused)
{
  g_autofree char *namespace = g_strdup_printf ("phosh test %s", __func__);
  GtkWidget *surface = g_object_new (PHOSH_TYPE_LAYER_SURFACE,
                                     "layer-shell", phosh_wayland_get_zwlr_layer_shell_v1(fixture->state->wl),
                                     "wl-output", fixture->state->output,
                                     "width", 10,
                                     "height", 10,
                                     "layer", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
                                     "kbd-interactivity", FALSE,
                                     "exclusive-zone", -1,
                                     "namespace", namespace,
                                     NULL);

  g_assert_true (PHOSH_IS_LAYER_SURFACE (surface));
  gtk_widget_show (surface);
  /* Run the unmapped code path */
  g_assert_true (gtk_widget_get_visible (surface));
  g_assert_true (gtk_widget_get_mapped (surface));
  gtk_widget_hide (surface);
  g_assert_false (gtk_widget_get_visible (surface));
  g_assert_false (gtk_widget_get_mapped (surface));
  gtk_widget_destroy (surface);
}


static void
on_layer_surface_notify_width (PhoshLayerSurface *surface,
                               GParamSpec        *pspec,
                               guint             *count)
{
  (*count)++;
}


static void
test_layer_surface_set_size (Fixture *fixture, gconstpointer unused)
{
  guint width_count = 0, height_count = 0;
  g_autofree char *namespace = g_strdup_printf ("phosh test %s", __func__);

  GtkWidget *surface = g_object_new (PHOSH_TYPE_LAYER_SURFACE,
                                     "layer-shell", phosh_wayland_get_zwlr_layer_shell_v1(fixture->state->wl),
                                     "wl-output", fixture->state->output,
                                     "layer", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
                                     "kbd-interactivity", FALSE,
                                     "exclusive-zone", -1,
                                     "namespace", namespace,
                                     NULL);

  g_assert_true (PHOSH_IS_LAYER_SURFACE (surface));

  g_signal_connect (surface,
                    "notify::height",
                    G_CALLBACK (on_layer_surface_notify),
                    &height_count);

  g_signal_connect (surface,
                    "notify::width",
                    G_CALLBACK (on_layer_surface_notify_width),
                    &width_count);

  phosh_layer_surface_set_size (PHOSH_LAYER_SURFACE (surface), 10, 10);
  g_assert_cmpint (width_count, ==, 1);
  g_assert_cmpint (height_count, ==, 1);
  phosh_layer_surface_set_size (PHOSH_LAYER_SURFACE (surface), 10, 11);
  g_assert_cmpint (width_count, ==, 1);
  g_assert_cmpint (height_count, ==, 2);
  phosh_layer_surface_set_size (PHOSH_LAYER_SURFACE (surface), 11, -1);
  g_assert_cmpint (width_count, ==, 2);
  g_assert_cmpint (height_count, ==, 2);
  g_object_set (surface, "width", 12, NULL);
  g_assert_cmpint (width_count, ==, 3);
  g_assert_cmpint (height_count, ==, 2);
  g_object_set (surface, "height", 12, NULL);
  g_assert_cmpint (width_count, ==, 3);
  g_assert_cmpint (height_count, ==, 3);
  gtk_widget_show (surface);
  gtk_widget_hide (surface);
  gtk_widget_destroy (surface);
}


static void
test_layer_surface_set_kbd_interactivity (Fixture *fixture, gconstpointer unused)
{
  guint count = 0;
  gboolean kbd_interacivity;
  g_autofree char *namespace = g_strdup_printf ("phosh test %s", __func__);

  GtkWidget *surface = g_object_new (PHOSH_TYPE_LAYER_SURFACE,
                                     "layer-shell", phosh_wayland_get_zwlr_layer_shell_v1(fixture->state->wl),
                                     "wl-output", fixture->state->output,
                                     "layer", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
                                     "kbd-interactivity", FALSE,
                                     "exclusive-zone", -1,
                                     "namespace", namespace,
                                     NULL);

  g_assert_true (PHOSH_IS_LAYER_SURFACE (surface));

  g_signal_connect (surface,
                    "notify::kbd-interactivity",
                    G_CALLBACK (on_layer_surface_notify),
                    &count);

  g_object_get (surface, "kbd-interactivity", &kbd_interacivity, NULL);
  g_assert_false (kbd_interacivity);
  phosh_layer_surface_set_kbd_interactivity (PHOSH_LAYER_SURFACE (surface), TRUE);
  g_object_get (surface, "kbd-interactivity", &kbd_interacivity, NULL);
  g_assert_true (kbd_interacivity);
  g_assert_cmpint (count, ==, 1);
  phosh_layer_surface_set_kbd_interactivity (PHOSH_LAYER_SURFACE (surface), TRUE);
  g_object_get (surface, "kbd-interactivity", &kbd_interacivity, NULL);
  g_assert_true (kbd_interacivity);
  g_assert_cmpint (count, ==, 1);
  phosh_layer_surface_set_kbd_interactivity (PHOSH_LAYER_SURFACE (surface), FALSE);
  g_object_get (surface, "kbd-interactivity", &kbd_interacivity, NULL);
  g_assert_false (kbd_interacivity);
  g_assert_cmpint (count, ==, 2);
}


int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/phosh/layer-surface/new", Fixture, NULL,
              compositor_setup, test_layer_surface_new, compositor_teardown);
  g_test_add ("/phosh/layer-surface/g_object_new", Fixture, NULL,
              compositor_setup, test_layer_surface_g_object_new, compositor_teardown);
  g_test_add ("/phosh/layer-surface/set_size", Fixture, NULL,
              compositor_setup, test_layer_surface_set_size, compositor_teardown);
  g_test_add ("/phosh/layer-surface/set_kbd_interactivity", Fixture, NULL,
              compositor_setup, test_layer_surface_set_kbd_interactivity, compositor_teardown);

  return g_test_run ();
}
