/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-ambient"

#include "phosh-config.h"
#include "animation.h"
#include "fader.h"
#include "ambient.h"
#include "shell.h"
#include "sensor-proxy-manager.h"
#include "util.h"

#define INTERFACE_SETTINGS      "org.gnome.desktop.interface"
#define HIGH_CONTRAST_THEME     "HighContrast"
#define KEY_GTK_THEME           "gtk-theme"
#define KEY_ICON_THEME          "icon-theme"

#define PHOSH_SETTINGS          "sm.puri.phosh"
#define KEY_AUTOMATIC_HC        "automatic-high-contrast"
#define KEY_AUTOMATIC_HC_THRESHOLD  "automatic-high-contrast-threshold"

#define NUM_VALUES              3

/**
 * PhoshAmbient:
 *
 * Ambient light sensor handling
 *
 * #PhoshAmbient handles enabling and disabling the ambient detection
 * based and toggles related actions.
 */

enum {
  PROP_0,
  PROP_SENSOR_PROXY_MANAGER,
  LAST_PROP,
};
static GParamSpec *props[LAST_PROP];


typedef struct _PhoshAmbient {
  GObject                  parent;

  gboolean                 claimed;
  PhoshSensorProxyManager *sensor_proxy_manager;
  GCancellable            *cancel;

  GSettings               *phosh_settings;
  GSettings               *interface_settings;
  gboolean                 use_hc;

  guint                    sample_id;
  GArray                  *values;

  PhoshFader              *fader;
  guint                    fader_id;
} PhoshAmbient;

G_DEFINE_TYPE (PhoshAmbient, phosh_ambient, G_TYPE_OBJECT);


static void
phosh_ambient_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  PhoshAmbient *self = PHOSH_AMBIENT (object);

  switch (property_id) {
  case PROP_SENSOR_PROXY_MANAGER:
    /* construct only */
    self->sensor_proxy_manager = g_value_dup_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_ambient_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  PhoshAmbient *self = PHOSH_AMBIENT (object);

  switch (property_id) {
  case PROP_SENSOR_PROXY_MANAGER:
    g_value_set_object (value, self->sensor_proxy_manager);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static gboolean
on_fade_in_done (PhoshAmbient *self)
{
  if (self->use_hc) {
    g_settings_set_string (self->interface_settings, KEY_GTK_THEME, HIGH_CONTRAST_THEME);
  } else {
    g_settings_reset (self->interface_settings, KEY_GTK_THEME);
    g_settings_reset (self->interface_settings, KEY_ICON_THEME);
  }

  phosh_fader_hide (self->fader);
  return G_SOURCE_REMOVE;
}


static void
switch_theme (PhoshAmbient *self, gboolean use_hc)
{
  const char *style_class;

  if (use_hc == self->use_hc)
    return;

  style_class = use_hc ? "phosh-fader-theme-to-hc" : "phosh-fader-theme-from-hc";
  self->fader = g_object_new (PHOSH_TYPE_FADER,
                              "style-class", style_class,
                              "fade-out-time", 1500,
                              "fade-out-type", PHOSH_ANIMATION_TYPE_EASE_IN_QUINTIC,
                              NULL);
  gtk_widget_show (GTK_WIDGET (self->fader));

  self->fader_id = g_timeout_add (100 * PHOSH_ANIMATION_SLOWDOWN,
                                  G_SOURCE_FUNC (on_fade_in_done),
                                  self);
  g_source_set_name_by_id (self->fader_id, "[phosh] ambient fader");

  self->use_hc = use_hc;
}


static gboolean
on_ambient_light_level_sample (gpointer data)
{
  PhoshAmbient *self = PHOSH_AMBIENT (data);
  double level, threshold;
  double avg = 0.0;
  gboolean use_hc;

  level = phosh_dbus_sensor_proxy_get_light_level (PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager));
  g_array_append_val (self->values, level);

  if (self->values->len < NUM_VALUES)
    return G_SOURCE_CONTINUE;

  for (int i = 0; i < self->values->len; i++)
    avg += g_array_index (self->values, double, i);

  avg /= self->values->len;
  threshold = g_settings_get_uint (self->phosh_settings, KEY_AUTOMATIC_HC_THRESHOLD);
  use_hc = avg > threshold;

  g_debug ("Avg: %f Switching theme to hc: %d", avg, use_hc);
  switch_theme (self, use_hc);

  g_array_set_size (self->values, 0);
  self->sample_id = 0;
  return G_SOURCE_REMOVE;
}


static void
on_ambient_light_level_changed (PhoshAmbient            *self,
                                GParamSpec              *pspec,
                                PhoshSensorProxyManager *sensor)
{
  double level, hyst, threshold;
  const char *unit;
  gboolean wants_hc;

  if (!self->claimed)
    return;

  /* Currently sampling, ignoring changes */
  if (self->sample_id)
    return;

  threshold = g_settings_get_uint (self->phosh_settings, KEY_AUTOMATIC_HC_THRESHOLD);
  level = phosh_dbus_sensor_proxy_get_light_level (PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager));
  unit = phosh_dbus_sensor_proxy_get_light_level_unit (PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager));

  g_debug ("Ambient light changed: %f %s", level, unit);

  if (g_ascii_strcasecmp (unit , "lux") != 0) {
    /* For vendor values we don't know if small or large values mean bright or dark so be conservative */
    g_warning_once ("Unknown unit light level unit %s", unit);
    return;
  }

  /* Use a bit of hysteresis to not switch too often around the threshold */
  hyst = (self->use_hc) ? 0.9 : 1.1;
  threshold *= hyst;

  wants_hc = level > threshold;
  /* New value wouldn't change anything, nothing to do */
  if (wants_hc == self->use_hc)
    return;

  /* new value would change hc mode, sample to see if it should stick */
  g_return_if_fail (self->sample_id == 0);
  g_return_if_fail (self->values->len == 0);
  g_array_append_val (self->values, level);
  self->sample_id = g_timeout_add_seconds (1, on_ambient_light_level_sample, self);
  g_source_set_name_by_id (self->sample_id, "[phosh] ambient_sample");
}


