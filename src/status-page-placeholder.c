/*
 * Copyright (C) 2024 Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-status-page-placeholder"

#include "phosh-config.h"

#include "status-page-placeholder.h"

#include <gmobile.h>

/**
 * PhoshStatusPagePlaceholder:
 *
 * A placeholder in a [class@StatusPage].
 *
 * The placeholder page has a title and an icon and can have a single
 * child which is put below the title.
 *
 * This widget can be replaced with `AdwStatusPage` and a bit of styling
 * once we switch to GTK4.
 */

enum {
  PROP_0,
  PROP_TITLE,
  PROP_ICON_NAME,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshStatusPagePlaceholder {
  GtkBin        parent;

  GtkBox       *toplevel_box;

  GtkImage     *icon;
  char         *icon_name;
  GtkLabel     *title_label;

  GtkWidget    *extra_widget;
};
G_DEFINE_TYPE (PhoshStatusPagePlaceholder, phosh_status_page_placeholder, GTK_TYPE_BIN)


static void
update_title_visibility (PhoshStatusPagePlaceholder *self)
{
  const char *text = gtk_label_get_text (self->title_label);

  gtk_widget_set_visible (GTK_WIDGET (self->title_label), !gm_str_is_null_or_empty (text));
}


static void
phosh_status_page_placeholder_destroy (GtkWidget *widget)
{
  PhoshStatusPagePlaceholder *self = PHOSH_STATUS_PAGE_PLACEHOLDER (widget);

  if (self->toplevel_box) {
    /* Trigger destruction of all contained widgets */
    gtk_container_remove (GTK_CONTAINER (self), GTK_WIDGET (self->toplevel_box));
    self->toplevel_box = NULL;
    self->icon = NULL;
    self->title_label = NULL;
    self->extra_widget = NULL;
  }

  GTK_WIDGET_CLASS (phosh_status_page_placeholder_parent_class)->destroy (widget);
}


static void
phosh_status_page_placeholder_add (GtkContainer *container,
                                   GtkWidget    *child)
{
  PhoshStatusPagePlaceholder *self = PHOSH_STATUS_PAGE_PLACEHOLDER (container);

  if (!self->toplevel_box) {
    GTK_CONTAINER_CLASS (phosh_status_page_placeholder_parent_class)->add (container, child);
  } else if (!self->extra_widget) {
    gtk_container_add (GTK_CONTAINER (self->toplevel_box), child);
    self->extra_widget = child;
  } else {
    g_warning ("Attempting to add a second child to a PhoshStatusPagePlaceholder,"
               "but a PhoshStatusPagePlaceholder can only have one child");
  }
}


static void
phosh_status_page_placeholder_remove (GtkContainer *container,
                                      GtkWidget    *child)
{
  PhoshStatusPagePlaceholder *self = PHOSH_STATUS_PAGE_PLACEHOLDER (container);

  if (child == GTK_WIDGET (self->toplevel_box)) {
    GTK_CONTAINER_CLASS (phosh_status_page_placeholder_parent_class)->remove (container, child);
  } else if (child == self->extra_widget) {
    gtk_container_remove (GTK_CONTAINER (self->toplevel_box), child);
    self->extra_widget = NULL;
  } else {
    g_return_if_reached ();
  }
}


static void
phosh_status_page_placeholder_forall (GtkContainer *container,
                                      gboolean      include_internals,
                                      GtkCallback   callback,
                                      gpointer      callback_data)
{
  PhoshStatusPagePlaceholder *self = PHOSH_STATUS_PAGE_PLACEHOLDER (container);

  if (include_internals)
    GTK_CONTAINER_CLASS (phosh_status_page_placeholder_parent_class)->forall (container,
                                                                              include_internals,
                                                                              callback,
                                                                              callback_data);
  else if (self->extra_widget)
    callback (self->extra_widget, callback_data);
}


static void
phosh_status_page_placeholder_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  PhoshStatusPagePlaceholder *self = PHOSH_STATUS_PAGE_PLACEHOLDER (object);

