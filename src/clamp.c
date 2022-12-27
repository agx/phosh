/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "clamp.h"

/**
 * PhoshClamp:
 *
 * A container limiting its natural size request
 *
 * This should not be confused with `HdyClamp`, which limits the size allocated
 * to its child and adds a dynamic margin around it. `PhoshClamp` instead
 * constraints the requested natural size, the child will still be allocated all
 * that the clamp gets.
 */

enum {
  PROP_0,
  PROP_NATURAL_SIZE,

  /* Overridden properties */
  PROP_ORIENTATION,

  LAST_PROP = PROP_NATURAL_SIZE + 1,
};

struct _PhoshClamp
{
  GtkBin parent_instance;

  gint natural_size;

  GtkOrientation orientation;
};

static GParamSpec *props[LAST_PROP];

G_DEFINE_TYPE_WITH_CODE (PhoshClamp, phosh_clamp, GTK_TYPE_BIN,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL))

static void
set_orientation (PhoshClamp     *self,
                 GtkOrientation  orientation)
{
  if (self->orientation == orientation)
    return;

  self->orientation = orientation;
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify (G_OBJECT (self), "orientation");
}

static void
phosh_clamp_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  PhoshClamp *self = PHOSH_CLAMP (object);

  switch (prop_id) {
  case PROP_NATURAL_SIZE:
    g_value_set_int (value, phosh_clamp_get_natural_size (self));
    break;
  case PROP_ORIENTATION:
    g_value_set_enum (value, self->orientation);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
phosh_clamp_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  PhoshClamp *self = PHOSH_CLAMP (object);

  switch (prop_id) {
  case PROP_NATURAL_SIZE:
    phosh_clamp_set_natural_size (self, g_value_get_int (value));
    break;
  case PROP_ORIENTATION:
    set_orientation (self, g_value_get_enum (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

/* This private method is prefixed by the call name because it will be a virtual
 * method in GTK 4.
 */
static void
phosh_clamp_measure (GtkWidget      *widget,
                     GtkOrientation  orientation,
                     int             for_size,
                     int            *minimum,
                     int            *natural,
                     int            *minimum_baseline,
                     int            *natural_baseline)
{
  PhoshClamp *self = PHOSH_CLAMP (widget);
  GtkBin *bin = GTK_BIN (widget);
  GtkWidget *child;
  int child_min = 0;
  int child_nat = 0;
  int child_min_baseline = -1;
  int child_nat_baseline = -1;

  if (minimum)
    *minimum = 0;
  if (natural)
    *natural = 0;
  if (minimum_baseline)
    *minimum_baseline = -1;
  if (natural_baseline)
    *natural_baseline = -1;

  child = gtk_bin_get_child (bin);

  if (!child || !gtk_widget_is_visible (child))
    return;

  if (self->orientation == orientation) {
    if (orientation == GTK_ORIENTATION_HORIZONTAL)
      gtk_widget_get_preferred_width (child, &child_min, &child_nat);
    else
      gtk_widget_get_preferred_height_and_baseline_for_width (child, -1,
                                                              &child_min,
                                                              &child_nat,
                                                              &child_min_baseline,
                                                              &child_nat_baseline);

    child_nat = MIN (child_nat, self->natural_size);
    child_nat = MAX (child_min, child_nat);
  } else {
    if (orientation == GTK_ORIENTATION_HORIZONTAL)
      gtk_widget_get_preferred_width_for_height (child, for_size,
                                                 &child_min, &child_nat);
    else
      gtk_widget_get_preferred_height_and_baseline_for_width (child, for_size,
                                                              &child_min,
                                                              &child_nat,
                                                              &child_min_baseline,
                                                              &child_nat_baseline);
  }

  if (minimum)
    *minimum = child_min;
  if (natural)
    *natural = child_nat;
  if (minimum_baseline && child_min_baseline > -1)
    *minimum_baseline = child_min_baseline;
  if (natural_baseline && child_nat_baseline > -1)
    *natural_baseline = child_nat_baseline;
}

static GtkSizeRequestMode
phosh_clamp_get_request_mode (GtkWidget *widget)
{
  PhoshClamp *self = PHOSH_CLAMP (widget);

  return self->orientation == GTK_ORIENTATION_HORIZONTAL ?
    GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH :
    GTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT;
}

static void
phosh_clamp_get_preferred_width_for_height (GtkWidget *widget,
                                            gint       height,
                                            gint      *minimum,
                                            gint      *natural)
{
  phosh_clamp_measure (widget, GTK_ORIENTATION_HORIZONTAL, height,
                       minimum, natural, NULL, NULL);
}

static void
phosh_clamp_get_preferred_width (GtkWidget *widget,
                                 gint      *minimum,
                                 gint      *natural)
{
  phosh_clamp_measure (widget, GTK_ORIENTATION_HORIZONTAL, -1,
                       minimum, natural, NULL, NULL);
}

static void
phosh_clamp_get_preferred_height_and_baseline_for_width (GtkWidget *widget,
                                                         gint       width,
                                                         gint      *minimum,
                                                         gint      *natural,
                                                         gint      *minimum_baseline,
                                                         gint      *natural_baseline)
{
  phosh_clamp_measure (widget, GTK_ORIENTATION_VERTICAL, width,
                       minimum, natural, minimum_baseline, natural_baseline);
}

static void
phosh_clamp_get_preferred_height_for_width (GtkWidget *widget,
                                            gint       width,
                                            gint      *minimum,
                                            gint      *natural)
{
  phosh_clamp_measure (widget, GTK_ORIENTATION_VERTICAL, width,
                       minimum, natural, NULL, NULL);
}

static void
phosh_clamp_get_preferred_height (GtkWidget *widget,
                                  gint      *minimum,
                                  gint      *natural)
{
  phosh_clamp_measure (widget, GTK_ORIENTATION_VERTICAL, -1,
                       minimum, natural, NULL, NULL);
}

static void
phosh_clamp_class_init (PhoshClampClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->get_property = phosh_clamp_get_property;
  object_class->set_property = phosh_clamp_set_property;

  widget_class->get_request_mode = phosh_clamp_get_request_mode;
  widget_class->get_preferred_width = phosh_clamp_get_preferred_width;
  widget_class->get_preferred_width_for_height = phosh_clamp_get_preferred_width_for_height;
  widget_class->get_preferred_height = phosh_clamp_get_preferred_height;
  widget_class->get_preferred_height_for_width = phosh_clamp_get_preferred_height_for_width;
  widget_class->get_preferred_height_and_baseline_for_width = phosh_clamp_get_preferred_height_and_baseline_for_width;

  gtk_container_class_handle_border_width (container_class);

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  /**
   * PhoshClamp:natural-size:
   *
   * The maximum natural size request.
   */
  props[PROP_NATURAL_SIZE] =
      g_param_spec_int ("natural-size", "", "",
                        -1, G_MAXINT, -1,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_css_name (widget_class, "clamp");
}

static void
phosh_clamp_init (PhoshClamp *self)
{
  self->natural_size = -1;
}

/**
 * phosh_clamp_new:
 *
 * Creates a new #PhoshClamp.
 *
 * Returns: a new #PhoshClamp
 */
GtkWidget *
phosh_clamp_new (void)
{
  return g_object_new (PHOSH_TYPE_CLAMP, NULL);
}

/**
 * phosh_clamp_get_natural_size:
 * @self: a #PhoshClamp
 *
 * Gets the maximum natural size request.
 *
 * Returns: the maximum natural size request.
 */
gint
phosh_clamp_get_natural_size (PhoshClamp *self)
{
  g_return_val_if_fail (PHOSH_IS_CLAMP (self), 0);

  return self->natural_size;
}

/**
 * phosh_clamp_set_natural_size:
 * @self: a #PhoshClamp
 * @natural_size: the maximum natural size
 *
 * Sets the maximum natural size request.
 */
void
phosh_clamp_set_natural_size (PhoshClamp *self,
                            gint      natural_size)
{
  g_return_if_fail (PHOSH_IS_CLAMP (self));

  if (self->natural_size == natural_size)
    return;

  self->natural_size = natural_size;

  gtk_widget_queue_resize (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_NATURAL_SIZE]);
}
