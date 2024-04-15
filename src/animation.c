/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Alexander Mikhaylenko <alexm@gnome.org>
 */

#include "phosh-config.h"

#include "animation.h"
#include <handy.h>

G_DEFINE_BOXED_TYPE (PhoshAnimation, phosh_animation, phosh_animation_ref, phosh_animation_unref)

struct _PhoshAnimation
{
  gatomicrefcount ref_count;

  GtkWidget *widget;

  double value;

  double value_from;
  double value_to;
  gint64 duration;
  PhoshAnimationType type;

  gint64 start_time;
  guint tick_cb_id;

  PhoshAnimationValueCallback value_cb;
  PhoshAnimationDoneCallback done_cb;
  gpointer user_data;
};

static void
set_value (PhoshAnimation *self,
           double          value)
{
  self->value = value;
  self->value_cb (value, self->user_data);
}

#define LERP(a, b, t) (a) * (1.0 - (t)) + (b) * (t)

/* Adapted from https://github.com/janrembold/es6-easings/blob/master/src/index.ts#L135 */
/* TODO: Move to libhandy at some point */
static double
ease_out_bounce (double t)
{
  double p;

  if (t < 1.0 / 2.75)
    return 7.5625 * t * t;

  if (t < 2.0 / 2.75) {
    p = t - (1.5 / 2.75);

    return 7.5625 * p * p + 0.75;
  }

  if (t < 2.5 / 2.75) {
    p = t - (2.25 / 2.75);

    return 7.5625 * p * p + 0.9375;
  }

  p = t - (2.625 / 2.75);

  return 7.5625 * p * p + 0.984375;
}


static double
ease_in_quintic (double t)
{
  return t * t * t * t * t;
}


static double
ease_out_quintic (double t)
{
  double p = t - 1;

  return p * p * p * p * p + 1;
}


static inline double
interpolate (PhoshAnimationType type, double t)
{
  switch (type) {
  case PHOSH_ANIMATION_TYPE_EASE_OUT_CUBIC:
    return hdy_ease_out_cubic (t);

  case PHOSH_ANIMATION_TYPE_EASE_IN_QUINTIC:
    return ease_in_quintic (t);

  case PHOSH_ANIMATION_TYPE_EASE_OUT_QUINTIC:
    return ease_out_quintic (t);

  case PHOSH_ANIMATION_TYPE_EASE_OUT_BOUNCE:
    return ease_out_bounce (t);

  default:
    g_assert_not_reached ();
  }
}

static gboolean
tick_cb (GtkWidget       *widget,
         GdkFrameClock   *frame_clock,
         PhoshAnimation  *self)
{
  gint64 frame_time = gdk_frame_clock_get_frame_time (frame_clock) / 1000;
  double t = (double) (frame_time - self->start_time) / self->duration;

  if (t >= 1) {
    self->tick_cb_id = 0;

    set_value (self, self->value_to);

    g_signal_handlers_disconnect_by_func (self->widget, phosh_animation_stop, self);

    self->done_cb (self->user_data);

    return G_SOURCE_REMOVE;
  }

  set_value (self, LERP (self->value_from, self->value_to, interpolate (self->type, t)));

  return G_SOURCE_CONTINUE;
}

static void
phosh_animation_free (PhoshAnimation *self)
{
  phosh_animation_stop (self);

  g_slice_free (PhoshAnimation, self);
}

/**
 * phosh_animation_new:
 * @widget: A widget
 * @from: The animation's start value
 * @to: The animation's end value
 * @type: The type of animation
 * @value_cb:(scope forever): The callback applying `value`
 * @done_cb:(scope forever): The callback invoked when the animation is done
 * @user_data: user_data passed to `value_cb` and `done_cb`
 *
 * Get a new animation object for @widget.
 *
 * Note that the scope of the `value_cb` and `done_cb` callbacks is
 * actually as long as the animation exists.
 *
 * Returns:(transfer full): The animation
 */
PhoshAnimation *
phosh_animation_new (GtkWidget                   *widget,
                     double                       from,
                     double                       to,
                     gint64                       duration,
                     PhoshAnimationType           type,
                     PhoshAnimationValueCallback  value_cb,
                     PhoshAnimationDoneCallback   done_cb,
                     gpointer                     user_data)
{
  PhoshAnimation *self;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (value_cb != NULL, NULL);
  g_return_val_if_fail (done_cb != NULL, NULL);

  self = g_slice_new0 (PhoshAnimation);

  g_atomic_ref_count_init (&self->ref_count);

  self->widget = widget;
  self->value_from = from;
  self->value_to = to;
  self->duration = duration;
  self->type = type;
  self->value_cb = value_cb;
  self->done_cb = done_cb;
  self->user_data = user_data;

  self->value = from;

  return self;
}

PhoshAnimation *
phosh_animation_ref (PhoshAnimation *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ref_count, NULL);

  g_atomic_ref_count_inc (&self->ref_count);

  return self;
}

void
phosh_animation_unref (PhoshAnimation *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ref_count);

  if (g_atomic_ref_count_dec (&self->ref_count))
    phosh_animation_free (self);
}

void
phosh_animation_start (PhoshAnimation *self)
{
  g_return_if_fail (self != NULL);

  if (!hdy_get_enable_animations (self->widget) ||
      !gtk_widget_get_mapped (self->widget) ||
      self->duration <= 0) {
    set_value (self, self->value_to);

    self->done_cb (self->user_data);

    return;
  }

  if (self->tick_cb_id)
    gtk_widget_remove_tick_callback (self->widget, self->tick_cb_id);
  else
    g_signal_connect_swapped (self->widget, "unmap",
                              G_CALLBACK (phosh_animation_stop), self);

  self->start_time = gdk_frame_clock_get_frame_time (gtk_widget_get_frame_clock (self->widget)) / 1000;
  self->tick_cb_id = gtk_widget_add_tick_callback (self->widget, (GtkTickCallback) tick_cb, self, NULL);
}

void
phosh_animation_stop (PhoshAnimation *self)
{
  g_return_if_fail (self != NULL);

  if (!self->tick_cb_id)
    return;

  gtk_widget_remove_tick_callback (self->widget, self->tick_cb_id);
  self->tick_cb_id = 0;

  g_signal_handlers_disconnect_by_func (self->widget, phosh_animation_stop, self);

  self->done_cb (self->user_data);
}

gdouble
phosh_animation_get_value (PhoshAnimation *self)
{
  g_return_val_if_fail (self != NULL, 0.0);

  return self->value;
}
