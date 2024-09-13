/*
 * Copyright (C) 2023 Tether Operations Limited
 *               2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Authors: Arun Mani J <arunmani@peartree.to>
 *          Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-status-page"

#include "status-page.h"
#include "status-page-placeholder.h"

/**
 * PhoshStatusPage:
 *
 * Additional status information associated with a [class@QuickSetting].
 *
 * This is displayed when the quick setting needs to show status.
 */

enum {
  PROP_0,
  PROP_TITLE,
  PROP_HEADER,
  PROP_FOOTER,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct {
  GtkBox       *toplevel_box;

  /* Header */
  GtkLabel     *title_label;
  GtkBox       *header_bin;
  GtkWidget    *header_widget;

  /* Content */
  GtkBox       *content_bin;
  GtkWidget    *content_widget;

  /* Footer */
  GtkSeparator *footer_separator;
  GtkBox       *footer_bin;
  GtkWidget    *footer_widget;
} PhoshStatusPagePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshStatusPage, phosh_status_page, GTK_TYPE_BIN);


static void
phosh_status_page_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  PhoshStatusPage *self = PHOSH_STATUS_PAGE (object);

  switch (property_id) {
  case PROP_TITLE:
    phosh_status_page_set_title (self, g_value_get_string (value));
    break;
  case PROP_HEADER:
    phosh_status_page_set_header (self, g_value_get_object (value));
    break;
  case PROP_FOOTER:
    phosh_status_page_set_footer (self, g_value_get_object (value));
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
  case PROP_TITLE:
    g_value_set_string (value, phosh_status_page_get_title (self));
    break;
  case PROP_HEADER:
    g_value_set_object (value, phosh_status_page_get_header (self));
    break;
  case PROP_FOOTER:
    g_value_set_object (value, phosh_status_page_get_footer (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static void
phosh_status_page_destroy (GtkWidget *widget)
{
  PhoshStatusPage *self = PHOSH_STATUS_PAGE (widget);
  PhoshStatusPagePrivate *priv = phosh_status_page_get_instance_private (self);

  if (priv->toplevel_box) {
    /* Trigger destruction of all contained widgets */
    gtk_container_remove (GTK_CONTAINER (self), GTK_WIDGET (priv->toplevel_box));
    priv->toplevel_box = NULL;
    priv->footer_widget = NULL;
    priv->header_widget = NULL;
    priv->content_widget = NULL;
    priv->footer_bin = NULL;
    priv->header_bin = NULL;
    priv->content_bin = NULL;
  }

  GTK_WIDGET_CLASS (phosh_status_page_parent_class)->destroy (widget);
}


static void
phosh_status_page_add (GtkContainer *container,
                       GtkWidget    *child)
{
  PhoshStatusPage *self = PHOSH_STATUS_PAGE (container);
  PhoshStatusPagePrivate *priv = phosh_status_page_get_instance_private (self);

  if (!priv->toplevel_box) {
    GTK_CONTAINER_CLASS (phosh_status_page_parent_class)->add (container, child);
  } else if (!priv->content_widget) {
    gtk_container_add (GTK_CONTAINER (priv->content_bin), child);
    priv->content_widget = child;
  } else {
    g_warning ("Attempting to add a second child to a PhoshStatusPage,"
               "but a PhoshStatusPage can only have one child");
  }
}


static void
phosh_status_page_remove (GtkContainer *container,
                          GtkWidget    *child)
{
  PhoshStatusPage *self = PHOSH_STATUS_PAGE (container);
  PhoshStatusPagePrivate *priv = phosh_status_page_get_instance_private (self);

  if (child == GTK_WIDGET (priv->toplevel_box)) {
    GTK_CONTAINER_CLASS (phosh_status_page_parent_class)->remove (container, child);
  } else if (child == priv->content_widget) {
    gtk_container_remove (GTK_CONTAINER (priv->content_bin), child);
    priv->content_widget = NULL;
  } else {
    g_return_if_reached ();
  }
}


static void
phosh_status_page_forall (GtkContainer *container,
                                      gboolean      include_internals,
                                      GtkCallback   callback,
                                      gpointer      callback_data)
{
  PhoshStatusPage *self = PHOSH_STATUS_PAGE (container);
  PhoshStatusPagePrivate *priv = phosh_status_page_get_instance_private (self);

  if (include_internals) {
    GTK_CONTAINER_CLASS (phosh_status_page_parent_class)->forall (container,
                                                                  include_internals,
                                                                  callback,
                                                                  callback_data);
  } else if (priv->content_widget) {
    callback (priv->content_widget, callback_data);
  }
}


static void
phosh_status_page_class_init (PhoshStatusPageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->set_property = phosh_status_page_set_property;
  object_class->get_property = phosh_status_page_get_property;

  widget_class->destroy = phosh_status_page_destroy;

  container_class->add = phosh_status_page_add;
  container_class->remove = phosh_status_page_remove;
  container_class->forall = phosh_status_page_forall;

  /**
   * PhoshStatusPage:title:
   *
   * The status page title
   */
  props[PROP_TITLE] =
    g_param_spec_string ("title", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  /**
   * PhoshStatusPage:header:
   *
   * An extra widget to add to end of the status page's header
   */
  props[PROP_HEADER] =
    g_param_spec_object ("header", "", "",
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  /**
   * PhoshStatusPage:footer:
   *
   * Widget displayed at the very bottom - usually a button.
   */
  props[PROP_FOOTER] =
    g_param_spec_object ("footer", "", "",
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  g_type_ensure (PHOSH_TYPE_STATUS_PAGE_PLACEHOLDER);
  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/phosh/ui/status-page.ui");

  gtk_widget_class_bind_template_child_private (widget_class, PhoshStatusPage, content_bin);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshStatusPage, footer_bin);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshStatusPage, footer_separator);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshStatusPage, header_bin);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshStatusPage, title_label);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshStatusPage, toplevel_box);

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
phosh_status_page_set_title (PhoshStatusPage *self, const char *title)
{
  PhoshStatusPagePrivate *priv;
  const char *current;

  g_return_if_fail (PHOSH_IS_STATUS_PAGE (self));
  priv = phosh_status_page_get_instance_private (self);

  current = gtk_label_get_label (priv->title_label);
  if (g_strcmp0 (current, title) == 0)
    return;

  gtk_label_set_label (priv->title_label, title);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TITLE]);
}


