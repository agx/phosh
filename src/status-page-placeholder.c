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
  PROP_EXTRA_WIDGET,
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
  case PROP_EXTRA_WIDGET:
    phosh_status_page_placeholder_set_extra_widget (self, g_value_get_object (value));
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
  case PROP_EXTRA_WIDGET:
    g_value_set_object (value, phosh_status_page_placeholder_get_extra_widget (self));
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
phosh_status_page_placeholder_destroy (GtkWidget *widget)
{
  PhoshStatusPagePlaceholder *self = PHOSH_STATUS_PAGE_PLACEHOLDER (widget);

  phosh_status_page_placeholder_set_extra_widget (self, NULL);

  GTK_WIDGET_CLASS (phosh_status_page_placeholder_parent_class)->destroy (widget);
}


static void
phosh_status_page_placeholder_class_init (PhoshStatusPagePlaceholderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_status_page_placeholder_get_property;
  object_class->set_property = phosh_status_page_placeholder_set_property;
  object_class->dispose = phosh_status_page_placeholder_dispose;

  widget_class->destroy = phosh_status_page_placeholder_destroy;

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
  /**
   * PhoshStatusPagePlaceholder:extra-widget:
   *
   * An extra widget to add to bottom of the placeholder page.
   */
  props[PROP_EXTRA_WIDGET] =
    g_param_spec_object ("extra-widget", "", "",
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);


  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/ui/status-page-placeholder.ui");

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

/**
 * phosh_status_page_placeholder_set_extra_widget:
 * @self: A status page placeholder
 *
 * Set the extra widget shown at the bottom of a status page placeholder. Use `NULL` to remove
 * existing widget.
 */
void
phosh_status_page_placeholder_set_extra_widget (PhoshStatusPagePlaceholder *self, GtkWidget *extra_widget)
{
  g_return_if_fail (PHOSH_IS_STATUS_PAGE_PLACEHOLDER (self));
  g_return_if_fail (extra_widget == NULL || GTK_IS_WIDGET (extra_widget));

  if (self->extra_widget == extra_widget)
    return;

  if (self->extra_widget)
    gtk_container_remove (GTK_CONTAINER (self->toplevel_box), self->extra_widget);

  self->extra_widget = extra_widget;

  if (self->extra_widget)
    gtk_container_add (GTK_CONTAINER (self->toplevel_box), self->extra_widget);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_EXTRA_WIDGET]);
}

/**
 * phosh_status_page_placeholder_get_extra_widget:
 * @self: A status page placeholder
 *
 * Get the extra_widget of the status page placeholder.
 *
 * Returns:(transfer none): The status page placeholder extra_widget
 */
GtkWidget *
phosh_status_page_placeholder_get_extra_widget (PhoshStatusPagePlaceholder *self)
{
  g_return_val_if_fail (PHOSH_IS_STATUS_PAGE_PLACEHOLDER (self), NULL);

  return self->extra_widget;
}
