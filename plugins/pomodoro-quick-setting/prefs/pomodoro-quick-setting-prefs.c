/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-plugin-prefs-config.h"

#include "pomodoro-quick-setting-prefs.h"

#include <gio/gio.h>

#include <glib/gi18n-lib.h>

#define POMODORO_QUICK_SETTING_SCHEMA_ID "mobi.phosh.plugins.pomodoro"
#define POMODORO_QUICK_SETTING_ACTIVE_DURATION_KEY "active-duration"
#define POMODORO_QUICK_SETTING_BREAK_DURATION_KEY "break-duration"

/**
 * PhoshPomodoroQuickSettingPrefs:
 *
 * Preferences for Pomodoro quick setting plugin
 */
struct _PhoshPomodoroQuickSettingPrefs {
  AdwPreferencesWindow  parent;

  AdwSpinRow           *active_duration_spin_row;
  AdwSpinRow           *break_duration_spin_row;

  GSettings            *setting;
};

G_DEFINE_TYPE (PhoshPomodoroQuickSettingPrefs, phosh_pomodoro_quick_setting_prefs,
               ADW_TYPE_PREFERENCES_WINDOW);


static gboolean
duration_get_mapping (GValue *value, GVariant *variant, gpointer user_data)
{
  float duration;

  duration = g_variant_get_int32 (variant) / 60.0;
  g_value_set_double (value, duration);

  return TRUE;
}


static GVariant *
duration_set_mapping (const GValue *value, const GVariantType *type, gpointer user_data)
{
  double duration;

  duration = g_value_get_double (value) * 60.0;

  return g_variant_new_int32 ((gint32)rint (duration));
}


static void
phosh_pomodoro_quick_setting_prefs_finalize (GObject *object)
{
  PhoshPomodoroQuickSettingPrefs *self = PHOSH_POMODORO_QUICK_SETTING_PREFS (object);

  g_clear_object (&self->setting);

  G_OBJECT_CLASS (phosh_pomodoro_quick_setting_prefs_parent_class)->finalize (object);
}


static void
phosh_pomodoro_quick_setting_prefs_class_init (PhoshPomodoroQuickSettingPrefsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_pomodoro_quick_setting_prefs_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/plugins/"
                                               "pomodoro-quick-setting-prefs/prefs.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshPomodoroQuickSettingPrefs,
                                        active_duration_spin_row);
  gtk_widget_class_bind_template_child (widget_class, PhoshPomodoroQuickSettingPrefs,
                                        break_duration_spin_row);
}


static void
phosh_pomodoro_quick_setting_prefs_init (PhoshPomodoroQuickSettingPrefs *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->setting = g_settings_new (POMODORO_QUICK_SETTING_SCHEMA_ID);

  g_settings_bind_with_mapping (self->setting, POMODORO_QUICK_SETTING_ACTIVE_DURATION_KEY,
                                self->active_duration_spin_row, "value",
                                G_SETTINGS_BIND_DEFAULT,
                                duration_get_mapping,
                                duration_set_mapping,
                                NULL /* userdata */,
                                NULL /* destroyfunc */);

  g_settings_bind_with_mapping (self->setting, POMODORO_QUICK_SETTING_BREAK_DURATION_KEY,
                                self->break_duration_spin_row, "value",
                                G_SETTINGS_BIND_DEFAULT,
                                duration_get_mapping,
                                duration_set_mapping,
                                NULL /* userdata */,
                                NULL /* destroyfunc */);
}
