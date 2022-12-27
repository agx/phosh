/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-docked-info"

#include "phosh-config.h"

#include "shell.h"
#include "docked-info.h"
#include "docked-manager.h"

/**
 * PhoshDockedInfo:
 *
 * A widget to display the docked status
 *
 * #PhoshDockedInfo displays whether the device is docked
 */

enum {
  PROP_0,
  PROP_ENABLED,
  PROP_PRESENT,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


struct _PhoshDockedInfo {
  PhoshStatusIcon     parent;

  gboolean            enabled;
  gboolean            present;
  PhoshDockedManager *manager;
};
G_DEFINE_TYPE (PhoshDockedInfo, phosh_docked_info, PHOSH_TYPE_STATUS_ICON);


static void
phosh_docked_info_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PhoshDockedInfo *self = PHOSH_DOCKED_INFO (object);

  switch (property_id) {
  case PROP_ENABLED:
    g_value_set_boolean (value, self->enabled);
    break;
  case PROP_PRESENT:
    g_value_set_boolean (value, self->present);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
on_docked_mode_enabled (PhoshDockedInfo *self, GParamSpec *pspec, PhoshDockedManager *manager)
{
  gboolean enabled;

  g_debug ("Updating docked status");
  g_return_if_fail (PHOSH_IS_DOCKED_INFO (self));
  g_return_if_fail (PHOSH_IS_DOCKED_MANAGER (manager));

  enabled = phosh_docked_manager_get_enabled (manager);
  if (self->enabled == enabled)
    return;

  self->enabled = enabled;
  phosh_status_icon_set_info (PHOSH_STATUS_ICON (self),
                              enabled ? _("Docked") : _("Undocked"));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);
}


static void
on_docked_present (PhoshDockedInfo *self, GParamSpec *pspec, PhoshDockedManager *manager)
{
  gboolean can_dock;

  g_return_if_fail (PHOSH_IS_DOCKED_INFO (self));
  g_return_if_fail (PHOSH_IS_DOCKED_MANAGER (manager));

  can_dock = phosh_docked_manager_get_can_dock (manager);
  if (self->present == can_dock)
    return;

  self->present = can_dock;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRESENT]);
}


static void
phosh_docked_info_idle_init (PhoshStatusIcon *icon)
{
  PhoshDockedInfo *self = PHOSH_DOCKED_INFO (icon);

  g_object_bind_property (self->manager, "icon-name", self, "icon-name",
                          G_BINDING_SYNC_CREATE);

  /* We don't use a binding for self->enabled so we can keep
     the property r/o */
  g_signal_connect_swapped (self->manager,
                            "notify::can-dock",
                            G_CALLBACK (on_docked_present),
                            self);
  on_docked_present (self, NULL, self->manager);

  g_signal_connect_swapped (self->manager,
                            "notify::enabled",
                            G_CALLBACK (on_docked_mode_enabled),
                            self);
  on_docked_mode_enabled (self, NULL, self->manager);
}


static void
phosh_docked_info_constructed (GObject *object)
{
  PhoshDockedInfo *self = PHOSH_DOCKED_INFO (object);
  PhoshShell *shell;

  G_OBJECT_CLASS (phosh_docked_info_parent_class)->constructed (object);

  shell = phosh_shell_get_default ();
  self->manager = g_object_ref (phosh_shell_get_docked_manager (shell));

  if (self->manager == NULL) {
    g_warning ("Failed to get docked manager");
    return;
  }
}


static void
phosh_docked_info_dispose (GObject *object)
{
  PhoshDockedInfo *self = PHOSH_DOCKED_INFO (object);

  if (self->manager) {
    g_signal_handlers_disconnect_by_data (self->manager, self);
    g_clear_object (&self->manager);
  }

  G_OBJECT_CLASS (phosh_docked_info_parent_class)->dispose (object);
}


static void
phosh_docked_info_class_init (PhoshDockedInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhoshStatusIconClass *status_icon_class = PHOSH_STATUS_ICON_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_docked_info_constructed;
  object_class->dispose = phosh_docked_info_dispose;
  object_class->get_property = phosh_docked_info_get_property;

  status_icon_class->idle_init = phosh_docked_info_idle_init;

  gtk_widget_class_set_css_name (widget_class, "phosh-docked-info");

  props[PROP_ENABLED] =
    g_param_spec_boolean ("enabled",
                          "enabled",
                          "Whether the docked is enabled",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);
  props[PROP_PRESENT] =
    g_param_spec_boolean ("present",
                          "Present",
                          "Whether docked hardware is present",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_docked_info_init (PhoshDockedInfo *self)
{
  phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), _("Undocked"));
}


GtkWidget *
phosh_docked_info_new (void)
{
  return g_object_new (PHOSH_TYPE_DOCKED_INFO, NULL);
}
