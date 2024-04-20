/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-audio-settings"

#include "phosh-config.h"

#include <pulse/pulseaudio.h>

#include "fading-label.h"
#include "settings/audio-device-row.h"
#include "settings/audio-device.h"
#include "settings/audio-devices.h"
#include "settings/audio-settings.h"
#include "settings/gvc-channel-bar.h"
#include "util.h"

#include "gvc-mixer-control.h"
#include "gvc-mixer-stream.h"

#include <gmobile.h>

#include <glib/gi18n.h>

#include <math.h>

/**
 * PhoshAudioSettings:
 *
 * Widget to conrol Audio device selection and volume.
 */

enum {
  PROP_0,
  PROP_IS_HEADPHONE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshAudioSettings {
  GtkBin             parent;

  GvcMixerControl   *mixer_control;

  /* Volume slider */
  GvcMixerStream    *output_stream;
  gboolean           allow_volume_above_100_percent;
  gboolean           setting_volume;
  gboolean           is_headphone;
  GtkWidget         *output_vol_bar;

  /* Device select */
  GtkWidget         *stack_audio_details;
  GtkWidget         *toggle_audio_details;
  GtkWidget         *box_audio_input_devices;
  GtkWidget         *box_audio_output_devices;
  GtkWidget         *listbox_audio_input_devices;
  GtkWidget         *listbox_audio_output_devices;
  PhoshAudioDevices *audio_input_devices;
  PhoshAudioDevices *audio_output_devices;
};
G_DEFINE_TYPE (PhoshAudioSettings, phosh_audio_settings, GTK_TYPE_BIN)


static void
update_output_vol_bar (PhoshAudioSettings *self)
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
  PhoshAudioSettings *self = PHOSH_AUDIO_SETTINGS (data);
  gboolean muted;

  muted = gvc_mixer_stream_get_is_muted (stream);
  if (!self->setting_volume) {
    gvc_channel_bar_set_is_muted (GVC_CHANNEL_BAR (self->output_vol_bar), muted);
    if (!muted)
      update_output_vol_bar (self);
  }
}


static void
output_stream_notify_volume_cb (GvcMixerStream *stream, GParamSpec *pspec, gpointer data)
{
  PhoshAudioSettings *self = PHOSH_AUDIO_SETTINGS (data);

  if (!self->setting_volume)
    update_output_vol_bar (self);
}


static gboolean
stream_uses_headphones (GvcMixerStream *stream)
{
  const char *form_factor;
  const GvcMixerStreamPort *port;

  form_factor = gvc_mixer_stream_get_form_factor (stream);
  if (g_strcmp0 (form_factor, "headset") == 0 ||
      g_strcmp0 (form_factor, "headphone") == 0) {
    return TRUE;
  }

  port = gvc_mixer_stream_get_port (stream);
  if (!port)
    return FALSE;

  if (g_strcmp0 (port->port, "[Out] Headphones") == 0 ||
      g_strcmp0 (port->port, "analog-output-headphones") == 0) {
    return TRUE;
  }

  return FALSE;
}


static void
on_output_stream_port_changed (GvcMixerStream *stream, GParamSpec *pspec, gpointer data)
{
  PhoshAudioSettings *self = PHOSH_AUDIO_SETTINGS (data);
  const char *icon = NULL;
  gboolean is_headphone = FALSE;
  const GvcMixerStreamPort *port;

  port = gvc_mixer_stream_get_port (stream);
  if (port)
    g_debug ("Port changed: %s (%s)", port->human_port ?: port->port, port->port);

  is_headphone = stream_uses_headphones (stream);
  if (is_headphone) {
    icon = "audio-headphones";
  } else {
    GvcMixerUIDevice *output;

    output = gvc_mixer_control_lookup_device_from_stream (self->mixer_control, stream);
    if (output)
      icon = gvc_mixer_ui_device_get_icon_name (output);
  }

  if (gm_str_is_null_or_empty (icon) || g_str_has_prefix (icon, "audio-card"))
    icon = "audio-speakers";

  gvc_channel_bar_set_icon_name (GVC_CHANNEL_BAR (self->output_vol_bar), icon);

  if (is_headphone == self->is_headphone)
    return;
  self->is_headphone = is_headphone;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_IS_HEADPHONE]);
}


