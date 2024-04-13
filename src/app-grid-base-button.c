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
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct {
  GtkBox                *box;
  GtkWidget             *button;
  PhoshFadingLabel      *label;
} PhoshAppGridBaseButtonPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshAppGridBaseButton, phosh_app_grid_base_button, GTK_TYPE_FLOW_BOX_CHILD);


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
phosh_app_grid_base_button_grab_focus (GtkWidget *widget)
{
  PhoshAppGridBaseButton *self = PHOSH_APP_GRID_BASE_BUTTON (widget);
  PhoshAppGridBaseButtonPrivate *priv = phosh_app_grid_base_button_get_instance_private (self);

  gtk_widget_grab_focus (priv->button);
}


static void
phosh_app_grid_base_button_add (GtkContainer *container, GtkWidget *child)
{
  PhoshAppGridBaseButton *self = PHOSH_APP_GRID_BASE_BUTTON (container);
  PhoshAppGridBaseButtonPrivate *priv = phosh_app_grid_base_button_get_instance_private (self);

  if (gtk_bin_get_child (GTK_BIN (container)) == NULL) {
    GTK_CONTAINER_CLASS (phosh_app_grid_base_button_parent_class)->add (GTK_CONTAINER (self), child);
    return;
  }

  gtk_box_pack_start (priv->box, child, FALSE, FALSE, 0);
}


static void
phosh_app_grid_base_button_class_init (PhoshAppGridBaseButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->set_property = phosh_app_grid_base_button_set_property;
  object_class->get_property = phosh_app_grid_base_button_get_property;

  widget_class->grab_focus = phosh_app_grid_base_button_grab_focus;
  container_class->add = phosh_app_grid_base_button_add;

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

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  g_type_ensure (PHOSH_TYPE_CLAMP);
  g_type_ensure (PHOSH_TYPE_FADING_LABEL);
  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/phosh/ui/app-grid-base-button.ui");

  gtk_widget_class_bind_template_callback (widget_class, on_clicked_cb);

  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGridBaseButton, box);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGridBaseButton, button);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGridBaseButton, label);

  gtk_widget_class_set_css_name (widget_class, "phosh-app-grid-base-button");
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
