/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Arun Mani J <arunmani@peartree.to>
 */

#define G_LOG_DOMAIN "phosh-app-grid-base-button"

#include "app-grid-base-button.h"

#include "fading-label.h"
#include "clamp.h"

/**
 * PhoshAppGridBaseButton:
 *
 * Base class for buttons in app grid. Add the display widget (like image or grid of images) as a
 * child. Use [property@Phosh.AppGridBaseButton:label] property to set the label.
 */

enum {
  PROP_0,
  PROP_LABEL,
  PROP_CHILD,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct {
  GtkBox                *box;
  GtkWidget             *button;
  PhoshFadingLabel      *label;
  GtkWidget             *child;
} PhoshAppGridBaseButtonPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (PhoshAppGridBaseButton, phosh_app_grid_base_button,
                                     GTK_TYPE_FLOW_BOX_CHILD);


static void
phosh_app_grid_base_button_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  PhoshAppGridBaseButton *self = PHOSH_APP_GRID_BASE_BUTTON (object);

  switch (property_id) {
    case PROP_LABEL:
      phosh_app_grid_base_button_set_label (self, g_value_get_string (value));
      break;
    case PROP_CHILD:
      phosh_app_grid_base_button_set_child (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static void
phosh_app_grid_base_button_get_property (GObject      *object,
                                         guint         property_id,
                                         GValue       *value,
                                         GParamSpec   *pspec)
{
  PhoshAppGridBaseButton *self = PHOSH_APP_GRID_BASE_BUTTON (object);

  switch (property_id) {
    case PROP_LABEL:
      g_value_set_string (value, phosh_app_grid_base_button_get_label (self));
      break;
    case PROP_CHILD:
      g_value_set_object (value, phosh_app_grid_base_button_get_child (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static void
on_clicked_cb (PhoshAppGridBaseButton *self)
{
  g_signal_emit_by_name (self, "activate", NULL);
}


static void
phosh_app_grid_base_button_destroy (GtkWidget *widget)
{
  PhoshAppGridBaseButton *self = PHOSH_APP_GRID_BASE_BUTTON (widget);

  phosh_app_grid_base_button_set_child (self, NULL);

  GTK_WIDGET_CLASS (phosh_app_grid_base_button_parent_class)->destroy (widget);
}


static void
phosh_app_grid_base_button_grab_focus (GtkWidget *widget)
{
  PhoshAppGridBaseButton *self = PHOSH_APP_GRID_BASE_BUTTON (widget);
  PhoshAppGridBaseButtonPrivate *priv = phosh_app_grid_base_button_get_instance_private (self);

  gtk_widget_grab_focus (priv->button);
}


static void
phosh_app_grid_base_button_class_init (PhoshAppGridBaseButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = phosh_app_grid_base_button_set_property;
  object_class->get_property = phosh_app_grid_base_button_get_property;

  widget_class->destroy = phosh_app_grid_base_button_destroy;
  widget_class->grab_focus = phosh_app_grid_base_button_grab_focus;

  /**
   * PhoshAppGridBaseButton:label:
   *
   * The label to display for button. A `NULL` results in the label widget being hidden.
   */
  props[PROP_LABEL] =
    g_param_spec_string ("label", "", "",
                         NULL,
                         G_PARAM_READWRITE |
                         G_PARAM_EXPLICIT_NOTIFY |
                         G_PARAM_STATIC_STRINGS);
  /**
   * PhoshAppGridBaseButton:child:
   *
   * The child content of the button.
   */
  props[PROP_CHILD] =
    g_param_spec_object ("child", "", "",
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  g_type_ensure (PHOSH_TYPE_CLAMP);
  g_type_ensure (PHOSH_TYPE_FADING_LABEL);
  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/ui/app-grid-base-button.ui");

  gtk_widget_class_bind_template_callback (widget_class, on_clicked_cb);

  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGridBaseButton, box);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGridBaseButton, button);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGridBaseButton, label);
}


static void
phosh_app_grid_base_button_init (PhoshAppGridBaseButton *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


void
phosh_app_grid_base_button_set_label (PhoshAppGridBaseButton *self, const char *label)
{
  PhoshAppGridBaseButtonPrivate *priv;

  g_return_if_fail (PHOSH_IS_APP_GRID_BASE_BUTTON (self));

  priv = phosh_app_grid_base_button_get_instance_private (self);

  if (g_strcmp0 (label, phosh_fading_label_get_label (priv->label)) == 0)
    return;

  phosh_fading_label_set_label (priv->label, label);
  gtk_widget_set_visible (GTK_WIDGET (priv->label), label != NULL);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_LABEL]);
}


const char *
phosh_app_grid_base_button_get_label (PhoshAppGridBaseButton *self)
{
  PhoshAppGridBaseButtonPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_APP_GRID_BASE_BUTTON (self), 0);

  priv = phosh_app_grid_base_button_get_instance_private (self);

  return phosh_fading_label_get_label (priv->label);
}

/**
 * phosh_app_grid_base_button_set_child:
 * @self: An app-grid base button
 * @child: A widget or `NULL`
 *
 * Set the child of base button. Use `NULL` to remove exisitng child.
 */
void
phosh_app_grid_base_button_set_child (PhoshAppGridBaseButton *self, GtkWidget *child)
{
  PhoshAppGridBaseButtonPrivate *priv;

  g_return_if_fail (PHOSH_IS_APP_GRID_BASE_BUTTON (self));
  g_return_if_fail (child == NULL || GTK_IS_WIDGET (child));

  priv = phosh_app_grid_base_button_get_instance_private (self);

  if (priv->child == child)
    return;

  if (priv->child)
    gtk_container_remove (GTK_CONTAINER (priv->box), priv->child);

  priv->child = child;

  if (priv->child)
    gtk_box_pack_start (priv->box, priv->child, FALSE, FALSE, 0);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CHILD]);
}

/**
 * phosh_app_grid_base_button_get_child:
 * @self: An app-grid base button
 *
 * Get the current child of base button.
 *
 * Returns:(transfer none): The child or `NULL`.
 */
GtkWidget *
phosh_app_grid_base_button_get_child (PhoshAppGridBaseButton *self)
{
  PhoshAppGridBaseButtonPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_APP_GRID_BASE_BUTTON (self), NULL);

  priv = phosh_app_grid_base_button_get_instance_private (self);

  return priv->child;
}
