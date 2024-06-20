/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Teemu Ikonen <tpikonen@mailbox.org>
 */

#include "monitor-manager.h"
#include "dark-mode-quick-setting.h"
#include "plugin-shell.h"
#include "quick-setting.h"

#include <glib/gi18n.h>
#include <gsettings-desktop-schemas/gdesktop-enums.h>

/**
 * PhoshDarkModeQuickSetting:
 *
 * Set the color-scheme enum.
 */
struct _PhoshDarkModeQuickSetting {
  PhoshQuickSetting parent;

  GSettings        *settings;
  PhoshStatusIcon  *info;
};

G_DEFINE_TYPE (PhoshDarkModeQuickSetting, phosh_dark_mode_quick_setting, PHOSH_TYPE_QUICK_SETTING);

static const char*
enum_to_info (gint color_scheme)
{
  const char *labels[] = {
    _("Default style"),
    _("Dark mode"),
    _("Light mode"),
  };
  return labels[CLAMP (color_scheme, 0, 2)];
}

static const char*
enum_to_icon_name (gint color_scheme)
{
  const char *icons[] = {
    "dark-mode-disabled-symbolic",
    "dark-mode-symbolic",
    "weather-clear",
  };
  return icons[CLAMP (color_scheme, 0, 2)];
}

static void
set_props_from_enum (PhoshDarkModeQuickSetting *self,
                     gint                       color_scheme)
{
  g_object_set (self->info, "icon-name", enum_to_icon_name (color_scheme), NULL);
  g_object_set (self->info, "info", enum_to_info (color_scheme), NULL);
  g_object_set (self, "active", color_scheme == 1, NULL);
}

static void
on_clicked (PhoshDarkModeQuickSetting *self)
{
  gint color_scheme = g_settings_get_enum (self->settings, "color-scheme");

  /* Only allow setting 'default' or 'prefer-dark' by clicking */
  g_settings_set_enum (self->settings, "color-scheme",
                       (color_scheme == G_DESKTOP_COLOR_SCHEME_PREFER_DARK) ?
                       G_DESKTOP_COLOR_SCHEME_DEFAULT :
                       G_DESKTOP_COLOR_SCHEME_PREFER_DARK);
}

static void
on_color_scheme_changed (PhoshDarkModeQuickSetting *self,
                         gchar                     *_key,
                         GSettings                 *_settings)
{
  gint color_scheme = g_settings_get_enum (self->settings, "color-scheme");

  set_props_from_enum (self, color_scheme);
}

static void
phosh_lockscreen_finalize (GObject *object)
{
  PhoshDarkModeQuickSetting *self = PHOSH_DARK_MODE_QUICK_SETTING (object);

  g_clear_object (&self->settings);

  G_OBJECT_CLASS (phosh_dark_mode_quick_setting_parent_class)->finalize (object);
}


static void
phosh_dark_mode_quick_setting_class_init (PhoshDarkModeQuickSettingClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_lockscreen_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/plugins/dark-mode-quick-setting/qs.ui");

  gtk_widget_class_bind_template_child (widget_class, PhoshDarkModeQuickSetting, info);

  gtk_widget_class_bind_template_callback (widget_class, on_clicked);
}


static void
phosh_dark_mode_quick_setting_init (PhoshDarkModeQuickSetting *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_icon_theme_add_resource_path (gtk_icon_theme_get_default (),
                                    "/mobi/phosh/plugins/dark-mode-quick-setting/icons");

  self->settings = g_settings_new ("org.gnome.desktop.interface");

  /* Initialize label and icon */
  on_color_scheme_changed (self, "color-scheme", self->settings);

  g_signal_connect_swapped (self->settings, "changed::color-scheme",
                            (GCallback) on_color_scheme_changed, self);
}
