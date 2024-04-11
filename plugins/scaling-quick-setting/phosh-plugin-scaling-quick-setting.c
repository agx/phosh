/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Adam Honse <calcprogrammer1@gmail.com>
 */

#include "scaling-quick-setting.h"
#include "phosh-plugin.h"

#include <gio/gio.h>
#include <gtk/gtk.h>

char **g_io_phosh_plugin_scaling_quick_setting_query (void);

void
g_io_module_load (GIOModule *module)
{
  g_type_module_use (G_TYPE_MODULE (module));

  g_io_extension_point_implement (PHOSH_PLUGIN_EXTENSION_POINT_QUICK_SETTING_WIDGET,
                                  PHOSH_TYPE_SCALING_QUICK_SETTING,
                                  PLUGIN_NAME,
                                  10);
}

void
g_io_module_unload (GIOModule *module)
{
}

char **
g_io_phosh_plugin_scaling_quick_setting_query (void)
{
  char *extension_points[] = {PHOSH_PLUGIN_EXTENSION_POINT_QUICK_SETTING_WIDGET, NULL};

  return g_strdupv (extension_points);
}
