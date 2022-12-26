/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 *
 * Based on <hdy-fading-label.c> from <libhandy 1.5.0> which is LGPL-2.1+.
 */

#include "phosh-config.h"
#include "fading-label.h"

#include <glib/gi18n-lib.h>
#include "bidi.h"

/**
 * PhoshFadingLabel:
 *
 * A label that visually fades out when too wide for the given space.
 */

#define FADE_WIDTH 18

struct _PhoshFadingLabel
{
  GtkBin parent_instance;

  GtkWidget *label;
  gfloat align;
  cairo_pattern_t *gradient;
};

G_DEFINE_TYPE (PhoshFadingLabel, phosh_fading_label, GTK_TYPE_BIN)

enum {
  PROP_0,
  PROP_LABEL,
  PROP_ALIGN,
  LAST_PROP
};

static GParamSpec *props[LAST_PROP];

static gboolean
is_rtl (PhoshFadingLabel *self)
{
  PangoDirection pango_direction = PANGO_DIRECTION_NEUTRAL;
  const char *label = phosh_fading_label_get_label (self);

  if (label)
    pango_direction = phosh_find_base_dir (label, -1);

  if (pango_direction == PANGO_DIRECTION_RTL)
    return TRUE;

  if (pango_direction == PANGO_DIRECTION_LTR)
    return FALSE;

  return gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL;
}

static void
ensure_gradient (PhoshFadingLabel *self)
{
  if (self->gradient)
    return;

  self->gradient = cairo_pattern_create_linear (0, 0, 1, 0);
  cairo_pattern_add_color_stop_rgba (self->gradient, 0, 1, 1, 1, 0);
  cairo_pattern_add_color_stop_rgba (self->gradient, 1, 1, 1, 1, 1);
}

static void
phosh_fading_label_get_preferred_width (GtkWidget *widget,
                                        gint      *min,
                                        gint      *nat)
{
  PhoshFadingLabel *self = PHOSH_FADING_LABEL (widget);

  gtk_widget_get_preferred_width (self->label, min, nat);

  if (min)
    *min = 0;
}

static void
phosh_fading_label_get_preferred_width_for_height (GtkWidget *widget,
                                                   gint       for_height,
                                                   gint      *min,
                                                   gint      *nat)
{
  phosh_fading_label_get_preferred_width (widget, min, nat);
}

static void
phosh_fading_label_size_allocate (GtkWidget     *widget,
                                  GtkAllocation *allocation)
{
  PhoshFadingLabel *self = PHOSH_FADING_LABEL (widget);
  gfloat align = is_rtl (self) ? 1 - self->align : self->align;
  GtkAllocation child_allocation;
  gint child_width;

  gtk_widget_set_allocation (widget, allocation);

  phosh_fading_label_get_preferred_width (widget, NULL, &child_width);

  child_allocation.x = allocation->x + (gint) ((allocation->width - child_width) * align);
  child_allocation.y = allocation->y;
  child_allocation.width = child_width;
  child_allocation.height = allocation->height;

  gtk_widget_size_allocate (self->label, &child_allocation);

  gtk_widget_get_clip (self->label, &child_allocation);
  child_allocation.x = allocation->x;
  child_allocation.width = allocation->width;
  gtk_widget_set_clip (self->label, &child_allocation);
}

static gboolean
phosh_fading_label_draw (GtkWidget *widget,
                         cairo_t   *cr)
{
  PhoshFadingLabel *self = PHOSH_FADING_LABEL (widget);
  gfloat align = is_rtl (self) ? 1 - self->align : self->align;
  GtkAllocation clip, alloc;
  int child_width = gtk_widget_get_allocated_width (self->label);

  gtk_widget_get_allocation (widget, &alloc);

  if (child_width <= alloc.width) {
      gtk_container_propagate_draw (GTK_CONTAINER (widget), self->label, cr);

      return GDK_EVENT_PROPAGATE;
  }

  ensure_gradient (self);

  gtk_widget_get_clip (self->label, &clip);
  clip.x = 0;
  clip.y -= alloc.y;
  clip.width = alloc.width;

  cairo_save (cr);
  cairo_rectangle (cr, clip.x, clip.y, clip.width, clip.height);
  cairo_clip (cr);

  cairo_push_group (cr);
  gtk_container_propagate_draw (GTK_CONTAINER (widget), self->label, cr);

  if (align > 0) {
      cairo_save (cr);
      cairo_translate (cr, clip.x + FADE_WIDTH, clip.y);
      cairo_scale (cr, -FADE_WIDTH, clip.height);
      cairo_set_source (cr, self->gradient);
      cairo_rectangle (cr, 0, 0, 1, 1);
      cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OUT);
      cairo_fill (cr);
      cairo_restore (cr);
  }

  if (align < 1) {
      cairo_translate (cr, clip.x + clip.width - FADE_WIDTH, clip.y);
      cairo_scale (cr, FADE_WIDTH, clip.height);
      cairo_set_source (cr, self->gradient);
      cairo_rectangle (cr, 0, 0, 1, 1);
      cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OUT);
      cairo_fill (cr);
  }

  cairo_pop_group_to_source (cr);
  cairo_paint (cr);

  cairo_restore (cr);

  return GDK_EVENT_PROPAGATE;
}

