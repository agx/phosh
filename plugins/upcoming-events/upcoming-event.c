/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-config.h"

#include "upcoming-event.h"

#include <glib/gi18n.h>

enum {
  PROP_0,
  PROP_SUMMARY,
  PROP_BEGIN,
  PROP_END,
  PROP_COLOR,
  PROP_IS_24H,
  PROP_FOR_DAY,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

#define COLOR_BAR_CSS "phosh-upcoming-events phosh-upcoming-event .color-bar {" \
                      "   background-color: %s;                               " \
                      "}                                                      "

struct _PhoshUpcomingEvent {
  GtkBox          parent;

  GtkLabel       *lbl_begin;
  GtkDrawingArea *color_bar;
  GtkLabel       *lbl_summary;

  GDateTime      *begin;
  GDateTime      *end;
  GDateTime      *for_day;

  char           *color;
  GtkCssProvider *color_css;
  gboolean        is_24h;
};
G_DEFINE_TYPE (PhoshUpcomingEvent, phosh_upcoming_event, GTK_TYPE_BOX)


static void
phosh_upcoming_event_format_time (PhoshUpcomingEvent *self, GString *string, GDateTime *datetime)
{
  g_autofree char *str = NULL;

  if (self->is_24h) {
    /* Translators: This is the time format used in 24-hour mode. */
    str = g_date_time_format (datetime, _("%R"));
  } else {
    /* Translators: This is the time format used in 12-hour mode. */
    str = g_date_time_format (datetime, _("%l:%M %p"));
  }

  g_string_append (string, str);
}


static gboolean
on_same_day (GDateTime *dt1, GDateTime *dt2)
{
  return (g_date_time_get_year (dt1) == g_date_time_get_year (dt2) &&
          g_date_time_get_month (dt1) == g_date_time_get_month (dt2) &&
          g_date_time_get_day_of_month (dt1) == g_date_time_get_day_of_month (dt2));
}

/**
 * phosh_upcoming_event_starts_day_before:
 * @self: The event
 * @self: The `GDateTime` to compare to
 *
 * Returns: %TRUE if the events starts on a day before the day of `dt`.
 */
static gboolean
phosh_upcoming_event_starts_before (PhoshUpcomingEvent *self, GDateTime *dt)
{
  return (on_same_day (self->begin, dt) == FALSE &&
          g_date_time_get_year (self->begin) <= g_date_time_get_year (dt) &&
          g_date_time_get_month (self->begin) <= g_date_time_get_month (dt) &&
          g_date_time_get_day_of_month (self->begin) <= g_date_time_get_day_of_month (dt));
}

/**
 * phosh_upcoming_event_ends_day_after:
 * @self: The event
 * @self: The `GDateTime` to compare to
 *
 * Returns: %TRUE if the events ends on a day after the day of `dt`.
 */
static gboolean
phosh_upcoming_event_ends_after (PhoshUpcomingEvent *self, GDateTime *dt)
{
  return (on_same_day (self->begin, dt) == FALSE &&
          g_date_time_get_year (self->end) >= g_date_time_get_year (dt) &&
          g_date_time_get_month (self->end) >= g_date_time_get_month (dt) &&
          g_date_time_get_day_of_month (self->end) >= g_date_time_get_day_of_month (dt));
}


static void
phosh_upcoming_event_update_timespec (PhoshUpcomingEvent *self)
{
  g_autoptr (GString) string = g_string_new (NULL);

  if (self->begin == NULL || self->end == NULL || self->for_day == NULL)
    return;

  /* All day event, the day we're at doesn't matter */
  if (g_date_time_get_hour (self->begin) == 0 &&
      g_date_time_get_minute (self->begin) == 0 &&
      g_date_time_get_hour (self->end) == 0 &&
      g_date_time_get_minute (self->end) == 0) {

    /* Translators: An all day event */
    g_string_append (string, _("All day"));

    gtk_label_set_label (self->lbl_begin, string->str);
    return;
  }

  /* Event starts and ends on same day */
  if (on_same_day (self->begin, self->for_day) &&
      on_same_day (self->end, self->for_day)) {
    phosh_upcoming_event_format_time (self, string, self->begin);
    g_string_append (string, "\r");
    phosh_upcoming_event_format_time (self, string, self->end);
    gtk_label_set_label (self->lbl_begin, string->str);
    return;
  }

  /* Event starts on for_day */
  if (on_same_day (self->begin, self->for_day)) {
    phosh_upcoming_event_format_time (self, string, self->begin);
    gtk_label_set_label (self->lbl_begin, string->str);
    return;
  }

  /* Event ends on for_day */
  if (on_same_day (self->end, self->for_day)) {
    /* Translators: When the event ends: Ends\r16:00 */
    g_string_append (string, _("Ends"));
    g_string_append (string, "\r");
    phosh_upcoming_event_format_time (self, string, self->end);
    gtk_label_set_label (self->lbl_begin, string->str);
    return;
  }

  /* Event starts before for_day and ends after for_day */
  if (phosh_upcoming_event_starts_before (self, self->for_day) &&
      phosh_upcoming_event_ends_after (self, self->for_day)) {
    /* Translators: An all day event */
    g_string_append (string, _ ("All day"));
    gtk_label_set_label (self->lbl_begin, string->str);
    return;
  }
}


static void
phosh_upcoming_event_set_color (PhoshUpcomingEvent *self, const char *color)
{
  g_autofree char* css = NULL;
  g_autoptr (GError) err = NULL;
  g_autofree char *colorstr = NULL;
  GdkRGBA rgba;

  if (g_strcmp0 (self->color, color) == 0)
    return;

  g_free (self->color);
  self->color = g_strdup (color);

  if (gdk_rgba_parse (&rgba, color) == FALSE)
    rgba.red = rgba.green = rgba.blue = 1.0;

  colorstr = gdk_rgba_to_string (&rgba);
  css = g_strdup_printf (COLOR_BAR_CSS, colorstr);
  if (gtk_css_provider_load_from_data (self->color_css, css, -1, &err) == FALSE) {
    g_warning ("Failed to load css: %s", err->message);
    return;
  }
}


static void
phosh_upcoming_event_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  PhoshUpcomingEvent *self = PHOSH_UPCOMING_EVENT (object);

