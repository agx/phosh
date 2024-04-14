/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Arun Mani J <arunmani@peartree.to>
 */

#define G_LOG_DOMAIN "phosh-status-page"

#include "status-page.h"

/**
 * PhoshStatusPage:
 *
 * A widget to show more details about a status indicator like WiFi, Bluetooth etc.
 *
 * PhoshStatusPage is used to show more details about a status indicator. It must be subclassed to
 * display the required information. PhoshSettings will show this information when the respective
 * PhoshQuickSetting is activated.
 */

enum {
  PROP_0,
  PROP_HEADER,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct {
  GtkWidget *header;
} PhoshStatusPagePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshStatusPage, phosh_status_page, GTK_TYPE_BOX);

static void
phosh_status_page_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  PhoshStatusPage *self = PHOSH_STATUS_PAGE (object);

  switch (property_id) {
  case PROP_HEADER:
    phosh_status_page_set_header (self, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static void
phosh_status_page_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PhoshStatusPage *self = PHOSH_STATUS_PAGE (object);

  switch (property_id) {
  case PROP_HEADER:
    g_value_set_object (value, phosh_status_page_get_header (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
phosh_status_page_class_init (PhoshStatusPageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = phosh_status_page_set_property;
  object_class->get_property = phosh_status_page_get_property;

  /**
   * PhoshStatusPage:header:
   *
   * The header widget
   */
  props[PROP_HEADER] =
    g_param_spec_object ("header", "", "",
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/phosh/ui/status-page.ui");

  gtk_widget_class_set_css_name (widget_class, "phosh-status-page");
}

static void
phosh_status_page_init (PhoshStatusPage *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

PhoshStatusPage *
phosh_status_page_new (void)
{
  return g_object_new (PHOSH_TYPE_STATUS_PAGE, NULL);
}

void
phosh_status_page_set_header (PhoshStatusPage *self, GtkWidget *header)
{
  PhoshStatusPagePrivate *priv;

  g_return_if_fail (PHOSH_IS_STATUS_PAGE (self));
  g_return_if_fail (GTK_IS_WIDGET (header));

  priv = phosh_status_page_get_instance_private (self);

  if (priv->header == header)
    return;

  if (priv->header)
    gtk_container_remove (GTK_CONTAINER (self), priv->header);

  priv->header = header;

  if (priv->header)
    gtk_box_pack_start (GTK_BOX (self), priv->header, FALSE, FALSE, 0);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HEADER]);
}

/**
 * phosh_status_page_get_header:
 * @self: A quick setting status page
 *
 * Get the header of the status page
 *
 * Returns:(transfer none): The status page header
 */
GtkWidget *
phosh_status_page_get_header (PhoshStatusPage *self)
{
  PhoshStatusPagePrivate *priv;

  g_return_val_if_fail (PHOSH_IS_STATUS_PAGE (self), 0);

  priv = phosh_status_page_get_instance_private (self);

  return priv->header;
}
