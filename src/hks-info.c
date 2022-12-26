/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-hks-info"

#include "phosh-config.h"

#include "shell.h"
#include "hks-info.h"
#include "hks-manager.h"

/**
 * PhoshHksInfo:
 *
 * A widget to display the HKS status of a device
 *
 * #PhoshHksInfo displays whether a device is disabled via a hardware
 * kill switch (HKS).
 */

enum {
  PROP_0,
  PROP_DEV_TYPE,
  PROP_BLOCKED,
  PROP_PRESENT,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


struct _PhoshHksInfo {
  PhoshStatusIcon  parent;

  char            *dev_type;
  gboolean         blocked;
  gboolean         present;
  PhoshHksManager *manager;
};
G_DEFINE_TYPE (PhoshHksInfo, phosh_hks_info, PHOSH_TYPE_STATUS_ICON);


static void
phosh_hks_info_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  PhoshHksInfo *self = PHOSH_HKS_INFO (object);

  switch (property_id) {
  case PROP_BLOCKED:
    g_value_set_boolean (value, self->blocked);
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
phosh_hks_info_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  PhoshHksInfo *self = PHOSH_HKS_INFO (object);

  switch (prop_id) {
  case PROP_DEV_TYPE:
    /* construct only */
    self->dev_type = g_value_dup_string (value);
    break;
  case PROP_BLOCKED:
    self->blocked = g_value_get_boolean (value);
    break;
  case PROP_PRESENT:
    self->present = g_value_get_boolean (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
phosh_hks_info_constructed (GObject *object)
{
  PhoshHksInfo *self = PHOSH_HKS_INFO (object);
  PhoshShell *shell;
  g_autofree gchar *propname = NULL;

  G_OBJECT_CLASS (phosh_hks_info_parent_class)->constructed (object);

  shell = phosh_shell_get_default ();
  self->manager = g_object_ref (phosh_shell_get_hks_manager (shell));

  if (self->manager == NULL) {
    g_warning ("Failed to get hks manager");
    return;
  }

  propname = g_strdup_printf ("%s-present", self->dev_type);
  g_object_bind_property (self->manager,
                          propname,
                          self,
                          "present",
                          G_BINDING_SYNC_CREATE);

  g_free (propname);
  propname = g_strdup_printf ("%s-blocked", self->dev_type);
  g_object_bind_property (self->manager,
                          propname,
                          self,
                          "blocked",
                          G_BINDING_SYNC_CREATE);

  g_free (propname);
  propname = g_strdup_printf ("%s-icon-name", self->dev_type);
  g_object_bind_property (self->manager,
                          propname,
                          self,
                          "icon-name",
                          G_BINDING_SYNC_CREATE);
}


static void
phosh_hks_info_dispose (GObject *object)
{
  PhoshHksInfo *self = PHOSH_HKS_INFO (object);

  g_clear_object (&self->manager);

  G_OBJECT_CLASS (phosh_hks_info_parent_class)->dispose (object);
}


static void
phosh_hks_info_finalize (GObject *object)
{
  PhoshHksInfo *self = PHOSH_HKS_INFO (object);

  g_clear_pointer (&self->dev_type, g_free);

  G_OBJECT_CLASS (phosh_hks_info_parent_class)->finalize (object);
}


static void
phosh_hks_info_class_init (PhoshHksInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_hks_info_constructed;
  object_class->dispose = phosh_hks_info_dispose;
  object_class->finalize = phosh_hks_info_finalize;
  object_class->get_property = phosh_hks_info_get_property;
  object_class->set_property = phosh_hks_info_set_property;

  gtk_widget_class_set_css_name (widget_class, "phosh-hks-info");

  props[PROP_DEV_TYPE] =
    g_param_spec_string ("device-type",
                         "Device type",
                         "The moniored device type",
                         FALSE,
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_CONSTRUCT_ONLY);
  props[PROP_BLOCKED] =
    g_param_spec_boolean ("blocked",
                          "blocked",
                          "Whether the device is blocked",
                          FALSE,
                          G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS);
  props[PROP_PRESENT] =
    g_param_spec_boolean ("present",
                          "Present",
                          "Whether hks hardware is present",
                          FALSE,
                          G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_hks_info_init (PhoshHksInfo *self)
{
  phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), "HKS");
}


GtkWidget *
phosh_hks_info_new (void)
{
  return g_object_new (PHOSH_TYPE_HKS_INFO, NULL);
}
