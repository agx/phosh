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
#include "quick-setting.h"
#include "settings/brightness.h"
#include "settings/gvc-channel-bar.h"
#include "wwan/phosh-wwan-mm.h"
#include "rotateinfo.h"
#include "feedbackinfo.h"
#include "feedback-manager.h"

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

typedef struct _PhoshSettings
{
  GtkBin parent;

  GtkWidget *box_settings;
  GtkWidget *quick_settings;
  GtkWidget *scale_brightness;
  GtkWidget *output_vol_bar;

  GtkWidget *btn_settings;
  GDesktopAppInfo *settings_info;
  GtkWidget *btn_shutdown;
  GtkWidget *btn_lock_screen;

  /* Output volume control */
  GvcMixerControl *mixer_control;
  GvcMixerStream *output_stream;
  gboolean allow_volume_above_100_percent;
  gboolean setting_volume;
} PhoshSettings;


G_DEFINE_TYPE (PhoshSettings, phosh_settings, GTK_TYPE_BIN)


static void
brightness_value_changed_cb (GtkScale *scale_brightness, gpointer *unused)
{
  int brightness;

  brightness = (int)gtk_range_get_value (GTK_RANGE (scale_brightness));
  brightness_set (brightness);
}


static void
rotation_setting_clicked_cb (PhoshSettings *self)
{
  PhoshShell *shell = phosh_shell_get_default ();
  gboolean rotated;

  g_return_if_fail (PHOSH_IS_SETTINGS (self));
  rotated = phosh_shell_get_rotation (shell);
  phosh_shell_rotate_display (shell, !rotated ? 90 : 0);
  g_signal_emit (self, signals[SETTING_DONE], 0);
}

static void
feedback_setting_clicked_cb (PhoshSettings *self)
{
  PhoshShell *shell;
  PhoshFeedbackManager *manager;

  shell = phosh_shell_get_default ();
  g_return_if_fail (PHOSH_IS_SHELL (shell));
  manager = phosh_shell_get_feedback_manager (shell);
  g_return_if_fail (PHOSH_IS_FEEDBACK_MANAGER (manager));
  manager = phosh_shell_get_feedback_manager (shell);
  phosh_feedback_manager_toggle (manager);
}

static void
feedback_setting_long_pressed_cb (PhoshSettings *self)
{
  phosh_quick_setting_open_settings_panel ("notifications");
  g_signal_emit (self, signals[SETTING_DONE], 0);
}

static void
batteryinfo_clicked_cb (PhoshSettings *self)
{
  phosh_quick_setting_open_settings_panel ("power");
  g_signal_emit (self, signals[SETTING_DONE], 0);
}


static void
settings_clicked_cb (PhoshSettings *self, gpointer *unused)
{
  g_return_if_fail (PHOSH_IS_SETTINGS (self));
  g_return_if_fail (self->settings_info);
  g_app_info_launch (G_APP_INFO (self->settings_info), NULL, NULL, NULL);

  g_signal_emit (self, signals[SETTING_DONE], 0);
}


static void
lock_screen_clicked_cb (PhoshSettings *self, gpointer *unused)
{
  g_return_if_fail (PHOSH_IS_SETTINGS (self));
  g_signal_emit (self, signals[SETTING_DONE], 0);
  phosh_shell_lock (phosh_shell_get_default ());
}


static void
update_output_vol_bar (PhoshSettings *self)
{
  GtkAdjustment *adj;

  self->setting_volume = TRUE;
  gvc_channel_bar_set_base_volume (GVC_CHANNEL_BAR (self->output_vol_bar),
                                   gvc_mixer_stream_get_base_volume (self->output_stream));
  gvc_channel_bar_set_is_amplified (GVC_CHANNEL_BAR (self->output_vol_bar),
                                    self->allow_volume_above_100_percent &&
                                    gvc_mixer_stream_get_can_decibel (self->output_stream));
  adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (self->output_vol_bar)));
  g_debug ("Adjusting volume to %d", gvc_mixer_stream_get_volume (self->output_stream));
  gtk_adjustment_set_value (adj, gvc_mixer_stream_get_volume (self->output_stream));
  self->setting_volume = FALSE;
}


static void
output_stream_notify_is_muted_cb (GvcMixerStream *stream, GParamSpec *pspec, gpointer data)
{
  PhoshSettings *self = PHOSH_SETTINGS (data);

  if (!self->setting_volume)
    update_output_vol_bar (self);
}


static void
output_stream_notify_volume_cb (GvcMixerStream *stream, GParamSpec *pspec, gpointer data)
{
  PhoshSettings *self = PHOSH_SETTINGS (data);

  if (!self->setting_volume)
    update_output_vol_bar (self);
}



static GtkWidget *
create_vol_channel_bar (PhoshSettings *self)
{
  GtkWidget *bar;

  bar = gvc_channel_bar_new ();
  gtk_widget_set_sensitive (bar, TRUE);
  gtk_widget_show (bar);
  return bar;
}


