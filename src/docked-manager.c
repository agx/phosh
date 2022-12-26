/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-docked-manager"

#include "phosh-config.h"

#include "docked-manager.h"
#include "mode-manager.h"
#include "shell.h"

#define DOCKED_DISABLED_ICON "phone-undocked-symbolic"
#define DOCKED_ENABLED_ICON  "phone-docked-symbolic"

#define PHOC_KEY_MAXIMIZE "auto-maximize"
#define A11Y_KEY_OSK "screen-keyboard-enabled"
#define WM_KEY_LAYOUT "button-layout"
#define GTK_KEY_IS_PHONE "is-phone"

/**
 * PhoshDockedManager:
 *
 * Handles 'docking" the phone to additional hardware
 *
 * #PhoshDockedManager allows to dock the phone to additional hardware
 * and performs the necessary configuration changes
 * (disable OSK, don't maximize windows by default, ...)
 */

enum {
  PROP_0,
  PROP_MODE_MANAGER,
  PROP_ICON_NAME,
  PROP_ENABLED,
  PROP_CAN_DOCK,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshDockedManager {
  GObject           parent;

  gboolean          enabled;
  gboolean          can_dock;
  const char       *icon_name;

  PhoshModeManager *mode_manager;

  GSettings        *phoc_settings;
  GSettings        *wm_settings;
  GSettings        *a11y_settings;
  GSettings        *gtk_settings;
  GSettings        *gtk4_settings;
};
G_DEFINE_TYPE (PhoshDockedManager, phosh_docked_manager, G_TYPE_OBJECT);


static void
phosh_docked_manager_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  PhoshDockedManager *self = PHOSH_DOCKED_MANAGER (object);

  switch (property_id) {
  case PROP_ICON_NAME:
    g_value_set_string (value, self->icon_name);
    break;
  case PROP_CAN_DOCK:
    g_value_set_boolean (value, self->can_dock);
    break;
  case PROP_ENABLED:
    g_value_set_boolean (value, self->enabled);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_docked_manager_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  PhoshDockedManager *self = PHOSH_DOCKED_MANAGER (object);

  switch (property_id) {
  case PROP_MODE_MANAGER:
    /* construct only */
    self->mode_manager = g_value_dup_object (value);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_MODE_MANAGER]);
    break;
  case PROP_ENABLED:
    phosh_docked_manager_set_enabled (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
mode_changed_cb (PhoshDockedManager *self, GParamSpec *pspec, PhoshModeManager *manager)
{
  gboolean can_dock = FALSE;

  g_return_if_fail (PHOSH_IS_DOCKED_MANAGER (self));
  g_return_if_fail (PHOSH_IS_MODE_MANAGER (manager));
  g_return_if_fail (self->mode_manager == manager);

  /*
   * Desktops, laptops and phones with enough external hardware should get floating
   * windows, etc. Unknown hardware is assumed to be a phone.
   */
  if (phosh_mode_manager_get_mimicry (manager) != PHOSH_MODE_DEVICE_TYPE_PHONE &&
      phosh_mode_manager_get_mimicry (manager) != PHOSH_MODE_DEVICE_TYPE_TABLET &&
      phosh_mode_manager_get_mimicry (manager) != PHOSH_MODE_DEVICE_TYPE_UNKNOWN)
    can_dock = TRUE;

  if (can_dock == self->can_dock)
    return;

  g_debug ("Docked mode possible: %d", can_dock);
  self->can_dock = can_dock;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CAN_DOCK]);

  /* Automatically enable/disable docked mode */
  phosh_docked_manager_set_enabled (self, can_dock);
}


static void
phosh_docked_manager_constructed (GObject *object)
{
  PhoshDockedManager *self = PHOSH_DOCKED_MANAGER (object);
  GSettingsSchemaSource *schema_source = g_settings_schema_source_get_default();
  g_autoptr (GSettingsSchema) schema = NULL;
  g_autoptr (GSettingsSchema) gtk4_schema = NULL;


  G_OBJECT_CLASS (phosh_docked_manager_parent_class)->constructed (object);

  self->phoc_settings = g_settings_new ("sm.puri.phoc");
  self->a11y_settings = g_settings_new ("org.gnome.desktop.a11y.applications");
  self->wm_settings = g_settings_new ("org.gnome.desktop.wm.preferences");

  /* This is a downstream only schema that is supposed to go away once GTK
   * GTK is adaptive (https://source.puri.sm/Librem5/gtk/-/merge_requests/18) */
  schema = g_settings_schema_source_lookup (schema_source, "org.gtk.Settings.Purism", TRUE);
  if (schema && g_settings_schema_has_key (schema, GTK_KEY_IS_PHONE))
    self->gtk_settings = g_settings_new ("org.gtk.Settings.Purism");

  gtk4_schema = g_settings_schema_source_lookup (schema_source, "org.gtk.gtk4.Settings.Purism", TRUE);
  if (gtk4_schema && g_settings_schema_has_key (gtk4_schema, GTK_KEY_IS_PHONE))
    self->gtk4_settings = g_settings_new ("org.gtk.gtk4.Settings.Purism");

  g_object_connect (
    self->mode_manager,
    "swapped-object-signal::notify::device-type", G_CALLBACK (mode_changed_cb), self,
    "swapped-object-signal::notify::mimicry", G_CALLBACK (mode_changed_cb), self,
    NULL);
  mode_changed_cb (self, NULL, self->mode_manager);
}


