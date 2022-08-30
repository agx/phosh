/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-config.h"
#include "plugin-loader.h"

static void
test_plugin_loader_new (void)
{
  PhoshPluginLoader *plugin_loader;
  const char *dirs[] = { "/doesnotexist", NULL };
  const char *const *cdirs;
  g_autofree char *prop_extension_point = NULL;
  g_auto (GStrv) prop_dirs = NULL;

  plugin_loader = phosh_plugin_loader_new ((GStrv)dirs, PHOSH_EXTENSION_POINT_LOCKSCREEN_WIDGET);
  g_assert_true (PHOSH_IS_PLUGIN_LOADER (plugin_loader));

  g_assert_cmpstr (phosh_plugin_loader_get_extension_point (plugin_loader), ==,
                   PHOSH_EXTENSION_POINT_LOCKSCREEN_WIDGET);
  cdirs = phosh_plugin_loader_get_plugin_dirs (plugin_loader);
  g_assert_cmpstr (cdirs[0], ==, dirs[0]);
  g_assert_cmpstr (cdirs[1], ==, dirs[1]);

  g_object_get (plugin_loader,
                "extension-point", &prop_extension_point,
                "plugin-dirs", &prop_dirs,
                NULL);
  g_assert_cmpstr (prop_extension_point, ==, PHOSH_EXTENSION_POINT_LOCKSCREEN_WIDGET);
  g_assert_cmpstr (prop_dirs[0], ==, dirs[0]);
  g_assert_cmpstr (prop_dirs[1], ==, dirs[1]);

  g_assert_finalize_object (plugin_loader);
}


static void
test_plugin_loader_load (void)
{
  PhoshPluginLoader *plugin_loader;
#ifndef PHOSH_USES_ASAN
  GtkWidget *widget;
#endif
  const char *dirs[] = { TEST_BUILD_DIR "/plugins/calendar", NULL };

  plugin_loader = phosh_plugin_loader_new ((GStrv)dirs, PHOSH_EXTENSION_POINT_LOCKSCREEN_WIDGET);
  g_assert_true (PHOSH_IS_PLUGIN_LOADER (plugin_loader));

#ifndef PHOSH_USES_ASAN
  widget = phosh_plugin_loader_load_plugin (plugin_loader, "calendar");
  g_assert_true (GTK_IS_WIDGET (widget));
  g_object_ref_sink (widget);

  gtk_widget_destroy (widget);
#endif
  g_assert_finalize_object (plugin_loader);
}


int
main (int   argc,
      char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func("/phosh/plugin-loader/new", test_plugin_loader_new);
  g_test_add_func("/phosh/plugin-loader/load", test_plugin_loader_load);

  return g_test_run();
}
