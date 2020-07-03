/*
 * Copyright (C) 2019 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

/* Bt Info widget */

#define G_LOG_DOMAIN "phosh-bt-info"

#include "config.h"

#include "shell.h"
#include "bt-info.h"
#include "bt-manager.h"

/**
 * SECTION:bt-info
 * @short_description: A widget to display the bluetooth status
 * @Title: PhoshBtInfo
 *
 * #PhoshBtInfo displays the current bluetooth status based on information
 * from #PhoshBtManager. To figure out if the widget should be shown
 * the #PhoshBtInfo::enabled property can be useful.
 */

enum {
  PROP_0,
  PROP_ENABLED,
  PROP_PRESENT,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshBtInfo {
  PhoshStatusIcon parent;

  gboolean        enabled;
  gboolean        present;
  PhoshBtManager *bt;
};
G_DEFINE_TYPE (PhoshBtInfo, phosh_bt_info, PHOSH_TYPE_STATUS_ICON);

static void
phosh_bt_info_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  PhoshBtInfo *self = PHOSH_BT_INFO (object);

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
update_icon (PhoshBtInfo *self, GParamSpec *pspec, PhoshBtManager *bt)
{
  const gchar *icon_name;

  g_return_if_fail (PHOSH_IS_BT_INFO (self));
  g_return_if_fail (PHOSH_IS_BT_MANAGER (bt));

  icon_name = phosh_bt_manager_get_icon_name (bt);
  g_debug ("Updating bt icon to %s", icon_name);
  if (icon_name)
    phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), icon_name);
}

static void
update_info (PhoshBtInfo *self)
{
  gboolean enabled;

  g_return_if_fail (PHOSH_IS_BT_INFO (self));

  /* TODO: show number of paired devices */
  enabled = phosh_bt_manager_get_enabled (self->bt);
  if (enabled)
    phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), _("On"));
  else
    phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), _("Bluetooth"));
}

static void
on_bt_enabled (PhoshBtInfo *self, GParamSpec *pspec, PhoshBtManager *bt)
{
  gboolean enabled;

  g_return_if_fail (PHOSH_IS_BT_INFO (self));
  g_return_if_fail (PHOSH_IS_BT_MANAGER (bt));

  enabled = phosh_bt_manager_get_enabled (bt);
  g_debug ("Updating bt enabled %d", enabled);
  if (self->enabled == enabled)
    return;

  self->enabled = enabled;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);
}

static void
on_bt_present (PhoshBtInfo *self, GParamSpec *pspec, PhoshBtManager *bt)
{
  gboolean present;

  g_return_if_fail (PHOSH_IS_BT_INFO (self));
  g_return_if_fail (PHOSH_IS_BT_MANAGER (bt));

  present = phosh_bt_manager_get_present (bt);
  if (self->present == present)
    return;

  self->present = present;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRESENT]);
}


static gboolean
on_idle (PhoshBtInfo *self)
{
  update_icon (self, NULL, self->bt);
  update_info (self);
  on_bt_enabled (self, NULL, self->bt);
  return G_SOURCE_REMOVE;
}


static void
phosh_bt_info_constructed (GObject *object)
{
  PhoshBtInfo *self = PHOSH_BT_INFO (object);
  PhoshShell *shell;

  G_OBJECT_CLASS (phosh_bt_info_parent_class)->constructed (object);

  shell = phosh_shell_get_default ();
  self->bt = g_object_ref (phosh_shell_get_bt_manager (shell));

  if (self->bt == NULL) {
    g_warning ("Failed to get bt manager");
    return;
  }

  g_signal_connect_swapped (self->bt,
                            "notify::icon-name",
                            G_CALLBACK (update_icon),
                            self);

  /* TODO: track number of BT devices */
  g_signal_connect_swapped (self->bt,
                            "notify::enabled",
                            G_CALLBACK (update_info),
                            self);

  /* We don't use a binding for self->enabled so we can keep
     the property r/o */
  g_signal_connect_swapped (self->bt,
                            "notify::enabled",
                            G_CALLBACK (on_bt_enabled),
                            self);
  g_signal_connect_swapped (self->bt,
                            "notify::present",
                            G_CALLBACK (on_bt_present),
                            self);

  g_idle_add ((GSourceFunc) on_idle, self);
}


static void
phosh_bt_info_dispose (GObject *object)
{
  PhoshBtInfo *self = PHOSH_BT_INFO (object);

  if (self->bt) {
    g_signal_handlers_disconnect_by_data (self->bt, self);
    g_clear_object (&self->bt);
  }

  G_OBJECT_CLASS (phosh_bt_info_parent_class)->dispose (object);
}


static void
phosh_bt_info_class_init (PhoshBtInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_bt_info_constructed;
  object_class->dispose = phosh_bt_info_dispose;
  object_class->get_property = phosh_bt_info_get_property;

  props[PROP_ENABLED] =
    g_param_spec_boolean ("enabled",
                          "enabled",
                          "Whether a bt device is enabled",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);
  props[PROP_PRESENT] =
    g_param_spec_boolean ("present",
                          "Present",
                          "Whether bt hardware is present",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_bt_info_init (PhoshBtInfo *self)
{
}


GtkWidget *
phosh_bt_info_new (void)
{
  return g_object_new (PHOSH_TYPE_BT_INFO, NULL);
}