static void
mixer_control_output_update_cb (GvcMixerControl *mixer, guint id, gpointer *data)
{
  PhoshSettings *self = PHOSH_SETTINGS (data);

  g_debug ("Output updated: %d", id);

  g_return_if_fail (PHOSH_IS_SETTINGS (self));

  if (self->output_stream)
    g_signal_handlers_disconnect_by_data (self->output_stream, self);

  self->output_stream = gvc_mixer_control_get_default_sink (self->mixer_control);
  g_return_if_fail (self->output_stream);

  g_signal_connect (self->output_stream,
                    "notify::volume",
                    G_CALLBACK (output_stream_notify_volume_cb),
                    self);

  g_signal_connect (self->output_stream,
                    "notify::is-muted",
                    G_CALLBACK (output_stream_notify_is_muted_cb),
                    self);
  update_output_vol_bar (self);
}


static void
vol_adjustment_value_changed_cb (GtkAdjustment *adjustment,
                                 PhoshSettings *self)
{
  gdouble volume, rounded;
  g_autofree gchar *name = NULL;

  if (!self->output_stream)
    self->output_stream = gvc_mixer_control_get_default_sink (self->mixer_control);

  volume = gtk_adjustment_get_value (adjustment);
  rounded = round (volume);

  g_object_get (self->output_vol_bar, "name", &name, NULL);
  g_debug ("Setting stream volume %lf (rounded: %lf) for bar '%s'", volume, rounded, name);

  g_return_if_fail (self->output_stream);
  if (gvc_mixer_stream_set_volume (self->output_stream, (pa_volume_t) rounded) != FALSE)
    gvc_mixer_stream_push_volume (self->output_stream);
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
on_quicksetting_activated (PhoshSettings   *self,
                           GtkFlowBoxChild *child,
                           GtkFlowBox      *box)

{
  GtkWidget *quick_setting;

  quick_setting = gtk_bin_get_child (GTK_BIN (child));
  gtk_button_clicked (GTK_BUTTON (quick_setting));
}


static void
phosh_settings_constructed (GObject *object)
{
  PhoshSettings *self = PHOSH_SETTINGS (object);
  GtkWidget *image;
  GtkAdjustment *adj;

  gtk_range_set_range (GTK_RANGE (self->scale_brightness), 0, 100);
  gtk_range_set_round_digits (GTK_RANGE (self->scale_brightness), 0);
  gtk_range_set_increments (GTK_RANGE (self->scale_brightness), 1, 10);
  brightness_init (GTK_SCALE (self->scale_brightness));
  g_signal_connect (self->scale_brightness,
                    "value-changed",
                    G_CALLBACK(brightness_value_changed_cb),
                    NULL);

  self->output_vol_bar = create_vol_channel_bar (self);
  gtk_box_pack_start (GTK_BOX (self->box_settings), self->output_vol_bar, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (self->box_settings), self->output_vol_bar, 1);

  gtk_style_context_remove_class (gtk_widget_get_style_context (self->btn_settings),
                                  "button");
  gtk_style_context_remove_class (gtk_widget_get_style_context (self->btn_settings),
                                  "image-button");
  image = gtk_image_new_from_icon_name ("preferences-system-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image(GTK_BUTTON (self->btn_settings), image);
  self->settings_info = g_desktop_app_info_new ("gnome-control-center.desktop");
  g_signal_connect_swapped (self->btn_settings,
                            "clicked",
                            G_CALLBACK (settings_clicked_cb),
                            self);

  image = gtk_image_new_from_icon_name ("system-lock-screen-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image(GTK_BUTTON (self->btn_lock_screen), image);
  g_signal_connect_swapped (self->btn_lock_screen,
                            "clicked",
                            G_CALLBACK (lock_screen_clicked_cb),
                            self);

  image = gtk_image_new_from_icon_name ("system-shutdown-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image(GTK_BUTTON (self->btn_shutdown), image);
  g_signal_connect_swapped (self->btn_shutdown,
                            "clicked",
                            G_CALLBACK (shutdown_clicked_cb),
                            self);

  self->mixer_control = gvc_mixer_control_new ("Phone Shell Volume Control");
  g_return_if_fail (self->mixer_control);

  gvc_mixer_control_open (self->mixer_control);
  g_signal_connect (self->mixer_control,
                    "active-output-update",
                    G_CALLBACK (mixer_control_output_update_cb),
                    self);
  adj = gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (self->output_vol_bar));
  g_signal_connect (adj,
                    "value-changed",
                    G_CALLBACK (vol_adjustment_value_changed_cb),
                    self);

  g_signal_connect (self->quick_settings,
                    "child-activated",
                    G_CALLBACK (on_quicksetting_activated),
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

  g_clear_object (&self->mixer_control);

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

  g_type_ensure (PHOSH_TYPE_QUICK_SETTING);
  g_type_ensure (PHOSH_TYPE_ROTATE_INFO);
  g_type_ensure (PHOSH_TYPE_FEEDBACK_INFO);

  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, box_settings);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, quick_settings);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, scale_brightness);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, btn_settings);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, btn_lock_screen);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, btn_shutdown);

  gtk_widget_class_bind_template_callback (widget_class, batteryinfo_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, rotation_setting_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, feedback_setting_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, feedback_setting_long_pressed_cb);
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