const char *
phosh_status_page_get_title (PhoshStatusPage *self)
{
  PhoshStatusPagePrivate *priv;

  g_return_val_if_fail (PHOSH_IS_STATUS_PAGE (self), NULL);
  priv = phosh_status_page_get_instance_private (self);

  return gtk_label_get_label (priv->title_label);
}

/**
 * phosh_status_page_set_header:
 * @self: A quick setting status page
 *
 * Set the header widget of the status page. See
 * [property@StatusPage:header].
 */
void
phosh_status_page_set_header (PhoshStatusPage *self, GtkWidget *header_widget)
{
  PhoshStatusPagePrivate *priv;

  g_return_if_fail (PHOSH_IS_STATUS_PAGE (self));
  g_return_if_fail (GTK_IS_WIDGET (header_widget));

  priv = phosh_status_page_get_instance_private (self);

  if (priv->header_widget == header_widget)
    return;

  if (priv->header_widget)
    gtk_container_remove (GTK_CONTAINER (priv->header_bin), priv->header_widget);

  priv->header_widget = header_widget;

  if (priv->header_widget)
    gtk_container_add (GTK_CONTAINER (priv->header_bin), priv->header_widget);

  gtk_widget_set_visible (GTK_WIDGET (priv->header_bin), !!header_widget);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HEADER]);
}

/**
 * phosh_status_page_get_header:
 * @self: A quick setting status page
 *
 * Get the header widget of the status page
 *
 * Returns:(transfer none): The status page header
 */
GtkWidget *
phosh_status_page_get_header (PhoshStatusPage *self)
{
  PhoshStatusPagePrivate *priv;

  g_return_val_if_fail (PHOSH_IS_STATUS_PAGE (self), NULL);

  priv = phosh_status_page_get_instance_private (self);

  return priv->header_widget;
}

/**
 * phosh_status_page_set_footer:
 * @self: A quick setting status page
 *
 * Set the footer widget shown at the bottom of a status page
 */
void
phosh_status_page_set_footer (PhoshStatusPage *self, GtkWidget *footer_widget)
{
  PhoshStatusPagePrivate *priv;

  g_return_if_fail (PHOSH_IS_STATUS_PAGE (self));
  g_return_if_fail (GTK_IS_WIDGET (footer_widget));

  priv = phosh_status_page_get_instance_private (self);

  if (priv->footer_widget == footer_widget)
    return;

  if (priv->footer_widget)
    gtk_container_remove (GTK_CONTAINER (priv->footer_bin), priv->footer_widget);

  priv->footer_widget = footer_widget;

  if (priv->footer_widget)
    gtk_container_add (GTK_CONTAINER (priv->footer_bin), priv->footer_widget);

  gtk_widget_set_visible (GTK_WIDGET (priv->footer_separator), !!footer_widget);
  gtk_widget_set_visible (GTK_WIDGET (priv->footer_bin), !!footer_widget);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_FOOTER]);
}

/**
 * phosh_status_page_get_footer:
 * @self: A quick setting status page
 *
 * Get the footer of the status page
 *
 * Returns:(transfer none): The status page footer
 */
GtkWidget *
phosh_status_page_get_footer (PhoshStatusPage *self)
{
  PhoshStatusPagePrivate *priv;

  g_return_val_if_fail (PHOSH_IS_STATUS_PAGE (self), NULL);

  priv = phosh_status_page_get_instance_private (self);

  return priv->footer_widget;
}
