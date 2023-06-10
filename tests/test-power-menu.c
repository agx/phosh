/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "power-menu.h"

#include "testlib-compositor.h"

static void
test_phosh_power_menu_new (PhoshTestCompositorFixture *fixture, gconstpointer data)
{
  PhoshPowerMenu *menu;

  menu = phosh_power_menu_new (phosh_test_get_monitor (fixture->state));
  gtk_widget_destroy (GTK_WIDGET (menu));
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  PHOSH_COMPOSITOR_TEST_ADD ("/phosh/power-menu/new", test_phosh_power_menu_new);

  return g_test_run ();
}
