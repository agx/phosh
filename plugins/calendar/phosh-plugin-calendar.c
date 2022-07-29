/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-plugin-calendar"

#include <gio/gio.h>
#include <gtk/gtk.h>

#define PHOSH_EXTENSION_POINT_LOCKSCREEN_WIDGET "phosh-lockscreen-widget"

char **g_io_phosh_plugin_calendar_query (void);

void
g_io_module_load (GIOModule *module)
{
  g_type_module_use (G_TYPE_MODULE (module));

  g_type_ensure (GTK_TYPE_CALENDAR);

  g_io_extension_point_implement ("phosh-lockscreen-widget",
                                  GTK_TYPE_CALENDAR,
                                  "calendar",
                                  10);
}

void
g_io_module_unload (GIOModule *module)
{
}

char **
g_io_phosh_plugin_calendar_query (void)
{
  char *extension_points[] = {PHOSH_EXTENSION_POINT_LOCKSCREEN_WIDGET, NULL};

  return g_strdupv (extension_points);
}