static void
phosh_fading_label_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  PhoshFadingLabel *self = PHOSH_FADING_LABEL (object);

  switch (prop_id) {
  case PROP_LABEL:
    g_value_set_string (value, phosh_fading_label_get_label (self));
    break;

  case PROP_ALIGN:
    g_value_set_float (value, phosh_fading_label_get_align (self));
    break;

    default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
phosh_fading_label_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  PhoshFadingLabel *self = PHOSH_FADING_LABEL (object);

  switch (prop_id) {
  case PROP_LABEL:
    phosh_fading_label_set_label (self, g_value_get_string (value));
    break;

  case PROP_ALIGN:
    phosh_fading_label_set_align (self, g_value_get_float (value));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
phosh_fading_label_finalize (GObject *object)
{
  PhoshFadingLabel *self = PHOSH_FADING_LABEL (object);

  g_clear_pointer (&self->gradient, cairo_pattern_destroy);

  G_OBJECT_CLASS (phosh_fading_label_parent_class)->finalize (object);
}

static void
phosh_fading_label_class_init (PhoshFadingLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_fading_label_get_property;
  object_class->set_property = phosh_fading_label_set_property;
  object_class->finalize = phosh_fading_label_finalize;

  widget_class->get_preferred_width = phosh_fading_label_get_preferred_width;
  widget_class->get_preferred_width_for_height = phosh_fading_label_get_preferred_width_for_height;
  widget_class->size_allocate = phosh_fading_label_size_allocate;
  widget_class->draw = phosh_fading_label_draw;

  props[PROP_LABEL] =
    g_param_spec_string ("label", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_ALIGN] =
    g_param_spec_float ("align", "", "",
                        0.0, 1.0, 0.0,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
phosh_fading_label_init (PhoshFadingLabel *self)
{
  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);

  self->label = gtk_label_new (NULL);
  gtk_widget_show (self->label);
  gtk_label_set_single_line_mode (GTK_LABEL (self->label), TRUE);

  gtk_container_add (GTK_CONTAINER (self), self->label);
}

GtkWidget *
phosh_fading_label_new (const char *label)
{
  return GTK_WIDGET (g_object_new (PHOSH_TYPE_FADING_LABEL, "label", label, NULL));
}

const char *
phosh_fading_label_get_label (PhoshFadingLabel *self)
{
  g_return_val_if_fail (PHOSH_IS_FADING_LABEL (self), NULL);

  return gtk_label_get_label (GTK_LABEL (self->label));
}

void
phosh_fading_label_set_label (PhoshFadingLabel *self,
                              const char       *label)
{
  g_return_if_fail (PHOSH_IS_FADING_LABEL (self));

  if (!g_strcmp0 (label, phosh_fading_label_get_label (self)))
    return;

  gtk_label_set_label (GTK_LABEL (self->label), label);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_LABEL]);
}

float
phosh_fading_label_get_align (PhoshFadingLabel *self)
{
  g_return_val_if_fail (PHOSH_IS_FADING_LABEL (self), 0.0f);

  return self->align;
}

void
phosh_fading_label_set_align (PhoshFadingLabel *self,
                              gfloat            align)
{
  g_return_if_fail (PHOSH_IS_FADING_LABEL (self));

  align = CLAMP (align, 0.0, 1.0);

  if (!(self->align > align || self->align < align))
    return;

  self->align = align;

  gtk_widget_queue_allocate (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ALIGN]);
}
