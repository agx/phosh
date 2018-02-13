/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include <glib/gi18n.h>

#include "settings.h"
#include "settings/brightness.h"


typedef struct
{
  GtkWidget *scale_brightness;
  GtkWidget *scale_volume;

  GtkAdjustment *adj_brightness;
  GtkAdjustment *adj_volume;

} PhoshSettingsPrivate;


typedef struct _PhoshSettings
{
  PhoshMenu parent;
} PhoshSettings;


G_DEFINE_TYPE_WITH_PRIVATE (PhoshSettings, phosh_settings, PHOSH_TYPE_MENU)


GtkWidget *phosh_settings (const char* name)
{
  return g_object_new (PHOSH_TYPE_SETTINGS, "name", name, NULL);
}


static void
brightness_changed_cb (GtkAdjustment *adj_brightness, gpointer *unused)
{
  int brightness;

  brightness = (int)gtk_adjustment_get_value (adj_brightness);
  brightness_set (brightness);
}


static void
phosh_settings_constructed (GObject *object)
{
  PhoshSettings *self = PHOSH_SETTINGS (object);
  PhoshSettingsPrivate *priv = phosh_settings_get_instance_private (self);

  priv->adj_brightness = gtk_adjustment_new (0, 0, 100, 1, 10, 10);
  gtk_range_set_adjustment (GTK_RANGE (priv->scale_brightness), priv->adj_brightness);
  gtk_range_set_round_digits (GTK_RANGE (priv->scale_brightness), 0);
  
  brightness_init (priv->adj_brightness);
  g_signal_connect (priv->adj_brightness,
		    "value-changed",
		    G_CALLBACK(brightness_changed_cb),
		    NULL);

  priv->adj_volume = gtk_adjustment_new (0, 0, 100, 1, 10, 10);
  gtk_range_set_adjustment (GTK_RANGE (priv->scale_volume), priv->adj_volume);

  G_OBJECT_CLASS (phosh_settings_parent_class)->constructed (object);
}


static void
phosh_settings_class_init (PhoshSettingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
					       "/sm/puri/phosh/ui/settings-menu.ui");

  object_class->constructed = phosh_settings_constructed;
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSettings, scale_volume);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSettings, scale_brightness);
}


static void
phosh_settings_init (PhoshSettings *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
phosh_settings_new (int position, const gpointer *shell)
{
  return g_object_new (PHOSH_TYPE_SETTINGS,
		       "name", "settings",
		       "shell", shell,
		       "position", position,
		       NULL);
}