static void
on_ambient_claimed (PhoshSensorProxyManager *sensor_proxy_manager,
                    GAsyncResult            *res,
                    PhoshAmbient            *self)
{
  g_autoptr (GError) err = NULL;
  gboolean success;

  success = phosh_dbus_sensor_proxy_call_claim_light_finish (
    PHOSH_DBUS_SENSOR_PROXY (sensor_proxy_manager),
    res, &err);

  if (!success) {
    g_warning ("Failed to claim ambient sensor: %s", err->message);
    return;
  }

  g_debug ("Claimed ambient sensor");
  self->claimed = TRUE;

  on_ambient_light_level_changed (self, NULL, self->sensor_proxy_manager);
}


static void
on_ambient_released (PhoshSensorProxyManager *sensor_proxy_manager,
                     GAsyncResult            *res,
                     PhoshAmbient            *self)
{
  g_autoptr (GError) err = NULL;
  gboolean success;

  g_return_if_fail (PHOSH_IS_SENSOR_PROXY_MANAGER (sensor_proxy_manager));
  g_return_if_fail (sensor_proxy_manager == self->sensor_proxy_manager);

  success = phosh_dbus_sensor_proxy_call_release_light_finish (
    PHOSH_DBUS_SENSOR_PROXY (sensor_proxy_manager),
    res, &err);

  if (!success) {
    g_warning ("Failed to release ambient sensor: %s", err->message);
    return;
  }

  g_debug ("Released ambient light sensor");
  self->claimed = FALSE;
}


static void
phosh_ambient_claim_light (PhoshAmbient *self, gboolean claim)
{
  if (claim == self->claimed)
    return;

  if (claim) {
    phosh_dbus_sensor_proxy_call_claim_light (
      PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager),
      self->cancel,
      (GAsyncReadyCallback)on_ambient_claimed,
      self);
  } else {
    g_clear_handle_id (&self->sample_id, g_source_remove);
    g_array_set_size (self->values, 0);
    phosh_dbus_sensor_proxy_call_release_light (
      PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager),
      self->cancel,
      (GAsyncReadyCallback)on_ambient_released,
      self);
  }
}


static void
on_automatic_high_contrast_changed (PhoshAmbient *self,
                                    GParamSpec   *pspec,
                                    GSettings    *settings)
{
  gboolean enable;

  enable = g_settings_get_boolean (self->phosh_settings, KEY_AUTOMATIC_HC);

  if (enable) {
    if (self->claimed)
      on_ambient_light_level_changed (self, NULL, self->sensor_proxy_manager);
    else
      phosh_ambient_claim_light (self, TRUE);
  } else {
    phosh_ambient_claim_light (self, FALSE);
  }
}


