/*
 * Copyright (C) 2024 Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "testlib-wall-clock-mock"

#include "phosh-config.h"

#include "testlib-wall-clock-mock.h"

/**
 * TestlibWallClockMock:
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

struct _TestlibWallClockMock {
  PhoshWallClock parent;

  GDateTime     *fake_offset;
  char          *fake_date_time;
  char          *fake_time;
};
G_DEFINE_TYPE (TestlibWallClockMock, testlib_wall_clock_mock, PHOSH_TYPE_WALL_CLOCK)


static const char *
get_clock (PhoshWallClock *wall_clock, gboolean time_only)
{
  TestlibWallClockMock *self = TESTLIB_WALL_CLOCK_MOCK (wall_clock);

  return time_only ? self->fake_time : self->fake_date_time;
}


static gint64
get_time_t (PhoshWallClock *wall_clock)
{
  TestlibWallClockMock *self = TESTLIB_WALL_CLOCK_MOCK (wall_clock);

  return g_date_time_to_unix (self->fake_offset);
}


static void
set_fake_offset (TestlibWallClockMock *self, GDateTime *fake_offset)
{
  self->fake_offset = g_date_time_ref (fake_offset);
  self->fake_date_time = phosh_wall_clock_string_for_datetime (PHOSH_WALL_CLOCK (self),
                                                               fake_offset,
                                                               TRUE,
                                                               G_DESKTOP_CLOCK_FORMAT_24H);
  self->fake_time = phosh_wall_clock_string_for_datetime (PHOSH_WALL_CLOCK (self),
                                                          fake_offset,
                                                          FALSE,
                                                          G_DESKTOP_CLOCK_FORMAT_24H);
}


static void
testlib_wall_clock_mock_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  TestlibWallClockMock *self = TESTLIB_WALL_CLOCK_MOCK (object);

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
testlib_wall_clock_mock_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  TestlibWallClockMock *self = TESTLIB_WALL_CLOCK_MOCK (object);

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
testlib_wall_clock_mock_dispose (GObject *object)
{
  TestlibWallClockMock *self = TESTLIB_WALL_CLOCK_MOCK (object);

  g_clear_pointer (&self->fake_offset, g_date_time_unref);
  g_clear_pointer (&self->fake_date_time, g_free);
  g_clear_pointer (&self->fake_time, g_free);

  G_OBJECT_CLASS (testlib_wall_clock_mock_parent_class)->dispose (object);
}


static void
testlib_wall_clock_mock_class_init (TestlibWallClockMockClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhoshWallClockClass *wall_clock_class = PHOSH_WALL_CLOCK_CLASS (klass);

  object_class->get_property = testlib_wall_clock_mock_get_property;
  object_class->set_property = testlib_wall_clock_mock_set_property;
  object_class->dispose = testlib_wall_clock_mock_dispose;

  wall_clock_class->get_clock = get_clock;
  wall_clock_class->get_time_t = get_time_t;

  /**
   * TestlibWallClockMock:fake-offset:
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
testlib_wall_clock_mock_init (TestlibWallClockMock *self)
{
}


TestlibWallClockMock *
testlib_wall_clock_mock_new (GDateTime *fake_offset)
{
  return g_object_new (TESTLIB_TYPE_WALL_CLOCK_MOCK, "fake-offset", fake_offset, NULL);
}
