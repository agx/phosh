/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-location-info"

#include "phosh-config.h"

#include "location-info.h"
#include "location-manager.h"

#include "shell.h"

/**
 * PhoshLocationInfo:
 *
 * A widget to display the location service status
 *
 * #PhoshLocationInfo indicates if the location service is active.
 * The widgets container should hide the widget if
 * #PhoshLocationInfo:active is %FALSE.
 */

enum {
  PROP_0,
  PROP_ACTIVE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshLocationInfo {
  PhoshStatusIcon       parent;

  gboolean              active;
  PhoshLocationManager *manager;
};
G_DEFINE_TYPE (PhoshLocationInfo, phosh_location_info, PHOSH_TYPE_STATUS_ICON);


static void
phosh_location_info_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  PhoshLocationInfo *self = PHOSH_LOCATION_INFO (object);

  switch (property_id) {
  case PROP_ACTIVE:
    g_value_set_boolean (value, self->active);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_location_info_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  PhoshLocationInfo *self = PHOSH_LOCATION_INFO (object);

  switch (property_id) {
  case PROP_ACTIVE:
    self->active = g_value_get_boolean (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static gboolean
active_to_icon_name (GBinding     *binding,
                     const GValue *from_value,
                     GValue       *to_value,
                     gpointer      user_data)
{
  gboolean active = g_value_get_boolean (from_value);

  g_value_set_string (to_value, active ? "location-services-active-symbolic" :
                      "location-services-disabled-symbolic");
  return TRUE;
}


static void
phosh_location_info_constructed (GObject *object)
{
  PhoshLocationInfo *self = PHOSH_LOCATION_INFO (object);
  PhoshShell *shell = phosh_shell_get_default ();;

  self->manager = g_object_ref (phosh_shell_get_location_manager (shell));

  g_object_bind_property (self->manager, "active",
                          self,"active",
                          G_BINDING_SYNC_CREATE);

  g_object_bind_property_full (self->manager, "active",
                               self,"icon-name",
                               G_BINDING_SYNC_CREATE,
                               active_to_icon_name,
                               NULL, NULL, NULL);

  G_OBJECT_CLASS (phosh_location_info_parent_class)->constructed (object);
}


static void
phosh_location_info_dispose (GObject *object)
{
  PhoshLocationInfo *self = PHOSH_LOCATION_INFO (object);

  g_clear_object (&self->manager);

  G_OBJECT_CLASS (phosh_location_info_parent_class)->dispose (object);
}


static void
phosh_location_info_class_init (PhoshLocationInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_location_info_constructed;
  object_class->dispose = phosh_location_info_dispose;
  object_class->get_property = phosh_location_info_get_property;
  object_class->set_property = phosh_location_info_set_property;

  gtk_widget_class_set_css_name (widget_class, "phosh-location-info");

  props[PROP_ACTIVE] =
    g_param_spec_boolean ("active",
                          "Active",
                          "Whether the location service is active (in use)",
                          FALSE,
                          G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_location_info_init (PhoshLocationInfo *self)
{
}


GtkWidget *
phosh_location_info_new (void)
{
  return g_object_new (PHOSH_TYPE_LOCATION_INFO, NULL);
}