static void
on_has_ambient_light_changed (PhoshAmbient            *self,
                              GParamSpec              *pspec,
                              PhoshSensorProxyManager *proxy)
{
  gboolean has_ambient;

  has_ambient = phosh_dbus_sensor_proxy_get_has_ambient_light (
    PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager));

  g_debug ("Found %s ambient sensor", has_ambient ? "a" : "no");

  on_automatic_high_contrast_changed (self, NULL, self->phosh_settings);
}


static void
on_shell_state_changed (PhoshAmbient  *self,
                        GParamSpec    *pspec,
                        PhoshShell    *shell)
{
  PhoshShellStateFlags state;

  g_return_if_fail (PHOSH_IS_AMBIENT (self));
  g_return_if_fail (PHOSH_IS_SHELL (shell));

  state = phosh_shell_get_state (shell);
  g_debug ("Shell state changed: %d", state);
  /* Claim/unclaim the sensor on screen unblank / blank */
  if (state & PHOSH_STATE_BLANKED) {
    phosh_ambient_claim_light (self, FALSE);
  } else {
    on_has_ambient_light_changed (self, NULL, self->sensor_proxy_manager);
  }
}


static void
phosh_ambient_constructed (GObject *object)
{
  PhoshAmbient *self = PHOSH_AMBIENT (object);

  G_OBJECT_CLASS (phosh_ambient_parent_class)->constructed (object);

  g_object_connect (self->sensor_proxy_manager,
                    "swapped-signal::notify::light-level",
                    on_ambient_light_level_changed,
                    self,
                    "swapped-signal::notify::has-ambient-light",
                    on_has_ambient_light_changed,
                    self,
                    NULL);

  g_object_connect (self->phosh_settings,
                    "swapped-signal::changed::" KEY_AUTOMATIC_HC,
                    G_CALLBACK (on_automatic_high_contrast_changed),
                    self,
                    "swapped-signal::changed::" KEY_AUTOMATIC_HC_THRESHOLD,
                    G_CALLBACK (on_automatic_high_contrast_changed),
                    self,
                    NULL);

  g_signal_connect_object (phosh_shell_get_default (),
                           "notify::shell-state",
                           G_CALLBACK (on_shell_state_changed),
                           self,
                           G_CONNECT_SWAPPED);

  on_has_ambient_light_changed (self, NULL, self->sensor_proxy_manager);
}


static void
phosh_ambient_dispose (GObject *object)
{
  PhoshAmbient *self = PHOSH_AMBIENT (object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  g_clear_handle_id (&self->sample_id, g_source_remove);
  g_clear_pointer (&self->values, g_array_unref);

  if (self->sensor_proxy_manager) {
    g_signal_handlers_disconnect_by_data (self->sensor_proxy_manager, self);
    phosh_dbus_sensor_proxy_call_release_light_sync (
      PHOSH_DBUS_SENSOR_PROXY (self->sensor_proxy_manager), NULL, NULL);
    g_clear_object (&self->sensor_proxy_manager);
  }

  g_clear_object (&self->phosh_settings);
  g_clear_object (&self->interface_settings);

  g_clear_handle_id (&self->fader_id, g_source_remove);
  g_clear_pointer (&self->fader, phosh_cp_widget_destroy);

  G_OBJECT_CLASS (phosh_ambient_parent_class)->dispose (object);
}


static void
phosh_ambient_class_init (PhoshAmbientClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = phosh_ambient_constructed;
  object_class->dispose = phosh_ambient_dispose;

  object_class->set_property = phosh_ambient_set_property;
  object_class->get_property = phosh_ambient_get_property;

  props[PROP_SENSOR_PROXY_MANAGER] =
    g_param_spec_object ("sensor-proxy-manager", "", "",
                         PHOSH_TYPE_SENSOR_PROXY_MANAGER,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);

}


static void
phosh_ambient_init (PhoshAmbient *self)
{
  g_autofree char *theme_name = NULL;

  self->cancel = g_cancellable_new ();

  self->values = g_array_new (FALSE, FALSE, sizeof(double));

  self->interface_settings = g_settings_new (INTERFACE_SETTINGS);
  self->phosh_settings = g_settings_new (PHOSH_SETTINGS);

  /* Check whether we're already using the hc theme */
  theme_name = g_settings_get_string (self->interface_settings, KEY_GTK_THEME);
  if (g_strcmp0 (theme_name, HIGH_CONTRAST_THEME) == 0)
    self->use_hc = TRUE;
}


PhoshAmbient *
phosh_ambient_new (PhoshSensorProxyManager *sensor_proxy_manager)
{
  return g_object_new (PHOSH_TYPE_AMBIENT,
                       "sensor-proxy-manager", sensor_proxy_manager,
                       NULL);
}
