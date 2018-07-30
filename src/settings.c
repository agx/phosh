/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-settings"

#include <glib/gi18n.h>

#include "phosh.h"
#include "settings.h"
#include "settings/brightness.h"

#include <gio/gdesktopappinfo.h>

enum {
  SETTING_DONE,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

typedef struct
{
  GtkWidget *scale_brightness;
  GtkWidget *scale_volume;
  GtkWidget *btn_rotation;

  GtkWidget *btn_settings;
  GDesktopAppInfo *settings_info;
  GtkWidget *btn_airplane_mode;
  GtkWidget *btn_lock_screen;

} PhoshSettingsPrivate;


typedef struct _PhoshSettings
{
  GtkWindow parent;
} PhoshSettings;


G_DEFINE_TYPE_WITH_PRIVATE (PhoshSettings, phosh_settings, GTK_TYPE_WINDOW)


static void
brightness_value_changed_cb (GtkScale *scale_brightness, gpointer *unused)
{
  int brightness;

  brightness = (int)gtk_range_get_value (GTK_RANGE (scale_brightness));
  brightness_set (brightness);
}


static void
rotation_changed_cb (GtkSwitch *btn, GParamSpec *pspec, PhoshSettings *self)
{
  PhoshShell *shell = phosh_shell_get_default ();
  gboolean rotate;

  rotate = gtk_switch_get_active(btn);

  if (rotate)
    phosh_shell_rotate_display (shell, 90);
  else
    phosh_shell_rotate_display (shell, 0);

  g_signal_emit (self, signals[SETTING_DONE], 0);
}


static void
settings_clicked_cb (PhoshSettings *self, gpointer *unused)
{
  PhoshSettingsPrivate *priv = phosh_settings_get_instance_private (self);

  g_return_if_fail (priv->settings_info);
  g_app_info_launch (G_APP_INFO (priv->settings_info), NULL, NULL, NULL);

  g_signal_emit (self, signals[SETTING_DONE], 0);
}


static void
lock_screen_clicked_cb (PhoshSettings *self, gpointer *unused)
{
  phosh_shell_lock (phosh_shell_get_default ());
  g_signal_emit (self, signals[SETTING_DONE], 0);
}


static void
phosh_settings_constructed (GObject *object)
{
  PhoshSettings *self = PHOSH_SETTINGS (object);
  PhoshSettingsPrivate *priv = phosh_settings_get_instance_private (self);
  GtkWidget *image;

  /* window properties */
  gtk_window_set_title (GTK_WINDOW (self), "phosh settings");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_window_resize (GTK_WINDOW (self), 100, 100);
  gtk_widget_realize(GTK_WIDGET (self));

  gtk_range_set_range (GTK_RANGE (priv->scale_brightness), 0, 100);
  gtk_range_set_round_digits (GTK_RANGE (priv->scale_brightness), 0);
  gtk_range_set_increments (GTK_RANGE (priv->scale_brightness), 1, 10);
  brightness_init (GTK_SCALE (priv->scale_brightness));
  g_signal_connect (priv->scale_brightness,
                    "value-changed",
                    G_CALLBACK(brightness_value_changed_cb),
                    NULL);

  gtk_range_set_range (GTK_RANGE (priv->scale_volume), 0, 100);

  if (phosh_shell_get_rotation (phosh_shell_get_default ()))
    gtk_switch_set_active (GTK_SWITCH (priv->btn_rotation), TRUE);
  g_signal_connect (priv->btn_rotation,
                    "notify::active",
                    G_CALLBACK (rotation_changed_cb),
                    self);

  gtk_style_context_remove_class (gtk_widget_get_style_context (priv->btn_settings),
                                  "button");
  gtk_style_context_remove_class (gtk_widget_get_style_context (priv->btn_settings),
                                  "image-button");
  image = gtk_image_new_from_icon_name ("preferences-system-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image(GTK_BUTTON (priv->btn_settings), image);
  priv->settings_info = g_desktop_app_info_new ("gnome-control-center.desktop");
  g_signal_connect_swapped (priv->btn_settings,
                            "clicked",
                            G_CALLBACK (settings_clicked_cb),
                            self);

  image = gtk_image_new_from_icon_name ("system-lock-screen-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image(GTK_BUTTON (priv->btn_lock_screen), image);
  g_signal_connect_swapped (priv->btn_lock_screen,
                            "clicked",
                            G_CALLBACK (lock_screen_clicked_cb),
                            self);

  /* FIXME: just so we have some buttons */
  image = gtk_image_new_from_icon_name ("airplane-mode-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image(GTK_BUTTON (priv->btn_airplane_mode), image);


  G_OBJECT_CLASS (phosh_settings_parent_class)->constructed (object);
}


static void
phosh_settings_dispose (GObject *object)
{
  brightness_dispose ();

  G_OBJECT_CLASS (phosh_settings_parent_class)->dispose (object);
}



static void
phosh_settings_class_init (PhoshSettingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = phosh_settings_dispose;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/settings-menu.ui");

  signals[SETTING_DONE] = g_signal_new ("setting-done",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  object_class->constructed = phosh_settings_constructed;
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSettings, scale_volume);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSettings, scale_brightness);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSettings, btn_rotation);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSettings, btn_settings);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSettings, btn_lock_screen);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSettings, btn_airplane_mode);
}


static void
phosh_settings_init (PhoshSettings *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
phosh_settings_new (void)
{
  return g_object_new (PHOSH_TYPE_SETTINGS, NULL);
}
