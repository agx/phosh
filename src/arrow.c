/*
 * Copyright © 2019 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "arrow.h"

#include <math.h>

#define WIDTH 32
#define HEIGHT 16
#define LENGTH 11

/**
 * PhoshArrow:
 *
 * An animated arrow
 *
 * An animated arrow that initially points upward and
 * rotates downwards as `progress` increases.
 */

struct _PhoshArrow
{
  GtkDrawingArea parent_instance;

  double progress;
};

G_DEFINE_TYPE (PhoshArrow, phosh_arrow, GTK_TYPE_DRAWING_AREA)

enum {
  PROP_0,
  PROP_PROGRESS,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];


static double
interpolate_progress (double t)
{
  if (t < 1.0 / 3.0)
    return sin (t * 1.5 * G_PI) / 2.0;

  if (t > 2.0 / 3.0)
    return cos (t * 1.5 * G_PI) / 2.0 + 1;

  return 0.5;
}


/* GtkWidget */


static gboolean
phosh_arrow_draw (GtkWidget *widget,
                  cairo_t   *cr)
{
  PhoshArrow *self = PHOSH_ARROW (widget);
  double progress, angle, center_x, center_y;
  GtkStyleContext *context;
  GtkStateFlags flags;
  GdkRGBA rgba;

  progress = interpolate_progress (self->progress);

  angle = (0.5 - progress) * G_PI / 2.5;

  center_x = gtk_widget_get_allocated_width (widget) / 2.0;
  center_y = (gtk_widget_get_allocated_height (widget) / 2.0 - 0.5) * (0.5 + progress);

  context = gtk_widget_get_style_context (widget);
  flags = gtk_widget_get_state_flags (widget);
  gtk_style_context_get_color (context, flags, &rgba);

  cairo_set_line_width (cr, 3);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_source_rgba (cr, rgba.red, rgba.green, rgba.blue, rgba.alpha);

  cairo_move_to (cr, center_x, center_y);
  cairo_line_to (cr, center_x + LENGTH * cos (angle), center_y + LENGTH * sin (angle));
  cairo_stroke (cr);

  cairo_move_to (cr, center_x, center_y);
  cairo_line_to (cr, center_x - LENGTH * cos (angle), center_y + LENGTH * sin (angle));
  cairo_stroke (cr);

  return GDK_EVENT_PROPAGATE;
}


static void
phosh_arrow_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  PhoshArrow *self = PHOSH_ARROW (object);

  switch (prop_id) {
    case PROP_PROGRESS:
      g_value_set_double (value, phosh_arrow_get_progress (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
phosh_arrow_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  PhoshArrow *self = PHOSH_ARROW (object);

  switch (prop_id) {
    case PROP_PROGRESS:
      phosh_arrow_set_progress (self, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
phosh_arrow_class_init (PhoshArrowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_arrow_get_property;
  object_class->set_property = phosh_arrow_set_property;
  widget_class->draw = phosh_arrow_draw;

  properties [PROP_PROGRESS] =
    g_param_spec_double ("progress",
                         "Progress",
                         "Progress",
                         0, 1, 0,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties(object_class, N_PROPS, properties);
}


static void
phosh_arrow_init (PhoshArrow *self)
{
  self->progress = 0;
  g_object_set (self,
                "width-request", WIDTH,
                "height-request", HEIGHT,
                NULL);
}


PhoshArrow *
phosh_arrow_new (void)
{
  return g_object_new (PHOSH_TYPE_ARROW, NULL);
}


double
phosh_arrow_get_progress (PhoshArrow *self)
{
  g_return_val_if_fail (PHOSH_IS_ARROW (self), 0);

  return self->progress;
}


void
phosh_arrow_set_progress (PhoshArrow *self,
                          double      progress)
{
  g_return_if_fail (PHOSH_IS_ARROW (self));

  self->progress = progress;
  gtk_widget_queue_draw (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PROGRESS]);
}
