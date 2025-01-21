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
#include "connectivity-manager.h"
#include "util.h"
#include "shell.h"

#include <NetworkManager.h>

/**
 * PhoshConnectivityInfo:
 *
 * A widget to display the connectivity status
 *
 * #PhoshConnectivityInfo displays the internet connection status as
 * determined by #PhoshConnectivityManager and alerts the user about
 * connectivity problems.
 *
 * Usually there's no point in showing the
 * widget when #PhoshConnectivityInfo:connectivity is %TRUE but it's
 * up to the container to decide.
 */

enum {
  PROP_0,
  PROP_CONNECTIVITY,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshConnectivityInfo {
  PhoshStatusIcon           parent;

  gboolean                  connectivity;
  PhoshConnectivityManager *connectivity_manager;
};
G_DEFINE_TYPE (PhoshConnectivityInfo, phosh_connectivity_info, PHOSH_TYPE_STATUS_ICON);


static void
phosh_connectivity_info_set_property (GObject      *obj,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  PhoshConnectivityInfo *self = PHOSH_CONNECTIVITY_INFO (obj);

  switch (prop_id) {
  case PROP_CONNECTIVITY:
    self->connectivity = g_value_get_boolean (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    break;
  }
}


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
phosh_connectivity_info_constructed (GObject *object)
{
  PhoshConnectivityInfo *self = PHOSH_CONNECTIVITY_INFO (object);
  PhoshConnectivityManager *manager;

  G_OBJECT_CLASS (phosh_connectivity_info_parent_class)->constructed (object);

  manager = phosh_shell_get_connectivity_manager (phosh_shell_get_default ());
  g_return_if_fail (PHOSH_IS_CONNECTIVITY_MANAGER (manager));

  g_object_bind_property (manager, "connectivity", self, "connectivity", G_BINDING_SYNC_CREATE);
  g_object_bind_property (manager, "icon-name", self, "icon-name", G_BINDING_SYNC_CREATE);
}


static void
phosh_connectivity_info_class_init (PhoshConnectivityInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_connectivity_info_constructed;
  object_class->get_property = phosh_connectivity_info_get_property;
  object_class->set_property = phosh_connectivity_info_set_property;

  gtk_widget_class_set_css_name (widget_class, "phosh-connectivity-info");

  /* PhoshConnectivityInfo:connectivity:
   *
   * %TRUE if a connection to the internet is present, otherwise %FALSE
   */
  props[PROP_CONNECTIVITY] =
    g_param_spec_boolean ("connectivity", "", "",
                          FALSE,
                          G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS);
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
