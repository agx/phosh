/*
 * Copyright (C) 2022 Chris Talbot
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "emergency-info-prefs.h"
#include "phosh-plugin.h"

char **g_io_phosh_plugin_prefs_emergency_info_query (void);

void
g_io_module_load (GIOModule *module)
{
  g_type_module_use (G_TYPE_MODULE (module));

  g_io_extension_point_implement (PHOSH_PLUGIN_EXTENSION_POINT_LOCKSCREEN_WIDGET_PREFS,
                                  PHOSH_TYPE_EMERGENCY_INFO_PREFS,
                                  PLUGIN_PREFS_NAME,
                                  10);
}

void
g_io_module_unload (GIOModule *module)
{
}


char **
g_io_phosh_plugin_prefs_emergency_info_query (void)
{
  char *extension_points[] = {PHOSH_PLUGIN_EXTENSION_POINT_LOCKSCREEN_WIDGET_PREFS, NULL};

  return g_strdupv (extension_points);
}