static void
phosh_docked_manager_dispose (GObject *object)
{
  PhoshDockedManager *self = PHOSH_DOCKED_MANAGER (object);

  g_clear_object (&self->phoc_settings);
  g_clear_object (&self->a11y_settings);
  g_clear_object (&self->wm_settings);
  g_clear_object (&self->gtk_settings);
  g_clear_object (&self->gtk4_settings);
  g_clear_object (&self->mode_manager);

  G_OBJECT_CLASS (phosh_docked_manager_parent_class)->dispose (object);
}


static void
phosh_docked_manager_class_init (PhoshDockedManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_docked_manager_constructed;
  object_class->dispose = phosh_docked_manager_dispose;
  object_class->get_property = phosh_docked_manager_get_property;
  object_class->set_property = phosh_docked_manager_set_property;

  props[PROP_MODE_MANAGER] =
    g_param_spec_object ("mode-manager",
                         "Mode manager",
                         "The hw mode object",
                         PHOSH_TYPE_MODE_MANAGER,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_ICON_NAME] =
    g_param_spec_string ("icon-name",
                         "icon name",
                         "The docked icon name",
                         DOCKED_DISABLED_ICON,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_ENABLED] =
    g_param_spec_boolean ("enabled",
                          "enabled",
                          "Whether docked mode is enabled",
                          FALSE,
                          G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS);

  props[PROP_CAN_DOCK] =
    g_param_spec_boolean ("can-dock",
                          "Can dock",
                          "Whether the device can be docked",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_docked_manager_init (PhoshDockedManager *self)
{
  self->icon_name = DOCKED_DISABLED_ICON;
  self->can_dock = -1;
  self->enabled = -1;
}


PhoshDockedManager *
phosh_docked_manager_new (PhoshModeManager *mode_manager)
{
  return PHOSH_DOCKED_MANAGER (g_object_new (PHOSH_TYPE_DOCKED_MANAGER,
                                             "mode-manager", mode_manager,
                                             NULL));
}


const char *
phosh_docked_manager_get_icon_name (PhoshDockedManager *self)
{
  g_return_val_if_fail (PHOSH_IS_DOCKED_MANAGER (self), NULL);

  return self->icon_name;
}


gboolean
phosh_docked_manager_get_enabled (PhoshDockedManager *self)
{
  g_return_val_if_fail (PHOSH_IS_DOCKED_MANAGER (self), FALSE);

  return self->enabled;
}


gboolean
phosh_docked_manager_get_can_dock (PhoshDockedManager *self)
{
  g_return_val_if_fail (PHOSH_IS_DOCKED_MANAGER (self), FALSE);

  return self->can_dock;
}


void
phosh_docked_manager_set_enabled (PhoshDockedManager *self, gboolean enable)
{
  const gchar *icon_name;

  g_return_if_fail (PHOSH_IS_DOCKED_MANAGER (self));
  g_return_if_fail ((enable && self->can_dock) || !enable);

  if (self->enabled == enable)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  self->enabled = enable;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);

  if (enable)
    g_settings_reset (self->wm_settings, WM_KEY_LAYOUT);
  else
    g_settings_set_string (self->wm_settings, WM_KEY_LAYOUT, "appmenu:");

  /* we could bind one boolean property via g_settings_bind but that would spread
   * mode setup over several places */
  g_settings_set_boolean (self->phoc_settings, PHOC_KEY_MAXIMIZE, !enable);
  g_settings_set_boolean (self->a11y_settings, A11Y_KEY_OSK, !enable);
  if (self->gtk_settings)
    g_settings_set_boolean (self->gtk_settings, GTK_KEY_IS_PHONE, !enable);
  if (self->gtk4_settings)
    g_settings_set_boolean (self->gtk4_settings, GTK_KEY_IS_PHONE, !enable);

  /* TODO: Other icons for non phones? */
  icon_name = enable ? DOCKED_ENABLED_ICON : DOCKED_DISABLED_ICON;
  if (icon_name != self->icon_name) {
    self->icon_name = icon_name;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ICON_NAME]);
  }

  g_object_thaw_notify (G_OBJECT (self));

  if (!enable) {
    PhoshShell *shell = phosh_shell_get_default ();
    PhoshMonitor *monitor = phosh_shell_get_builtin_monitor (shell);
    if (monitor)
      phosh_shell_set_primary_monitor (shell, monitor);
  }
  g_debug ("Docked mode %sabled", self->enabled ? "en" : "dis");
}
