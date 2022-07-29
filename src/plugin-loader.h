/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Extension point names */
#define PHOSH_EXTENSION_POINT_LOCKSCREEN_WIDGET "phosh-lockscreen-widget"

#define PHOSH_TYPE_PLUGIN_LOADER phosh_plugin_loader_get_type()

G_DECLARE_FINAL_TYPE (PhoshPluginLoader, phosh_plugin_loader, PHOSH, PLUGIN_LOADER, GObject)

PhoshPluginLoader *phosh_plugin_loader_new (GStrv plugin_dirs, const char *extension_point);
GtkWidget         *phosh_plugin_loader_load_plugin (PhoshPluginLoader *self, const char *name);

G_END_DECLS