  switch (property_id) {
  case PROP_SUMMARY:
    phosh_upcoming_event_set_summary (self, g_value_get_string (value));
    break;
  case PROP_BEGIN:
    g_clear_pointer (&self->begin, g_date_time_unref);
    self->begin = g_value_dup_boxed (value);
    phosh_upcoming_event_update_timespec (self);
    break;
  case PROP_END:
    g_clear_pointer (&self->end, g_date_time_unref);
    self->end = g_value_dup_boxed (value);
    phosh_upcoming_event_update_timespec (self);
    break;
  case PROP_FOR_DAY:
    g_clear_pointer (&self->for_day, g_date_time_unref);
    self->for_day = g_value_dup_boxed (value);
    phosh_upcoming_event_update_timespec (self);
    break;
  case PROP_COLOR:
    phosh_upcoming_event_set_color (self, g_value_get_string (value));
    break;
  case PROP_IS_24H:
    self->is_24h = g_value_get_boolean (value);
    phosh_upcoming_event_update_timespec (self);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_upcoming_event_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  PhoshUpcomingEvent *self = PHOSH_UPCOMING_EVENT (object);

  switch (property_id) {
  case PROP_SUMMARY:
    g_value_set_string (value, gtk_label_get_label (self->lbl_summary));
    break;
  case PROP_BEGIN:
    g_value_set_boxed (value, self->begin);
    break;
  case PROP_END:
    g_value_set_boxed (value, self->end);
    break;
  case PROP_FOR_DAY:
    g_value_set_boxed (value, self->for_day);
    break;
  case PROP_COLOR:
    g_value_set_string (value, self->color);
    break;
  case PROP_IS_24H:
    g_value_set_boolean (value, self->is_24h);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_upcoming_event_finalize (GObject *object)
{
  PhoshUpcomingEvent *self = PHOSH_UPCOMING_EVENT (object);

  g_clear_pointer (&self->begin, g_date_time_unref);
  g_clear_pointer (&self->end, g_date_time_unref);
  g_clear_pointer (&self->for_day, g_date_time_unref);
  g_clear_pointer (&self->color, g_free);
  g_clear_object (&self->color_css);

  G_OBJECT_CLASS (phosh_upcoming_event_parent_class)->finalize (object);
}


static void
phosh_upcoming_event_class_init (PhoshUpcomingEventClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_upcoming_event_get_property;
  object_class->set_property = phosh_upcoming_event_set_property;
  object_class->finalize = phosh_upcoming_event_finalize;

  /**
   * PhoshUpcomingEvent:summary:
   *
   * A summary what the event is about
   */
  props[PROP_SUMMARY] =
    g_param_spec_string ("summary", "", "",
                         NULL,
                         G_PARAM_READWRITE);
  /**
   * PhoshUpcomingEvent:start:
   *
   * The date and time the event starts at.
   */
  props[PROP_BEGIN] =
    g_param_spec_boxed ("begin", "", "",
                        G_TYPE_DATE_TIME,
                        G_PARAM_READWRITE);
  /**
   * PhoshUpcomingEvent:end:
   *
   * The date and time the event ends at.
   */
  props[PROP_END] =
    g_param_spec_boxed ("end", "", "",
                        G_TYPE_DATE_TIME,
                        G_PARAM_READWRITE);

  /**
   * PhoshUpcomingEvent:for-day:
   *
   * The day this entry is rendered for.
   */
  props[PROP_FOR_DAY] =
    g_param_spec_boxed ("for-day", "", "",
                        G_TYPE_DATE_TIME,
                        G_PARAM_READWRITE);
  /**
   * PhoshUpcomingEvent:for-day:
   *
   * The events color as parsed by `gdk_rgba_parse()`.
   */
  props[PROP_COLOR] =
    g_param_spec_string ("color", "", "",
                         NULL,
                         G_PARAM_READWRITE);
  /**
   * PhoshUpcomingEvent:is-24h
   *
   * Whether to use 24h clock format
   */
  props[PROP_IS_24H] =
    g_param_spec_boolean ("is-24h", "", "",
                          FALSE,
                          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/plugins/upcoming-events/upcoming-event.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshUpcomingEvent, color_bar);
  gtk_widget_class_bind_template_child (widget_class, PhoshUpcomingEvent, lbl_begin);
  gtk_widget_class_bind_template_child (widget_class, PhoshUpcomingEvent, lbl_summary);

  gtk_widget_class_set_css_name (widget_class, "phosh-upcoming-event");
}


static void
phosh_upcoming_event_init (PhoshUpcomingEvent *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->color_css = gtk_css_provider_new ();
  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (self->color_bar)),
                                  GTK_STYLE_PROVIDER (self->color_css),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);
  self->is_24h = TRUE;
}


GtkWidget *
phosh_upcoming_event_new (const char *summary,
                          GDateTime  *begin,
                          GDateTime  *end,
                          GDateTime  *for_day,
                          const char *color,
                          gboolean    is_24h)
{
  return GTK_WIDGET (g_object_new (PHOSH_TYPE_UPCOMING_EVENT,
                                   "summary", summary,
                                   "begin", begin,
                                   "end", end,
                                   "color", color,
                                   "for-day", for_day,
                                   "is-24h", is_24h,
                                   NULL));
}


void
phosh_upcoming_event_set_summary (PhoshUpcomingEvent *self, const char *summary)
{
  g_return_if_fail (PHOSH_IS_UPCOMING_EVENT (self));

  if (summary && summary[0] != '\0')
    gtk_label_set_label (self->lbl_summary, summary);
  else
    gtk_label_set_label (self->lbl_summary, _("Untitled event"));
}
