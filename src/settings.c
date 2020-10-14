/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-settings"

#include <glib/gi18n.h>

#include "bt-info.h"
#include "shell.h"
#include "settings.h"
#include "quick-setting.h"
#include "settings/brightness.h"
#include "settings/gvc-channel-bar.h"
#include "torch-info.h"
#include "wwan/phosh-wwan-mm.h"
#include "rotateinfo.h"
#include "feedbackinfo.h"
#include "feedback-manager.h"
#include "notifications/notify-manager.h"
#include "notifications/notification-frame.h"
#include "media-player.h"
#include "keyboard-events.h"

#include <pulse/pulseaudio.h>
#include "gvc-mixer-control.h"
#include "gvc-mixer-stream.h"
#include <gio/gdesktopappinfo.h>
#include <xkbcommon/xkbcommon.h>

#include <math.h>

#define LIBFEEDBACK_USE_UNSTABLE_API
#include <libfeedback.h>

#define VOLUME_SCALE 5

/**
 * SECTION:settings
 * @short_description: The settings menu
 * @Title: PhoshSettings
 */

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

  /* Output volume control */
  GvcMixerControl *mixer_control;
  GvcMixerStream *output_stream;
  gboolean allow_volume_above_100_percent;
  gboolean setting_volume;

  /* Notifications */
  GtkWidget *list_notifications;
  GtkWidget *sw_notifications;
  LfbEvent  *notify_event;

  /* KeyboardEvents */
  PhoshKeyboardEvents *keyboard_events;
  GHashTable *accelerator_callbacks;
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
  PhoshMonitorTransform transform;

  g_return_if_fail (PHOSH_IS_SETTINGS (self));
  transform = phosh_shell_get_transform (shell);
  phosh_shell_set_transform (shell, transform == PHOSH_MONITOR_TRANSFORM_NORMAL
                             ? PHOSH_MONITOR_TRANSFORM_90
                             : PHOSH_MONITOR_TRANSFORM_NORMAL);
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
wifi_setting_clicked_cb (PhoshSettings *self)
{
  phosh_quick_setting_open_settings_panel ("wifi");
  g_signal_emit (self, signals[SETTING_DONE], 0);
}

static void
wwan_setting_clicked_cb (PhoshSettings *self)
{
  phosh_quick_setting_open_settings_panel ("wwan");
  g_signal_emit (self, signals[SETTING_DONE], 0);
}

static void
bt_setting_clicked_cb (PhoshSettings *self)
{
  phosh_quick_setting_open_settings_panel ("bluetooth");
  g_signal_emit (self, signals[SETTING_DONE], 0);
}

static void
feedback_setting_long_pressed_cb (PhoshSettings *self)
{
  phosh_quick_setting_open_settings_panel ("notifications");
  g_signal_emit (self, signals[SETTING_DONE], 0);
}

static void
battery_setting_clicked_cb (PhoshSettings *self)
{
  phosh_quick_setting_open_settings_panel ("power");
  g_signal_emit (self, signals[SETTING_DONE], 0);
}


static void
torch_setting_clicked_cb (PhoshSettings *self)
{
  PhoshShell *shell;
  PhoshTorchManager *manager;

  shell = phosh_shell_get_default ();
  g_return_if_fail (PHOSH_IS_SHELL (shell));
  manager = phosh_shell_get_torch_manager (shell);
  g_return_if_fail (PHOSH_IS_TORCH_MANAGER (manager));
  phosh_torch_manager_toggle (manager);
}

static void
change_volume (PhoshSettings *self,
               int            steps)
{
  GtkAdjustment *adj;
  double vol, inc;

  adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (self->output_vol_bar)));

  vol = gtk_adjustment_get_value (adj);
  inc = gtk_adjustment_get_step_increment (adj);

  vol += steps * inc * VOLUME_SCALE;

  g_debug ("Setting volume to %f", vol);

  gtk_adjustment_set_value (adj, vol);
}

static void
lower_volume (PhoshSettings *self)
{
  change_volume (self, -1);
}

static void
raise_volume (PhoshSettings *self)
{
  change_volume (self, 1);
}