  switch (property_id) {
  case PROP_TITLE:
    phosh_status_page_placeholder_set_title (self, g_value_get_string (value));
    break;
  case PROP_ICON_NAME:
    phosh_status_page_placeholder_set_icon_name (self, g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_status_page_placeholder_get_property (GObject    *object,
                                            guint       property_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
  PhoshStatusPagePlaceholder *self = PHOSH_STATUS_PAGE_PLACEHOLDER (object);

  switch (property_id) {
  case PROP_TITLE:
    g_value_set_string (value, phosh_status_page_placeholder_get_title (self));
    break;
  case PROP_ICON_NAME:
    g_value_set_string (value, phosh_status_page_placeholder_get_icon_name (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_status_page_placeholder_dispose (GObject *object)
{
  PhoshStatusPagePlaceholder *self = PHOSH_STATUS_PAGE_PLACEHOLDER (object);

  g_clear_pointer (&self->icon_name, g_free);

  G_OBJECT_CLASS (phosh_status_page_placeholder_parent_class)->dispose (object);
}


static void
phosh_status_page_placeholder_class_init (PhoshStatusPagePlaceholderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->get_property = phosh_status_page_placeholder_get_property;
  object_class->set_property = phosh_status_page_placeholder_set_property;
  object_class->dispose = phosh_status_page_placeholder_dispose;

  widget_class->destroy = phosh_status_page_placeholder_destroy;

  container_class->add = phosh_status_page_placeholder_add;
  container_class->remove = phosh_status_page_placeholder_remove;
  container_class->forall = phosh_status_page_placeholder_forall;

  /**
   * PhoshStatusPagePlaceholder:title:
   *
   * The title of the placeholder page, displayed below the icon.
   */
  props[PROP_TITLE] =
    g_param_spec_string ("title", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  /**
   * PhoshStatusPagePlaceholder:icon-name:
   *
   * The name of the icon on the placeholder page
   */
  props[PROP_ICON_NAME] =
    g_param_spec_string ("icon-name", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/status-page-placeholder.ui");

  gtk_widget_class_bind_template_child (widget_class, PhoshStatusPagePlaceholder, icon);
  gtk_widget_class_bind_template_child (widget_class, PhoshStatusPagePlaceholder, title_label);
  gtk_widget_class_bind_template_child (widget_class, PhoshStatusPagePlaceholder, toplevel_box);

  gtk_widget_class_set_css_name (widget_class, "phosh-status-page-placeholder");
}


static void
phosh_status_page_placeholder_init (PhoshStatusPagePlaceholder *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  update_title_visibility (self);
}


PhoshStatusPagePlaceholder *
phosh_status_page_placeholder_new (void)
{
  return g_object_new (PHOSH_TYPE_STATUS_PAGE_PLACEHOLDER, NULL);
}


void
phosh_status_page_placeholder_set_title (PhoshStatusPagePlaceholder *self, const char *title)
{
  const char *current;

  g_return_if_fail (PHOSH_IS_STATUS_PAGE_PLACEHOLDER (self));

  current = gtk_label_get_label (self->title_label);
  if (g_strcmp0 (current, title) == 0)
    return;

  gtk_label_set_label (self->title_label, title);
  update_title_visibility (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TITLE]);
}


const char *
phosh_status_page_placeholder_get_title (PhoshStatusPagePlaceholder *self)
{
  g_return_val_if_fail (PHOSH_IS_STATUS_PAGE_PLACEHOLDER (self), NULL);

  return gtk_label_get_label (self->title_label);
}


void
phosh_status_page_placeholder_set_icon_name (PhoshStatusPagePlaceholder *self, const char *icon_name)
{
  g_return_if_fail (PHOSH_IS_STATUS_PAGE_PLACEHOLDER (self));

  if (g_strcmp0 (self->icon_name, icon_name) == 0)
    return;

  g_free (self->icon_name);
  self->icon_name = g_strdup (icon_name);

  if (!icon_name)
    g_object_set (G_OBJECT (self->icon), "icon-name", "image-missing", NULL);
  else
    g_object_set (G_OBJECT (self->icon), "icon-name", icon_name, NULL);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ICON_NAME]);
}


const char *
phosh_status_page_placeholder_get_icon_name (PhoshStatusPagePlaceholder *self)
{
  g_return_val_if_fail (PHOSH_IS_STATUS_PAGE_PLACEHOLDER (self), NULL);

  return self->icon_name;
}
