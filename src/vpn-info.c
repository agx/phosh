/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-vpn-info"

#include "phosh-config.h"

#include "shell.h"
#include "vpn-info.h"
#include "vpn-manager.h"

/**
 * PhoshVpnInfo:
 *
 * A widget to display the vpn status
 *
 * #PhoshVpnInfo displays the current vpn status based on information
 * from #PhoshVpnManager. To figure out if the widget should be shown
 * the #PhoshVpnInfo:enabled property can be useful.
 */

enum {
  PROP_0,
  PROP_PRESENT,
  PROP_ENABLED,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshVpnInfo {
  PhoshStatusIcon  parent;

  gboolean         present;
  gboolean         enabled;
  PhoshVpnManager *vpn;
};
G_DEFINE_TYPE (PhoshVpnInfo, phosh_vpn_info, PHOSH_TYPE_STATUS_ICON);

static void
phosh_vpn_info_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  PhoshVpnInfo *self = PHOSH_VPN_INFO (object);

  switch (property_id) {
  case PROP_PRESENT:
    g_value_set_boolean (value, self->present);
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
update_icon (PhoshVpnInfo *self, GParamSpec *pspec, PhoshVpnManager *vpn)
{
  const char *icon_name;

  g_debug ("Updating vpn icon");
  g_return_if_fail (PHOSH_IS_VPN_INFO (self));
  g_return_if_fail (PHOSH_IS_VPN_MANAGER (vpn));

  icon_name = phosh_vpn_manager_get_icon_name (vpn);
  if (icon_name)
    phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), icon_name);
}

static void
update_info (PhoshVpnInfo *self)
{
  const char *info;
  g_return_if_fail (PHOSH_IS_VPN_INFO (self));

  info = phosh_vpn_manager_get_last_connection (self->vpn);
  if (info)
    phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), info);
  else
    phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), _("VPN"));
}

static void
on_vpn_enabled (PhoshVpnInfo *self, GParamSpec *pspec, PhoshVpnManager *vpn)
{
  gboolean enabled;

  g_debug ("Updating vpn status");
  g_return_if_fail (PHOSH_IS_VPN_INFO (self));
  g_return_if_fail (PHOSH_IS_VPN_MANAGER (vpn));

  enabled = phosh_vpn_manager_get_enabled (vpn);
  if (self->enabled == enabled)
    return;

  self->enabled = enabled;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);
}


static void
on_vpn_present (PhoshVpnInfo *self, GParamSpec *pspec, PhoshVpnManager *vpn)
{
  gboolean present;

  g_return_if_fail (PHOSH_IS_VPN_INFO (self));
  g_return_if_fail (PHOSH_IS_VPN_MANAGER (vpn));

  present = phosh_vpn_manager_get_present (vpn);
  g_debug ("Updating vpn status: %d", present);
  if (self->present == present)
    return;

  self->present = present;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRESENT]);
}

static void
on_vpn_last_con_changed (PhoshVpnInfo *self, GParamSpec *pspec, PhoshVpnManager *vpn)
{
  update_info (self);
}

static void
phosh_vpn_info_idle_init (PhoshStatusIcon *icon)
{
  PhoshVpnInfo *self = PHOSH_VPN_INFO (icon);

  update_icon (self, NULL, self->vpn);

  on_vpn_present (self, NULL, self->vpn);
  on_vpn_enabled (self, NULL, self->vpn);
  on_vpn_last_con_changed (self, NULL, self->vpn);
}


static void
phosh_vpn_info_constructed (GObject *object)
{
  PhoshVpnInfo *self = PHOSH_VPN_INFO (object);
  PhoshShell *shell;

  G_OBJECT_CLASS (phosh_vpn_info_parent_class)->constructed (object);

  shell = phosh_shell_get_default ();
  self->vpn = g_object_ref (phosh_shell_get_vpn_manager (shell));

  if (self->vpn == NULL) {
    g_warning ("Failed to get vpn manager");
    return;
  }

  g_signal_connect_swapped (self->vpn,
                            "notify::icon-name",
                            G_CALLBACK (update_icon),
                            self);

  /* We don't use a binding for self->enabled so we can keep
     the property r/o */
  g_object_connect (self->vpn,
                    "swapped-signal::notify::enabled", G_CALLBACK (on_vpn_enabled), self,
                    "swapped-signal::notify::present",G_CALLBACK (on_vpn_present), self,
                    "swapped-signal::notify::last-connection",G_CALLBACK (on_vpn_last_con_changed), self,
                    NULL);
}


static void
phosh_vpn_info_dispose (GObject *object)
{
  PhoshVpnInfo *self = PHOSH_VPN_INFO (object);

  if (self->vpn) {
    g_signal_handlers_disconnect_by_data (self->vpn, self);
    g_clear_object (&self->vpn);
  }

  G_OBJECT_CLASS (phosh_vpn_info_parent_class)->dispose (object);
}


static void
phosh_vpn_info_class_init (PhoshVpnInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhoshStatusIconClass *status_icon_class = PHOSH_STATUS_ICON_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_vpn_info_constructed;
  object_class->dispose = phosh_vpn_info_dispose;
  object_class->get_property = phosh_vpn_info_get_property;

  status_icon_class->idle_init = phosh_vpn_info_idle_init;

  gtk_widget_class_set_css_name (widget_class, "phosh-vpn-info");

  props[PROP_PRESENT] =
    g_param_spec_boolean ("present", "", "",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);
  /**
   * PhoshVpnInfo:enabled
   *
   * Whether a VPN connection is enabled
   */
  props[PROP_ENABLED] =
    g_param_spec_boolean ("enabled", "", "",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_vpn_info_init (PhoshVpnInfo *self)
{
}


GtkWidget *
phosh_vpn_info_new (void)
{
  return g_object_new (PHOSH_TYPE_VPN_INFO, NULL);
}
