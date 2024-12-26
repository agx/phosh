/*
 * Copyright (C) 2024 Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-fake-clock"

#include "phosh-config.h"

#include "fake-clock.h"

/**
 * PhoshWallClockMock:
 *
 * A wall clock that fakes a constant date and time. The provided date and time
 * are determined from `fake-offset`.
 */

enum {
  PROP_0,
  PROP_FAKE_OFFSET,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshFakeClock {
  PhoshWallClock parent;

  GDateTime     *fake_offset;
  char          *fake_date_time;
  char          *fake_time;

  GSettings     *settings;
};
G_DEFINE_TYPE (PhoshFakeClock, phosh_fake_clock, PHOSH_TYPE_WALL_CLOCK)


static const char *
get_clock (PhoshWallClock *wall_clock, gboolean time_only)
{
  PhoshFakeClock *self = PHOSH_FAKE_CLOCK (wall_clock);

  return time_only ? self->fake_time : self->fake_date_time;
}


static gint64
get_time_t (PhoshWallClock *wall_clock)
{
  PhoshFakeClock *self = PHOSH_FAKE_CLOCK (wall_clock);

  return g_date_time_to_unix (self->fake_offset);
}


static void
update_clocks (PhoshFakeClock *self)
{
  GDesktopClockFormat clock_format;
  gboolean show_date;

  clock_format = g_settings_get_enum (self->settings, "clock-format");
  show_date = g_settings_get_boolean (self->settings, "clock-show-date");

  /* Show date and time or only time based on gsettings */
  self->fake_date_time = phosh_wall_clock_string_for_datetime (PHOSH_WALL_CLOCK (self),
                                                               self->fake_offset,
                                                               clock_format,
                                                               show_date);
  /* This clock is always time only */
  self->fake_time = phosh_wall_clock_string_for_datetime (PHOSH_WALL_CLOCK (self),
                                                          self->fake_offset,
                                                          clock_format,
                                                          FALSE);
}


static void
set_fake_offset (PhoshFakeClock *self, GDateTime *fake_offset)
{
  self->fake_offset = g_date_time_ref (fake_offset);
  update_clocks (self);
}


static void
on_settings_changed (PhoshFakeClock *self, const char *key, GSettings *settings)
{
  if (g_strcmp0 (key, "clock-format") && g_strcmp0 (key, "clock-show-date") != 0)
    return;

  update_clocks (self);
}


static void
phosh_fake_clock_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  PhoshFakeClock *self = PHOSH_FAKE_CLOCK (object);

  switch (property_id) {
  case PROP_FAKE_OFFSET:
    set_fake_offset (self, g_value_get_boxed (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_fake_clock_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PhoshFakeClock *self = PHOSH_FAKE_CLOCK (object);

  switch (property_id) {
  case PROP_FAKE_OFFSET:
    g_value_set_boxed (value, self->fake_offset);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_fake_clock_dispose (GObject *object)
{
  PhoshFakeClock *self = PHOSH_FAKE_CLOCK (object);

  g_clear_object (&self->settings);

  g_clear_pointer (&self->fake_offset, g_date_time_unref);
  g_clear_pointer (&self->fake_date_time, g_free);
  g_clear_pointer (&self->fake_time, g_free);

  G_OBJECT_CLASS (phosh_fake_clock_parent_class)->dispose (object);
}


static void
phosh_fake_clock_class_init (PhoshFakeClockClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhoshWallClockClass *wall_clock_class = PHOSH_WALL_CLOCK_CLASS (klass);

  object_class->get_property = phosh_fake_clock_get_property;
  object_class->set_property = phosh_fake_clock_set_property;
  object_class->dispose = phosh_fake_clock_dispose;

  wall_clock_class->get_clock = get_clock;
  wall_clock_class->get_time_t = get_time_t;

  /**
   * PhoshFakeClock:fake-offset:
   *
   * The faked offset from the start of the epoch this clock represents.
   */
  props[PROP_FAKE_OFFSET] =
    g_param_spec_boxed ("fake-offset", "", "",
                        G_TYPE_DATE_TIME,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_fake_clock_init (PhoshFakeClock *self)
{
  self->settings = g_settings_new ("org.gnome.desktop.interface");

  g_signal_connect_swapped (self->settings, "changed", G_CALLBACK (on_settings_changed), self);
}


PhoshFakeClock *
phosh_fake_clock_new (GDateTime *fake_offset)
{
  return g_object_new (PHOSH_TYPE_FAKE_CLOCK, "fake-offset", fake_offset, NULL);
}