static void
mixer_control_output_update_cb (GvcMixerControl *mixer, guint id, gpointer data)
{
  PhoshAudioSettings *self = PHOSH_AUDIO_SETTINGS (data);

  g_debug ("Audio output updated: %d", id);

  g_return_if_fail (PHOSH_IS_AUDIO_SETTINGS (self));

  if (self->output_stream)
    g_signal_handlers_disconnect_by_data (self->output_stream, self);

  g_set_object (&self->output_stream, gvc_mixer_control_get_default_sink (self->mixer_control));
  g_return_if_fail (self->output_stream);

  g_signal_connect_object (self->output_stream,
                           "notify::volume",
                           G_CALLBACK (output_stream_notify_volume_cb),
                           self, 0);

  g_signal_connect_object (self->output_stream,
                           "notify::is-muted",
                           G_CALLBACK (output_stream_notify_is_muted_cb),
                           self, 0);

  g_signal_connect_object (self->output_stream,
                           "notify::port",
                           G_CALLBACK (on_output_stream_port_changed),
                           self, 0);
  on_output_stream_port_changed (self->output_stream, NULL, self);

  update_output_vol_bar (self);
}


static void
vol_bar_value_changed_cb (GvcChannelBar *bar, PhoshAudioSettings *self)
{
  double volume, rounded;
  g_autofree char *name = NULL;

  if (!self->output_stream)
    self->output_stream = g_object_ref (gvc_mixer_control_get_default_sink (self->mixer_control));

  volume = gvc_channel_bar_get_volume (bar);
  rounded = round (volume);

  g_object_get (self->output_vol_bar, "name", &name, NULL);
  g_debug ("Setting stream volume %lf (rounded: %lf) for bar '%s'", volume, rounded, name);

  g_return_if_fail (self->output_stream);
  if (gvc_mixer_stream_set_volume (self->output_stream, (pa_volume_t) rounded) != FALSE)
    gvc_mixer_stream_push_volume (self->output_stream);

  gvc_mixer_stream_change_is_muted (self->output_stream, (int) rounded == 0);
}


static void
on_audio_input_device_row_activated (PhoshAudioSettings  *self,
                                     PhoshAudioDeviceRow *row,
                                     GtkListBox          *list)
{
  PhoshAudioDevice *audio_device = phosh_audio_device_row_get_audio_device (row);
  GvcMixerUIDevice *device;
  guint id;

  g_return_if_fail (PHOSH_IS_AUDIO_DEVICE (audio_device));
  id = phosh_audio_device_get_id (audio_device);
  device = gvc_mixer_control_lookup_input_id (self->mixer_control, id);
  gvc_mixer_control_change_input (self->mixer_control, device);
}


static void
on_audio_output_device_row_activated (PhoshAudioSettings  *self,
                                      PhoshAudioDeviceRow *row,
                                      GtkListBox          *list)
{
  PhoshAudioDevice *audio_device = phosh_audio_device_row_get_audio_device (row);
  GvcMixerUIDevice *device;
  guint id;

  g_return_if_fail (PHOSH_IS_AUDIO_DEVICE (audio_device));
  id = phosh_audio_device_get_id (audio_device);
  device = gvc_mixer_control_lookup_output_id (self->mixer_control, id);
  gvc_mixer_control_change_output (self->mixer_control, device);
}


static GtkWidget *
create_audio_device_row (gpointer item, gpointer user_data)
{
  PhoshAudioDevice *audio_device = PHOSH_AUDIO_DEVICE (item);

  return GTK_WIDGET (phosh_audio_device_row_new (audio_device));
}


