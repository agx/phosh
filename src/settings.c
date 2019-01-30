/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-settings"

#include <glib/gi18n.h>

#include "shell.h"
#include "session.h"
#include "settings.h"
#include "settings/brightness.h"
#include "settings/gvc-channel-bar.h"

#include <pulse/pulseaudio.h>
#include "gvc-mixer-control.h"
#include "gvc-mixer-stream.h"
#include <gio/gdesktopappinfo.h>

#include <math.h>

enum {
  SETTING_DONE,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

typedef struct
{
  GtkWidget *box_settings;
  GtkWidget *scale_brightness;
  GtkWidget *output_vol_bar;
  GtkWidget *btn_rotation;

  GtkWidget *btn_settings;
  GDesktopAppInfo *settings_info;
  GtkWidget *btn_shutdown;
  GtkWidget *btn_lock_screen;

  /* Output volume control */
  GvcMixerControl *mixer_control;
  GvcMixerStream *output_stream;
  gboolean allow_volume_above_100_percent;
  gboolean setting_volume;

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
  phosh_shell_rotate_display (shell, rotate ? 90 : 0);
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
update_output_vol_bar (PhoshSettings *self)
{
  PhoshSettingsPrivate *priv = phosh_settings_get_instance_private (self);
  GtkAdjustment *adj;

  priv->setting_volume = TRUE;
  gvc_channel_bar_set_base_volume (GVC_CHANNEL_BAR (priv->output_vol_bar),
                                   gvc_mixer_stream_get_base_volume (priv->output_stream));
  gvc_channel_bar_set_is_amplified (GVC_CHANNEL_BAR (priv->output_vol_bar),
                                    priv->allow_volume_above_100_percent &&
                                    gvc_mixer_stream_get_can_decibel (priv->output_stream));
  adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (priv->output_vol_bar)));
  g_debug ("Adjusting volume to %d", gvc_mixer_stream_get_volume (priv->output_stream));
  gtk_adjustment_set_value (adj, gvc_mixer_stream_get_volume (priv->output_stream));
  priv->setting_volume = FALSE;
}


static void
output_stream_notify_is_muted_cb (GvcMixerStream *stream, GParamSpec *pspec, gpointer data)
{
  PhoshSettings *self = PHOSH_SETTINGS (data);
  PhoshSettingsPrivate *priv = priv = phosh_settings_get_instance_private (self);

  if (!priv->setting_volume)
    update_output_vol_bar (self);
}


static void
output_stream_notify_volume_cb (GvcMixerStream *stream, GParamSpec *pspec, gpointer data)
{
  PhoshSettings *self = PHOSH_SETTINGS (data);
  PhoshSettingsPrivate *priv = phosh_settings_get_instance_private (self);

  if (!priv->setting_volume)
    update_output_vol_bar (self);
}



static GtkWidget *
create_vol_channel_bar (PhoshSettings *self)
{
  GtkWidget *bar;

  bar = gvc_channel_bar_new ();
  gtk_widget_set_sensitive (bar, TRUE);
  return bar;
}


static void
mixer_control_output_update_cb (GvcMixerControl *mixer, guint id, gpointer *data)
{
  PhoshSettings *self = PHOSH_SETTINGS (data);
  PhoshSettingsPrivate *priv;

  g_debug ("Output updated: %d", id);

  g_return_if_fail (PHOSH_IS_SETTINGS (self));
  priv = phosh_settings_get_instance_private (self);

  if (priv->output_stream)
    g_signal_handlers_disconnect_by_data (priv->output_stream, self);

  priv->output_stream = gvc_mixer_control_get_default_sink (priv->mixer_control);
  g_return_if_fail (priv->output_stream);

  g_signal_connect (priv->output_stream,
                    "notify::volume",
                    G_CALLBACK (output_stream_notify_volume_cb),
                    self);

  g_signal_connect (priv->output_stream,
                    "notify::is-muted",
                    G_CALLBACK (output_stream_notify_is_muted_cb),
                    self);
  update_output_vol_bar (self);
}


