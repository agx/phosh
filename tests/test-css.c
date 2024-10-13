/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "style-manager.h"

/* Load the stylesheets to catch CSS parser warnings */

static const char *
load_theme (const char *theme_name)
{
  const char *style;
  g_autoptr (GtkCssProvider) provider = gtk_css_provider_new ();

  g_debug ("GTK theme: %s", theme_name);

  style = phosh_style_manager_get_stylesheet (theme_name);
  gtk_css_provider_load_from_resource (provider, style);
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  return style;
}


static void
test_phosh_css_default(void)
{
  g_assert_cmpstr (load_theme ("Adwaita"), ==, "/sm/puri/phosh/stylesheet/adwaita-dark.css");
}


static void
test_phosh_css_highcontrast(void)
{
  g_assert_cmpstr (load_theme ("HighContrast"), ==, "/sm/puri/phosh/stylesheet/adwaita-hc-light.css");
}


int
main (int   argc,
      char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func("/phosh/css/default", test_phosh_css_default);
  g_test_add_func("/phosh/css/highcontrast", test_phosh_css_highcontrast);
  return g_test_run();
}