static void
phosh_audio_settings_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  PhoshAudioSettings *self = PHOSH_AUDIO_SETTINGS (object);

  switch (property_id) {
  case PROP_IS_HEADPHONE:
    g_value_set_boolean (value, phosh_audio_settings_get_output_is_headphone (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_audio_settings_dispose (GObject *object)
{
  PhoshAudioSettings *self = PHOSH_AUDIO_SETTINGS (object);

  g_clear_object (&self->output_stream);
  g_clear_object (&self->audio_output_devices);
  g_clear_object (&self->audio_input_devices);

  G_OBJECT_CLASS (phosh_audio_settings_parent_class)->dispose (object);
}


static void
phosh_audio_settings_finalize (GObject *object)
{
  PhoshAudioSettings *self = PHOSH_AUDIO_SETTINGS (object);

  g_clear_object (&self->mixer_control);

  G_OBJECT_CLASS (phosh_audio_settings_parent_class)->finalize (object);
}


static void
phosh_audio_settings_class_init (PhoshAudioSettingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_audio_settings_get_property;
  object_class->dispose = phosh_audio_settings_dispose;
  object_class->finalize = phosh_audio_settings_finalize;

  /**
   * PhoshAudioSettings:is-headphone:
   *
   * Whether the current output is a headphone
   */
  props[PROP_IS_HEADPHONE] =
    g_param_spec_boolean ("is-headphone", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  g_type_ensure (GVC_TYPE_CHANNEL_BAR);
  g_type_ensure (PHOSH_TYPE_FADING_LABEL);

  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/phosh/ui/audio-settings.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshAudioSettings, box_audio_input_devices);
  gtk_widget_class_bind_template_child (widget_class, PhoshAudioSettings, box_audio_output_devices);
  gtk_widget_class_bind_template_child (widget_class, PhoshAudioSettings, listbox_audio_input_devices);
  gtk_widget_class_bind_template_child (widget_class, PhoshAudioSettings, listbox_audio_output_devices);
  gtk_widget_class_bind_template_child (widget_class, PhoshAudioSettings, output_vol_bar);
  gtk_widget_class_bind_template_child (widget_class, PhoshAudioSettings, stack_audio_details);
  gtk_widget_class_bind_template_child (widget_class, PhoshAudioSettings, toggle_audio_details);

  gtk_widget_class_bind_template_callback (widget_class, on_audio_input_device_row_activated);
  gtk_widget_class_bind_template_callback (widget_class, on_audio_output_device_row_activated);

  gtk_widget_class_set_css_name (widget_class, "phosh-audio-settings");
}


static gboolean
transform_toggle_stack_child_name (GBinding     *binding,
                                   const GValue *from_value,
                                   GValue       *to_value,
                                   gpointer      user_data)
{
  gboolean active = g_value_get_boolean (from_value);

  g_value_set_string (to_value, active ? "audio-details" : "no-audio-details");

  return TRUE;
}


static void
phosh_audio_settings_init (PhoshAudioSettings *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->mixer_control = gvc_mixer_control_new (_("Phone Shell Volume Control"));
  g_return_if_fail (self->mixer_control);

  /* Volume slider */
  gvc_mixer_control_open (self->mixer_control);
  g_signal_connect (self->mixer_control,
                    "active-output-update",
                    G_CALLBACK (mixer_control_output_update_cb),
                    self);
  g_signal_connect (self->output_vol_bar,
                    "value-changed",
                    G_CALLBACK (vol_bar_value_changed_cb),
                    self);

  /* Toggle details button */
  g_object_bind_property_full (self->toggle_audio_details, "active",
                               self->stack_audio_details, "visible-child-name",
                               G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE,
                               transform_toggle_stack_child_name,
                               NULL, NULL, NULL);

  /* Audio device selection */
  self->audio_output_devices = phosh_audio_devices_new (self->mixer_control, FALSE);
  gtk_list_box_bind_model (GTK_LIST_BOX (self->listbox_audio_output_devices),
                           G_LIST_MODEL (self->audio_output_devices),
                           create_audio_device_row,
                           self,
                           NULL);
  g_object_bind_property (self->audio_output_devices, "has-devices",
                          self->box_audio_output_devices, "visible",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  self->audio_input_devices = phosh_audio_devices_new (self->mixer_control, TRUE);
  gtk_list_box_bind_model (GTK_LIST_BOX (self->listbox_audio_input_devices),
                           G_LIST_MODEL (self->audio_input_devices),
                           create_audio_device_row,
                           self,
                           NULL);
  g_object_bind_property (self->audio_input_devices, "has-devices",
                          self->box_audio_input_devices, "visible",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);
}


PhoshAudioSettings *
phosh_audio_settings_new (void)
{
  return g_object_new (PHOSH_TYPE_AUDIO_SETTINGS, NULL);
}


gboolean
phosh_audio_settings_get_output_is_headphone (PhoshAudioSettings *self)
{
  g_return_val_if_fail (PHOSH_IS_AUDIO_SETTINGS (self), FALSE);

  return self->is_headphone;
}

/**
 * phosh_audio_settings_hide_details:
 * @self: The audio settings widget
 *
 * Hides the audio settings details
 */
void
phosh_audio_settings_hide_details (PhoshAudioSettings *self)
{
  g_return_if_fail (PHOSH_IS_AUDIO_SETTINGS (self));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->toggle_audio_details), FALSE);
}
