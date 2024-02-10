/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-connectivity-info"

#include "phosh-config.h"

#include "connectivity-info.h"
#include "util.h"

#include <NetworkManager.h>

/**
 * PhoshConnectivityInfo:
 *
 * A widget to display the connectivity status
 *
 * #PhoshConnectivityInfo monitors the connectivity to the internet
 * via NetworkManager and alerts the user about connectivity problems.
 * Usually there's no point in showing the widget when
 * #PhoshConnectivityInfo:connectivity it %TRUE but it's up to the container to
 * decide.
 */

enum {
  PROP_0,
  PROP_CONNECTIVITY,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshConnectivityInfo {
  PhoshStatusIcon parent;

  gboolean        connectivity;
  NMClient       *nmclient;
  GCancellable   *cancel;
};
G_DEFINE_TYPE (PhoshConnectivityInfo, phosh_connectivity_info, PHOSH_TYPE_STATUS_ICON);


static void
phosh_connectivity_info_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  PhoshConnectivityInfo *self = PHOSH_CONNECTIVITY_INFO (object);

  switch (property_id) {
  case PROP_CONNECTIVITY:
    g_value_set_boolean (value, self->connectivity);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
on_connectivity_changed (PhoshConnectivityInfo *self, GParamSpec *pspec, NMClient *nmclient)
{
  const char *icon_name;
  NMConnectivityState state;
  gboolean connectivity = FALSE;

  g_debug ("Updating connectivity icon");
  g_return_if_fail (PHOSH_IS_CONNECTIVITY_INFO (self));
  g_return_if_fail (NM_IS_CLIENT (nmclient));

  state = nm_client_get_connectivity (nmclient);

  switch (state) {
  case NM_CONNECTIVITY_NONE:
    icon_name = "network-offline-symbolic";
    break;
  case NM_CONNECTIVITY_PORTAL:
  case NM_CONNECTIVITY_LIMITED:
    icon_name = "network-no-route-symbolic";
    break;
  case NM_CONNECTIVITY_UNKNOWN:
  case NM_CONNECTIVITY_FULL:
  default:
    icon_name = "network-transmit-receive-symbolic";
    connectivity = TRUE;
  }

  g_debug ("Connectivity changed (%d), updating icon to '%s'",
           state, icon_name);

  phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), icon_name);

  if (connectivity != self->connectivity) {
    self->connectivity = connectivity;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CONNECTIVITY]);
  }
}


static gboolean
on_idle (PhoshConnectivityInfo *self)
{
  on_connectivity_changed (self, NULL, self->nmclient);
  return G_SOURCE_REMOVE;
}


static void
on_nm_client_ready (GObject *obj, GAsyncResult *res, PhoshConnectivityInfo *self)
{
  g_autoptr (GError) err = NULL;
  NMClient *nmclient;
  guint id;

  nmclient = nm_client_new_finish (res, &err);
  if (!nmclient) {
    phosh_async_error_warn (err, "Failed to init NM");
    return;
  }

  g_return_if_fail (PHOSH_IS_CONNECTIVITY_INFO (self));
  self->nmclient = nmclient;

  g_return_if_fail (NM_IS_CLIENT (self->nmclient));

  g_signal_connect_swapped (self->nmclient, "notify::connectivity",
                            G_CALLBACK (on_connectivity_changed), self);

  id = g_idle_add ((GSourceFunc) on_idle, self);
  g_source_set_name_by_id (id, "[PhoshConnectiviyInfo] idle");
}


static void
phosh_connectivity_info_constructed (GObject *object)
{
  PhoshConnectivityInfo *self = PHOSH_CONNECTIVITY_INFO (object);

  G_OBJECT_CLASS (phosh_connectivity_info_parent_class)->constructed (object);

  self->cancel = g_cancellable_new ();
  nm_client_new_async (self->cancel, (GAsyncReadyCallback)on_nm_client_ready, self);
}


static void
phosh_connectivity_info_dispose (GObject *object)
{
  PhoshConnectivityInfo *self = PHOSH_CONNECTIVITY_INFO (object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  g_clear_object (&self->nmclient);

  G_OBJECT_CLASS (phosh_connectivity_info_parent_class)->dispose (object);
}


static void
phosh_connectivity_info_class_init (PhoshConnectivityInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_connectivity_info_constructed;
  object_class->dispose = phosh_connectivity_info_dispose;
  object_class->get_property = phosh_connectivity_info_get_property;

  gtk_widget_class_set_css_name (widget_class, "phosh-connectivity-info");

  props[PROP_CONNECTIVITY] =
    g_param_spec_boolean ("connectivity",
                          "Connectivity",
                          "Whether full connectivity is present",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_connectivity_info_init (PhoshConnectivityInfo *self)
{
}


GtkWidget *
phosh_connectivity_info_new (void)
{
  return g_object_new (PHOSH_TYPE_CONNECTIVITY_INFO, NULL);
}
