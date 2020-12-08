/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Alexander Mikhaylenko <alexm@gnome.org>
 */

#include "config.h"

#include "animation.h"
#include "swipe-away-bin.h"
#include <handy.h>

enum {
  REMOVED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


enum {
  PROP_0,
  PROP_ORIENTATION,

  LAST_PROP = PROP_ORIENTATION
};


struct _PhoshSwipeAwayBin
{
  GtkEventBox parent_instance;

  GtkOrientation orientation;

  double progress;
  HdySwipeTracker *tracker;
  PhoshAnimation *animation;
};

static void phosh_swipe_away_bin_swipeable_init (HdySwipeableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (PhoshSwipeAwayBin, phosh_swipe_away_bin, GTK_TYPE_EVENT_BOX,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (HDY_TYPE_SWIPEABLE, phosh_swipe_away_bin_swipeable_init))


static void
set_progress (PhoshSwipeAwayBin *self,
              double             progress)
{
  self->progress = progress;

  gtk_widget_set_opacity (GTK_WIDGET (self), hdy_ease_out_cubic (1 - ABS (self->progress)));
  gtk_widget_queue_allocate (GTK_WIDGET (self));
}


static void
animation_value_cb (double             value,
                    PhoshSwipeAwayBin *self)
{
  set_progress (self, value);
}


static gboolean
animation_done_idle_cb (PhoshSwipeAwayBin *self)
{
  g_signal_emit (self, signals[REMOVED], 0);

  return G_SOURCE_REMOVE;
}


static void
animation_done_cb (PhoshSwipeAwayBin *self)
{
  g_clear_pointer (&self->animation, phosh_animation_unref);

  if (ABS (self->progress) < 1)
    return;

  g_idle_add ((GSourceFunc) animation_done_idle_cb, self);
}


static void
animate (PhoshSwipeAwayBin *self,
         gint64             duration,
         double             to,
         PhoshAnimationType type)
{
  self->animation =
    phosh_animation_new (GTK_WIDGET (self),
                         self->progress,
                         to,
                         duration,
                         type,
                         (PhoshAnimationValueCallback) animation_value_cb,
                         (PhoshAnimationDoneCallback) animation_done_cb,
                         self);

  phosh_animation_start (self->animation);
}


static void
begin_swipe_cb (PhoshSwipeAwayBin *self)
{
  if (self->animation)
    phosh_animation_stop (self->animation);
}


static void
update_swipe_cb (PhoshSwipeAwayBin *self,
                 double             progress)
{
  set_progress (self, progress);
}


static void
end_swipe_cb (PhoshSwipeAwayBin *self,
              gint64             duration,
              double             to)
{
  animate (self, duration, to, PHOSH_ANIMATION_TYPE_EASE_OUT_CUBIC);
}


static void
update_orientation (PhoshSwipeAwayBin *self)
{
  gboolean reversed =
    self->orientation == GTK_ORIENTATION_HORIZONTAL &&
    gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL;

  hdy_swipe_tracker_set_reversed (self->tracker, reversed);

  gtk_widget_queue_allocate (GTK_WIDGET (self));
}


static void
phosh_swipe_away_bin_finalize (GObject *object)
{
  PhoshSwipeAwayBin *self = PHOSH_SWIPE_AWAY_BIN (object);

  g_object_unref (self->tracker);
  g_clear_pointer (&self->animation, phosh_animation_unref);

  G_OBJECT_CLASS (phosh_swipe_away_bin_parent_class)->finalize (object);
}


static void
phosh_swipe_away_bin_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  PhoshSwipeAwayBin *self = PHOSH_SWIPE_AWAY_BIN (object);

  switch (prop_id) {
  case PROP_ORIENTATION:
    g_value_set_enum (value, self->orientation);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
phosh_swipe_away_bin_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  PhoshSwipeAwayBin *self = PHOSH_SWIPE_AWAY_BIN (object);

  switch (prop_id) {
  case PROP_ORIENTATION:
    {
      GtkOrientation orientation = g_value_get_enum (value);
      if (orientation != self->orientation) {
        self->orientation = orientation;
        update_orientation (self);
        g_object_notify (G_OBJECT (self), "orientation");
      }
    }
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
phosh_swipe_away_bin_size_allocate (GtkWidget     *widget,
                                    GtkAllocation *alloc)
{
  PhoshSwipeAwayBin *self = PHOSH_SWIPE_AWAY_BIN (widget);
  GtkWidget *child = gtk_bin_get_child (GTK_BIN (widget));
  GtkAllocation child_alloc;

  GTK_WIDGET_CLASS (phosh_swipe_away_bin_parent_class)->size_allocate (widget, alloc);

  if (!child || !gtk_widget_get_visible (child))
    return;

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL) {
    if (gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL)
      child_alloc.x = alloc->x + (int) (self->progress * alloc->width);
    else
      child_alloc.x = alloc->x - (int) (self->progress * alloc->width);

    child_alloc.y = alloc->y;
  } else {
    child_alloc.x = alloc->x;
    child_alloc.y = alloc->y - (int) (self->progress * alloc->height);
  }

  child_alloc.width = alloc->width;
  child_alloc.height = alloc->height;

  gtk_widget_size_allocate (child, &child_alloc);
}


static void
phosh_swipe_away_bin_direction_changed (GtkWidget        *widget,
                                        GtkTextDirection  previous_direction)
{
  PhoshSwipeAwayBin *self = PHOSH_SWIPE_AWAY_BIN (widget);

  update_orientation (self);
}


static void
phosh_swipe_away_bin_class_init (PhoshSwipeAwayBinClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_swipe_away_bin_finalize;
  object_class->get_property = phosh_swipe_away_bin_get_property;
  object_class->set_property = phosh_swipe_away_bin_set_property;
  widget_class->size_allocate = phosh_swipe_away_bin_size_allocate;
  widget_class->direction_changed = phosh_swipe_away_bin_direction_changed;

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  signals[REMOVED] =
    g_signal_new ("removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);
}


static void
phosh_swipe_away_bin_init (PhoshSwipeAwayBin *self)
{
  self->tracker = hdy_swipe_tracker_new (HDY_SWIPEABLE (self));

  g_object_bind_property (self, "orientation",
                          self->tracker, "orientation",
                          G_BINDING_SYNC_CREATE);
  hdy_swipe_tracker_set_allow_mouse_drag (self->tracker, TRUE);
  update_orientation (self);

  g_signal_connect_object (self->tracker, "begin-swipe",
                           G_CALLBACK (begin_swipe_cb), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->tracker, "update-swipe",
                           G_CALLBACK (update_swipe_cb), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->tracker, "end-swipe",
                           G_CALLBACK (end_swipe_cb), self,
                           G_CONNECT_SWAPPED);
}


static HdySwipeTracker *
phosh_swipe_away_bin_get_swipe_tracker (HdySwipeable *swipeable)
{
  PhoshSwipeAwayBin *self = PHOSH_SWIPE_AWAY_BIN (swipeable);

  return self->tracker;
}


static double
phosh_swipe_away_bin_get_distance (HdySwipeable *swipeable)
{
  PhoshSwipeAwayBin *self = PHOSH_SWIPE_AWAY_BIN (swipeable);

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL)
    return (double) gtk_widget_get_allocated_width (GTK_WIDGET (self));
  else
    return (double) gtk_widget_get_allocated_height (GTK_WIDGET (self));
}


static double
phosh_swipe_away_bin_get_cancel_progress (HdySwipeable *swipeable)
{
  return 0;
}


static double
phosh_swipe_away_bin_get_progress (HdySwipeable *swipeable)
{
  PhoshSwipeAwayBin *self = PHOSH_SWIPE_AWAY_BIN (swipeable);

  return self->progress;
}


static double *
phosh_swipe_away_bin_get_snap_points (HdySwipeable *swipeable,
                                      int          *n_snap_points)
{
  double *points = g_new0 (double, 2);

  points[1] = 1;

  if (n_snap_points)
    *n_snap_points = 2;

  return points;
}


static void
phosh_swipe_away_bin_switch_child (HdySwipeable *swipeable,
                                   guint         index,
                                   gint64        duration)
{
}


static void
phosh_swipe_away_bin_swipeable_init (HdySwipeableInterface *iface)
{
  iface->get_swipe_tracker = phosh_swipe_away_bin_get_swipe_tracker;
  iface->get_distance = phosh_swipe_away_bin_get_distance;
  iface->get_cancel_progress = phosh_swipe_away_bin_get_cancel_progress;
  iface->get_progress = phosh_swipe_away_bin_get_progress;
  iface->get_snap_points = phosh_swipe_away_bin_get_snap_points;
  iface->switch_child = phosh_swipe_away_bin_switch_child;
}


void
phosh_swipe_away_bin_hide (PhoshSwipeAwayBin *self)
{
  g_return_if_fail (PHOSH_IS_SWIPE_AWAY_BIN (self));

  if (self->animation)
    phosh_animation_stop (self->animation);

  set_progress (self, 1);
}


void
phosh_swipe_away_bin_reveal (PhoshSwipeAwayBin *self)
{
  g_return_if_fail (PHOSH_IS_SWIPE_AWAY_BIN (self));

  if (self->animation)
    phosh_animation_stop (self->animation);

  animate (self, 200, 0, PHOSH_ANIMATION_TYPE_EASE_OUT_CUBIC);
}


void
phosh_swipe_away_bin_remove (PhoshSwipeAwayBin *self)
{
  g_return_if_fail (PHOSH_IS_SWIPE_AWAY_BIN (self));

  if (self->animation)
    phosh_animation_stop (self->animation);

  animate (self, 200, 1, PHOSH_ANIMATION_TYPE_EASE_OUT_CUBIC);
}


void
phosh_swipe_away_bin_undo (PhoshSwipeAwayBin *self)
{
  g_return_if_fail (PHOSH_IS_SWIPE_AWAY_BIN (self));

  if (self->animation)
    phosh_animation_stop (self->animation);

  animate (self, 600, 0, PHOSH_ANIMATION_TYPE_EASE_OUT_BOUNCE);
}
