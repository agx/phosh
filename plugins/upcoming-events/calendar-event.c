/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-config.h"

#include "calendar-event.h"

enum {
  PROP_0,
  PROP_ID,
  PROP_SUMMARY,
  PROP_BEGIN,
  PROP_END,
  PROP_COLOR,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

/**
 * PhoshCalendarEvent:
 *
 * The data for an event in a calendar
 */
struct _PhoshCalendarEvent {
  GObject    parent;

  char      *id;
  char      *summary;
  GDateTime *begin;
  GDateTime *end;
  char      *color;
};
G_DEFINE_TYPE (PhoshCalendarEvent, phosh_calendar_event, G_TYPE_OBJECT)


static void
phosh_calendar_event_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  PhoshCalendarEvent *self = PHOSH_CALENDAR_EVENT (object);

  switch (property_id) {
  case PROP_ID:
    /* construct only */
    self->id = g_value_dup_string (value);
    break;
  case PROP_SUMMARY:
    g_free (self->summary);
    self->summary = g_value_dup_string (value);
    break;
  case PROP_BEGIN:
    g_clear_pointer (&self->begin, g_date_time_unref);
    self->begin = g_value_dup_boxed (value);
    break;
  case PROP_END:
    g_clear_pointer (&self->end, g_date_time_unref);
    self->end = g_value_dup_boxed (value);
    break;
  case PROP_COLOR:
    g_free (self->color);
    self->color = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_calendar_event_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  PhoshCalendarEvent *self = PHOSH_CALENDAR_EVENT (object);

  switch (property_id) {
  case PROP_ID:
    g_value_set_string (value, self->id);
    break;
  case PROP_SUMMARY:
    g_value_set_string (value, self->summary);
    break;
  case PROP_BEGIN:
    g_value_set_boxed (value, self->begin);
    break;
  case PROP_END:
    g_value_set_boxed (value, self->end);
    break;
  case PROP_COLOR:
    g_value_set_string (value, self->color);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_calendar_event_finalize (GObject *object)
{
  PhoshCalendarEvent *self = PHOSH_CALENDAR_EVENT (object);

  g_clear_pointer (&self->id, g_free);
  g_clear_pointer (&self->summary, g_free);
  g_clear_pointer (&self->begin, g_date_time_unref);
  g_clear_pointer (&self->end, g_date_time_unref);
  g_clear_pointer (&self->color, g_free);

  G_OBJECT_CLASS (phosh_calendar_event_parent_class)->finalize (object);
}


static void
phosh_calendar_event_class_init (PhoshCalendarEventClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phosh_calendar_event_get_property;
  object_class->set_property = phosh_calendar_event_set_property;
  object_class->finalize = phosh_calendar_event_finalize;

  props[PROP_ID] =
    g_param_spec_string ("id", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_SUMMARY] =
    g_param_spec_string ("summary", "", "",
                         NULL,
                         G_PARAM_READWRITE);

  props[PROP_BEGIN] =
    g_param_spec_boxed ("begin", "", "",
                        G_TYPE_DATE_TIME,
                        G_PARAM_READWRITE);

  props[PROP_END] =
    g_param_spec_boxed ("end", "", "",
                        G_TYPE_DATE_TIME,
                        G_PARAM_READWRITE);

  props[PROP_COLOR] =
    g_param_spec_string ("color", "", "",
                         NULL,
                         G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_calendar_event_init (PhoshCalendarEvent *self)
{
}


PhoshCalendarEvent *
phosh_calendar_event_new (const char *id,
                          const char *summary,
                          GDateTime  *begin,
                          GDateTime  *end,
                          const char *color)
{
  return PHOSH_CALENDAR_EVENT (g_object_new (PHOSH_TYPE_CALENDAR_EVENT,
                                             "id", id,
                                             "summary", summary,
                                             "begin", begin,
                                             "end", end,
                                             "color", color,
                                             NULL));
}


const char *
phosh_calendar_event_get_id (PhoshCalendarEvent *self)
{
  g_return_val_if_fail (PHOSH_IS_CALENDAR_EVENT (self), NULL);

  return self->id;
}

const char *
phosh_calendar_event_get_summary (PhoshCalendarEvent *self)
{
  g_return_val_if_fail (PHOSH_IS_CALENDAR_EVENT (self), NULL);

  return self->summary;
}

GDateTime *
phosh_calendar_event_get_begin (PhoshCalendarEvent *self)
{
  g_return_val_if_fail (PHOSH_IS_CALENDAR_EVENT (self), NULL);

  return self->begin;
}

GDateTime *
phosh_calendar_event_get_end (PhoshCalendarEvent *self)
{
  g_return_val_if_fail (PHOSH_IS_CALENDAR_EVENT (self), NULL);

  return self->end;
}

const char *
phosh_calendar_event_get_color (PhoshCalendarEvent *self)
{
  g_return_val_if_fail (PHOSH_IS_CALENDAR_EVENT (self), 0);

  return self->color;
}