static void
accelerator_grabbed_cb (PhoshSettings *self,
                        const char    *accelerator,
                        uint32_t       action_id)
{
  guint64 action = action_id;
  if (g_strcmp0 (accelerator, "XF86AudioLowerVolume") == 0) {
    g_hash_table_insert (self->accelerator_callbacks, (gpointer) action, (gpointer) lower_volume);
  } else if (g_strcmp0 (accelerator, "XF86AudioRaiseVolume") == 0) {
     g_hash_table_insert (self->accelerator_callbacks, (gpointer) action, (gpointer) raise_volume);
  }
}

static void
accelerator_activated_cb (PhoshSettings *self,
                          uint32_t       action_id,
                          uint32_t       timestamp)
{
  void (*callback)(PhoshSettings *);
  guint64 action = action_id;
  g_return_if_fail (PHOSH_IS_SETTINGS (self));
  callback = g_hash_table_lookup (self->accelerator_callbacks, (gpointer) action);
  if (callback == NULL) {
    g_warning ("No callback for action %d", action_id);
    return;
  }
  callback (self);
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


static void
mixer_control_output_update_cb (GvcMixerControl *mixer, guint id, gpointer *data)
{
  PhoshSettings *self = PHOSH_SETTINGS (data);

  g_debug ("Audio output updated: %d", id);

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
  double volume, rounded;
  g_autofree char *name = NULL;

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
on_quicksetting_activated (PhoshSettings   *self,
                           GtkFlowBoxChild *child,
                           GtkFlowBox      *box)

{
  GtkWidget *quick_setting;

  quick_setting = gtk_bin_get_child (GTK_BIN (child));
  gtk_button_clicked (GTK_BUTTON (quick_setting));
}

static void
on_media_player_raised (PhoshSettings *self,
                        gpointer       unused)
{
  g_return_if_fail (PHOSH_IS_SETTINGS (self));
  g_signal_emit (self, signals[SETTING_DONE], 0);
}

static GtkWidget *
create_notification_row (gpointer item, gpointer data)
{
  GtkWidget *row = NULL;
  GtkWidget *frame = NULL;

  row = g_object_new (GTK_TYPE_LIST_BOX_ROW,
                      "activatable", FALSE,
                      "visible", TRUE,
                      NULL);

  frame = phosh_notification_frame_new ();
  phosh_notification_frame_bind_model (PHOSH_NOTIFICATION_FRAME (frame), item);

  gtk_widget_show (frame);

  gtk_container_add (GTK_CONTAINER (row), frame);

  return row;
}


static void
end_notify_feedback (PhoshSettings *self)
{
  if (lfb_event_get_state (self->notify_event) == LFB_EVENT_STATE_RUNNING)
    lfb_event_end_feedback_async (self->notify_event, NULL, NULL, NULL);
}


static void
on_notifcation_items_changed (PhoshSettings *self,
                              guint          position,
                              guint          removed,
                              guint          added,
                              GListModel    *list)
{
  gboolean is_empty;

  g_return_if_fail (PHOSH_IS_SETTINGS (self));
  g_return_if_fail (G_IS_LIST_MODEL (list));

  is_empty = !g_list_model_get_n_items (list);
  g_debug("Notification list empty: %d", is_empty);

  gtk_widget_set_visible (GTK_WIDGET (self->sw_notifications), !is_empty);
  if (is_empty) {
    g_signal_emit (self, signals[SETTING_DONE], 0);
    end_notify_feedback (self);
  } else if (phosh_shell_get_locked (phosh_shell_get_default ())) {
    if (lfb_event_get_state (self->notify_event) != LFB_EVENT_STATE_RUNNING)
      lfb_event_trigger_feedback_async (self->notify_event, NULL, NULL, NULL);
  }
}


static void
setup_brightness_range (PhoshSettings *self)
{
  gtk_range_set_range (GTK_RANGE (self->scale_brightness), 0, 100);
  gtk_range_set_round_digits (GTK_RANGE (self->scale_brightness), 0);
  gtk_range_set_increments (GTK_RANGE (self->scale_brightness), 1, 10);
  brightness_init (GTK_SCALE (self->scale_brightness));
  g_signal_connect (self->scale_brightness,
                    "value-changed",
                    G_CALLBACK(brightness_value_changed_cb),
                    NULL);
}


static void
setup_volume_bar (PhoshSettings *self)
{
  GtkAdjustment *adj;

  self->output_vol_bar = gvc_channel_bar_new ();
  gtk_widget_set_sensitive (self->output_vol_bar, TRUE);
  gtk_widget_show (self->output_vol_bar);

  gtk_box_pack_start (GTK_BOX (self->box_settings), self->output_vol_bar, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (self->box_settings), self->output_vol_bar, 1);

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
}


static void
phosh_settings_constructed (GObject *object)
{
  PhoshSettings *self = PHOSH_SETTINGS (object);
  PhoshNotifyManager *manager;

  setup_brightness_range (self);
  setup_volume_bar (self);

  g_signal_connect (self->quick_settings,
                    "child-activated",
                    G_CALLBACK (on_quicksetting_activated),
                    self);

  manager = phosh_notify_manager_get_default ();
  gtk_list_box_bind_model (GTK_LIST_BOX (self->list_notifications),
                           G_LIST_MODEL (phosh_notify_manager_get_list (manager)),
                           create_notification_row,
                           NULL,
                           NULL);
  g_signal_connect_object (phosh_notify_manager_get_list (manager),
                           "items-changed",
                           G_CALLBACK (on_notifcation_items_changed),
                           self,
                           G_CONNECT_SWAPPED);
  on_notifcation_items_changed (self, -1, -1, -1,
                                G_LIST_MODEL (phosh_notify_manager_get_list (manager)));

  G_OBJECT_CLASS (phosh_settings_parent_class)->constructed (object);
}


static void
phosh_settings_dispose (GObject *object)
{
  PhoshSettings *self = PHOSH_SETTINGS (object);

  brightness_dispose ();

  if (self->notify_event) {
    end_notify_feedback (self);
    g_clear_object (&self->notify_event);
  }

  if (self->accelerator_callbacks != NULL) {
    g_hash_table_remove_all (self->accelerator_callbacks);
    g_hash_table_unref (self->accelerator_callbacks);
    self->accelerator_callbacks = NULL;
  }

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

  g_type_ensure (PHOSH_TYPE_BT_INFO);
  g_type_ensure (PHOSH_TYPE_FEEDBACK_INFO);
  g_type_ensure (PHOSH_TYPE_MEDIA_PLAYER);
  g_type_ensure (PHOSH_TYPE_QUICK_SETTING);
  g_type_ensure (PHOSH_TYPE_ROTATE_INFO);
  g_type_ensure (PHOSH_TYPE_TORCH_INFO);

  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, box_settings);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, list_notifications);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, quick_settings);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, scale_brightness);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, sw_notifications);

  gtk_widget_class_bind_template_callback (widget_class, battery_setting_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, bt_setting_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, feedback_setting_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, feedback_setting_long_pressed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_media_player_raised);
  gtk_widget_class_bind_template_callback (widget_class, rotation_setting_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, torch_setting_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, wifi_setting_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, wwan_setting_clicked_cb);
}


static void
phosh_settings_init (PhoshSettings *self)
{
  char *subscribe_accelerators[] = {
    "XF86AudioLowerVolume",
    "XF86AudioRaiseVolume",
    "XF86AudioMute",
  };

  self->notify_event = lfb_event_new ("message-missed-notification");

  gtk_widget_init_template (GTK_WIDGET (self));

  self->keyboard_events = phosh_keyboard_events_new ();

  if (!self->keyboard_events)
    return;

  self->accelerator_callbacks = g_hash_table_new (g_direct_hash, g_direct_equal);

  phosh_keyboard_events_register_keys (self->keyboard_events,
                                       subscribe_accelerators,
                                       G_N_ELEMENTS (subscribe_accelerators));
  g_signal_connect_swapped (self->keyboard_events,
                            "accelerator-activated",
                            G_CALLBACK (accelerator_activated_cb),
                            self);
  g_signal_connect_swapped (self->keyboard_events,
                            "accelerator-grabbed",
                            G_CALLBACK (accelerator_grabbed_cb),
                            self);

}

GtkWidget *
phosh_settings_new (void)
{
  return g_object_new (PHOSH_TYPE_SETTINGS, NULL);
}