static void
vol_adjustment_value_changed_cb (GtkAdjustment *adjustment,
                                 PhoshSettings *self)
{
  PhoshSettingsPrivate *priv = phosh_settings_get_instance_private (self);
  gdouble volume, rounded;
  g_autofree gchar *name = NULL;

  if (!priv->output_stream)
    priv->output_stream = gvc_mixer_control_get_default_sink (priv->mixer_control);

  volume = gtk_adjustment_get_value (adjustment);
  rounded = round (volume);

  g_object_get (priv->output_vol_bar, "name", &name, NULL);
  g_debug ("Setting stream volume %lf (rounded: %lf) for bar '%s'", volume, rounded, name);

  g_return_if_fail (priv->output_stream);
  if (gvc_mixer_stream_set_volume (priv->output_stream, (pa_volume_t) rounded) != FALSE)
    gvc_mixer_stream_push_volume (priv->output_stream);
}


static void
shutdown_clicked_cb (PhoshSettings *self, gpointer *unused)
{
  phosh_session_shutdown ();
  /* TODO: Since we don't implement
   * gnome.SessionManager.EndSessionDialog yet */
  phosh_session_shutdown ();
  g_signal_emit (self, signals[SETTING_DONE], 0);
}


static void
phosh_settings_constructed (GObject *object)
{
  PhoshSettings *self = PHOSH_SETTINGS (object);
  PhoshSettingsPrivate *priv = phosh_settings_get_instance_private (self);
  GtkWidget *image;
  GtkAdjustment *adj;

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

  priv->output_vol_bar = create_vol_channel_bar (self);
  gtk_box_pack_start (GTK_BOX (priv->box_settings), priv->output_vol_bar, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (priv->box_settings), priv->output_vol_bar, 0);

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

  image = gtk_image_new_from_icon_name ("system-shutdown-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image(GTK_BUTTON (priv->btn_shutdown), image);
  g_signal_connect_swapped (priv->btn_shutdown,
                            "clicked",
                            G_CALLBACK (shutdown_clicked_cb),
                            self);

  priv->mixer_control = gvc_mixer_control_new ("Phone Shell Volume Control");
  g_return_if_fail (priv->mixer_control);

  gvc_mixer_control_open (priv->mixer_control);
  g_signal_connect (priv->mixer_control,
                    "active-output-update",
                    G_CALLBACK (mixer_control_output_update_cb),
                    self);
  adj = gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (priv->output_vol_bar));
  g_signal_connect (adj,
                    "value-changed",
                    G_CALLBACK (vol_adjustment_value_changed_cb),
                    self);

  G_OBJECT_CLASS (phosh_settings_parent_class)->constructed (object);
}


static void
phosh_settings_dispose (GObject *object)
{
  brightness_dispose ();

  G_OBJECT_CLASS (phosh_settings_parent_class)->dispose (object);
}


static void
phosh_settings_finalize (GObject *object)
{
  PhoshSettings *self = PHOSH_SETTINGS (object);
  PhoshSettingsPrivate *priv = phosh_settings_get_instance_private (self);

  g_clear_object (&priv->mixer_control);

  G_OBJECT_CLASS (phosh_settings_parent_class)->finalize (object);
}



static void
phosh_settings_class_init (PhoshSettingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = phosh_settings_dispose;
  object_class->finalize = phosh_settings_finalize;
  object_class->constructed = phosh_settings_constructed;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/settings-menu.ui");

  signals[SETTING_DONE] = g_signal_new ("setting-done",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  gtk_widget_class_bind_template_child_private (widget_class, PhoshSettings, box_settings);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSettings, scale_brightness);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSettings, btn_rotation);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSettings, btn_settings);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSettings, btn_lock_screen);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSettings, btn_shutdown);
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
