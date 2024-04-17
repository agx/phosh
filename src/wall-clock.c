/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <locale.h>
#include <glib/gi18n.h>

#include "wall-clock.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-wall-clock.h>

enum {
  PROP_0,
  PROP_DATE_TIME,
  PROP_TIME,
  LAST_PROP,
};
static GParamSpec *props[LAST_PROP];

typedef struct _PhoshWallClock {
  GObject         parent;
  GnomeWallClock *time;
  GnomeWallClock *date_time;
} PhoshWallClock;

G_DEFINE_TYPE (PhoshWallClock, phosh_wall_clock, G_TYPE_OBJECT)

static void
on_date_time_changed (PhoshWallClock *self, GParamSpec *pspec, GnomeWallClock *clock)
{
  g_return_if_fail (PHOSH_IS_WALL_CLOCK (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_DATE_TIME]);
}

static void
on_time_changed (PhoshWallClock *self, GParamSpec *pspec, GnomeWallClock *clock)
{
  g_return_if_fail (PHOSH_IS_WALL_CLOCK (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TIME]);
}

static void
phosh_wall_clock_init (PhoshWallClock *self)
{
  self->date_time = gnome_wall_clock_new ();
  self->time = g_object_new (GNOME_TYPE_WALL_CLOCK, "time-only", TRUE, NULL);

  /* Somewhat icky, we need a distinct handler for each clock because one can
   * update before the other, and we don't know which one the caller wants to
   * read from.
   */
  g_signal_connect_object (self->date_time, "notify::clock", G_CALLBACK (on_date_time_changed),
                           self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->time, "notify::clock", G_CALLBACK (on_time_changed),
                           self, G_CONNECT_SWAPPED);
}


static void
phosh_wall_clock_dispose (GObject *object)
{
  PhoshWallClock *self = PHOSH_WALL_CLOCK (object);

  g_clear_object (&self->date_time);
  g_clear_object (&self->time);

  G_OBJECT_CLASS (phosh_wall_clock_parent_class)->dispose (object);
}


static void
phosh_wall_clock_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PhoshWallClock *self = PHOSH_WALL_CLOCK (object);

  switch (property_id) {
  case PROP_DATE_TIME:
    g_value_set_string (value, phosh_wall_clock_get_clock (self, FALSE));
    break;
  case PROP_TIME:
    g_value_set_string (value, phosh_wall_clock_get_clock (self, TRUE));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_wall_clock_class_init (PhoshWallClockClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = phosh_wall_clock_dispose;
  object_class->get_property = phosh_wall_clock_get_property;

  props[PROP_DATE_TIME] =
    g_param_spec_string ("date-time", "", "",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  props[PROP_TIME] =
    g_param_spec_string ("time", "", "",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}


/**
 * phosh_wall_clock_get_default:
 *
 * Get the wall clock singleton
 *
 * Returns:(transfer none): The wall clock singleton
 */
PhoshWallClock*
phosh_wall_clock_get_default (void)
{
  static PhoshWallClock *instance;

  if (instance == NULL) {
    instance = g_object_new (PHOSH_TYPE_WALL_CLOCK, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
  }
  return instance;
}


/**
 * phosh_wall_clock_get_clock:
 * @time_only: whether to return full clock string or just the time
 *
 * Gets the current clock string, if time_only is true this will be just the
 * current time, otherwise the date + time.
 *
 * Returns: the clock time string
 */
const char *
phosh_wall_clock_get_clock (PhoshWallClock *self, gboolean time_only)
{
  g_return_val_if_fail (PHOSH_IS_WALL_CLOCK (self), "");

  return gnome_wall_clock_get_clock (time_only ? self->time : self->date_time);
}
