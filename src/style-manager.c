/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-style-manager"

#include "phosh-config.h"

#include "style-manager.h"
#include "util.h"

#include <gtk/gtk.h>

/**
 * PhoshStyleManager:
 *
 * The style manager is responsible for picking style sheets and
 * themes and notifying other parts of the shell about changes.
 */


struct _PhoshStyleManager {
  GObject         parent;

  char           *theme_name;
  GtkCssProvider *css_provider;
};
G_DEFINE_TYPE (PhoshStyleManager, phosh_style_manager, G_TYPE_OBJECT)


static void
on_gtk_theme_name_changed (PhoshStyleManager *self, GParamSpec *pspec, GtkSettings *settings)
{
  const char *style;
  g_autofree char *name = NULL;
  g_autoptr (GtkCssProvider) provider = gtk_css_provider_new ();

  g_object_get (settings, "gtk-theme-name", &name, NULL);

  if (g_strcmp0 (self->theme_name, name) == 0)
    return;

  self->theme_name = g_steal_pointer (&name);
  g_debug ("GTK theme: %s", self->theme_name);

  if (self->css_provider) {
    gtk_style_context_remove_provider_for_screen (gdk_screen_get_default (),
                                                  GTK_STYLE_PROVIDER (self->css_provider));
  }

  style = phosh_style_manager_get_stylesheet (self->theme_name);

  gtk_css_provider_load_from_resource (provider, style);
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_set_object (&self->css_provider, provider);
}


static void
phosh_style_manager_dispose (GObject *object)
{
  PhoshStyleManager *self = PHOSH_STYLE_MANAGER (object);

  g_clear_pointer (&self->theme_name, g_free);
  g_clear_object (&self->css_provider);

  G_OBJECT_CLASS (phosh_style_manager_parent_class)->dispose (object);
}


static void
phosh_style_manager_class_init (PhoshStyleManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = phosh_style_manager_dispose;
}


static void
phosh_style_manager_init (PhoshStyleManager *self)
{
  GtkSettings *gtk_settings = gtk_settings_get_default ();

  g_object_set (G_OBJECT (gtk_settings), "gtk-application-prefer-dark-theme", TRUE, NULL);

  g_signal_connect_swapped (gtk_settings,
                            "notify::gtk-theme-name",
                            G_CALLBACK (on_gtk_theme_name_changed),
                            self);
  on_gtk_theme_name_changed (self, NULL, gtk_settings);
}


PhoshStyleManager *
phosh_style_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_STYLE_MANAGER, NULL);
}

/**
 * phosh_style_manager_get_stylesheet:
 * @theme_name: A theme name
 *
 * Get the proper style sheet based on the given theme name
 */
const char *
phosh_style_manager_get_stylesheet (const char *theme_name)
{
  const char *style;

  if (g_strcmp0 (theme_name, "HighContrast") == 0)
    style = "/sm/puri/phosh/stylesheet/adwaita-hc-light.css";
  else
    style = "/sm/puri/phosh/stylesheet/adwaita-dark.css";

  return style;
}
