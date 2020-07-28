/* timestamp-label.c
 *
 * Copyright 2020 Lugsole
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "phosh-timestamp-label"

#include "timestamp-label.h"
#include "config.h"
#include <glib/gi18n.h>

/**
 * SECTION:timestamp-label
 * @short_description: A simple way of displaying a time difference
 * @Title: PhoshTimestampLabel
 *
 * The #PhoshTimestampLabel is used to display the time difference between
 * the timestamp stored in the #PhoshTimestampLabel and the current time.
 */


struct _PhoshTimestampLabel {
  GtkLabel   parent;
  GDateTime *date;
  guint      refresh_time;
};


enum {
  PROP_0,
  PROP_TIMESTAMP,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];


G_DEFINE_TYPE (PhoshTimestampLabel, phosh_timestamp_label, GTK_TYPE_LABEL)


#define SECONDS_PER_MINUTE 60.0
#define SECONDS_PER_HOUR   3600.0
#define SECONDS_PER_DAY    86400.0
#define SECONDS_PER_MONTH  2592000.0
#define SECONDS_PER_YEAR   31536000.0

/**
 * phosh_time_ago_in_words:
 * @time_stamp: the time to represent
 *
 * Generate a string to represent a datetime
 *
 * Based on [ChattyListRow](https://source.puri.sm/Librem5/chatty/blob/master/src/chatty-list-row.c#L47)
 * itself based on the ruby on rails method 'distance_of_time_in_words'
 *
 * Returns: (transfer full): the generated string
 */
static char *
phosh_time_ago_in_words (GDateTime *time_stamp)
{

  g_autoptr (GDateTime) time_now = g_date_time_new_now_local ();

  g_autofree char *fallback = NULL;

  const char *unit;
  const char *prefix;

  const char *str_about, *str_less_than;
  const char *str_seconds, *str_minute, *str_minutes;
  const char *str_hour, *str_hours, *str_day, *str_days;
  const char *str_month, *str_months, *str_year, *str_years;

  int number, seconds, minutes, hours, days, months, years, offset, remainder;

  gboolean show_date = FALSE;

  double dist_in_seconds;

  str_about     =   "~";
  str_less_than =   "<";
  str_seconds   = _("s");
  str_minute    = _("m");
  str_minutes   = _("m");
  str_hour      = _("h");
  str_hours     = _("h");
  str_day       = _("d");
  str_days      = _("d");
  str_month     = _("mo");
  str_months    = _("mos");
  str_year      = _("y");
  str_years     = _("y");

  /* Translators: this is the date in number only format */
  fallback = g_date_time_format (time_stamp, _("%d.%m.%y"));

  dist_in_seconds = g_date_time_difference (time_now, time_stamp) / G_TIME_SPAN_SECOND;

  seconds = (int) dist_in_seconds;
  minutes = (int) (dist_in_seconds / SECONDS_PER_MINUTE);
  hours   = (int) (dist_in_seconds / SECONDS_PER_HOUR);
  days    = (int) (dist_in_seconds / SECONDS_PER_DAY);
  months  = (int) (dist_in_seconds / SECONDS_PER_MONTH);
  years   = (int) (dist_in_seconds / SECONDS_PER_YEAR);

  switch (minutes) {
  case 0 ... 1:
    unit = str_seconds;

    switch (seconds) {
    case 0 ... 14:
      prefix = str_less_than;
      number = 15;
      break;
    case 15 ... 29:
      prefix = str_less_than;
      number = 30;
      break;
    case 30 ... 59:
      prefix = str_less_than;
      number = 1;
      unit = str_minute;
      break;
    default:
      prefix = str_about;
      number = 1;
      unit = str_minute;
      break;
    }
    break;

  case 2 ... 44:
    prefix = "";
    number = minutes;
    unit = str_minutes;
    break;
  case 45 ... 89:
    prefix = str_about;
    number = 1;
    unit = str_hour;
    break;
  case 90 ... 1439:
    prefix = str_about;
    number = hours;
    unit = str_hours;
    break;
  case 1440 ... 2529:
    prefix = str_about;
    number = 1;
    unit = str_day;
    show_date = TRUE;
    break;
  case 2530 ... 43199:
    prefix = "";
    number = days;
    unit = str_days;
    show_date = TRUE;
    break;
  case 43200 ... 86399:
    prefix = str_about;
    number = 1;
    unit = str_month;
    show_date = TRUE;
    break;
  case 86400 ... 525600:
    prefix = "";
    number = months;
    unit = str_months;
    show_date = TRUE;
    break;

  default:
    number = years;

    unit = (number == 1) ? str_year : str_years;

    offset = (int)((float)years / 4.0) * 1440.0;

    remainder = (minutes - offset) % 525600;
    show_date = TRUE;

    if (remainder < 131400) {
      prefix = str_about;
    } else if (remainder < 394200) {
      prefix = _("Over");
    } else {
      ++number;
      unit = str_years;
      prefix = _("Almost");
    }
    break;
  }

  return show_date ? g_strdup (fallback) :
    g_strdup_printf ("%s%d%s", prefix, number, unit);
}


static GTimeSpan
phosh_timestamp_label_calc_timeout (PhoshTimestampLabel *self)
{

  g_autoptr (GDateTime) time_now = g_date_time_new_now_local ();
  GDateTime *timeout_time = NULL;


  int seconds, minutes, hours, days, months;
  double dist_in_seconds;
  GTimeSpan timeout_diff;

  dist_in_seconds = g_date_time_difference (time_now, self->date) / G_TIME_SPAN_SECOND;
  seconds = (int) dist_in_seconds;
  minutes = (int) (dist_in_seconds / SECONDS_PER_MINUTE);
  hours   = (int) (dist_in_seconds / SECONDS_PER_HOUR);
  days    = (int) (dist_in_seconds / SECONDS_PER_DAY);
  months  = (int) (dist_in_seconds / SECONDS_PER_MONTH);

  switch (minutes) {
  case 0 ... 1:

    switch (seconds) {
    case 0 ... 14:
      timeout_time = g_date_time_add_seconds (self->date, 15);
      break;
    case 15 ... 29:
      timeout_time = g_date_time_add_seconds (self->date, 30);
      break;
    case 30 ... 59:
      timeout_time = g_date_time_add_minutes (self->date, 1);
      break;
    default:
      timeout_time = g_date_time_add_minutes (self->date, 2);
      break;
    }
    break;

  case 2 ... 44:
    timeout_time = g_date_time_add_minutes (self->date, minutes + 1);
    break;
  case 45 ... 89:
    timeout_time = g_date_time_add_minutes (self->date, 90);
    break;
  case 90 ... 1439:
    timeout_time = g_date_time_add_hours (self->date, hours + 1);
    break;
  case 1440 ... 43199:
    timeout_time = g_date_time_add_days (self->date, days + 1);
    break;
  case 43200 ... 525600:
    timeout_time = g_date_time_add_months (self->date, months + 1);
    break;
  default:
    timeout_time = g_date_time_add_months (self->date, months + 1);
    break;
  }
  timeout_diff = g_date_time_difference (timeout_time, time_now);
  g_debug ("time out duration = %ld", timeout_diff);
  return timeout_diff;
}


static gboolean
phosh_timestamp_label_update (PhoshTimestampLabel *self)
{
  g_autofree char *str = NULL;
  GTimeSpan time = 0;
  if (self->date != NULL) {
    str = phosh_time_ago_in_words (self ->date);
    gtk_label_set_label (GTK_LABEL (self), str);

    g_clear_handle_id (&(self->refresh_time), g_source_remove);
    time = phosh_timestamp_label_calc_timeout (self);
    self->refresh_time = g_timeout_add (time/ G_TIME_SPAN_MILLISECOND,
                                        (GSourceFunc) phosh_timestamp_label_update,
                                        self);
  } else {
    gtk_label_set_label (GTK_LABEL (self), "");

    g_clear_handle_id (&(self->refresh_time), g_source_remove);
  }
  return G_SOURCE_REMOVE;
}


static void
phosh_timestamp_label_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  PhoshTimestampLabel *self = PHOSH_TIMESTAMP_LABEL (object);

  switch (property_id) {
    case PROP_TIMESTAMP:
      g_value_set_boxed (value, phosh_timestamp_label_get_timestamp (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_timestamp_label_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PhoshTimestampLabel *self = PHOSH_TIMESTAMP_LABEL (object);

  switch (property_id) {
    case PROP_TIMESTAMP:
      phosh_timestamp_label_set_timestamp (self, g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_timestamp_label_dispose (GObject *object)
{
  PhoshTimestampLabel *self = PHOSH_TIMESTAMP_LABEL (object);

  g_clear_pointer (&self->date, g_date_time_unref);
  g_clear_handle_id (&(self->refresh_time), g_source_remove);

  G_OBJECT_CLASS (phosh_timestamp_label_parent_class)->dispose (object);
}


static void
phosh_timestamp_label_class_init (PhoshTimestampLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = phosh_timestamp_label_dispose;
  object_class->set_property = phosh_timestamp_label_set_property;
  object_class->get_property = phosh_timestamp_label_get_property;

  props[PROP_TIMESTAMP] =
    g_param_spec_boxed (
      "timestamp",
      "Timestamp",
      "The label's timestamp",
      G_TYPE_DATE_TIME,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}


static void
phosh_timestamp_label_init (PhoshTimestampLabel *self)
{
  PangoAttrList *attrs = pango_attr_list_new ();

  pango_attr_list_insert (attrs, pango_attr_font_features_new ("tnum=1"));
  gtk_label_set_attributes (GTK_LABEL (self), attrs);

  g_clear_pointer (&attrs, pango_attr_list_unref);
}


PhoshTimestampLabel *
phosh_timestamp_label_new (void)
{
  return g_object_new (PHOSH_TYPE_TIMESTAMP_LABEL, NULL);
}


GDateTime *
phosh_timestamp_label_get_timestamp (PhoshTimestampLabel *self)
{
  g_return_val_if_fail (PHOSH_IS_TIMESTAMP_LABEL (self), NULL);
  return self->date;
}


void
phosh_timestamp_label_set_timestamp (PhoshTimestampLabel *self,
                                     GDateTime           *date)
{
  g_return_if_fail (PHOSH_IS_TIMESTAMP_LABEL (self));

  g_debug ("notification setting timestamp %d %d", self->date == NULL , date == NULL);

  if (self->date && date && g_date_time_compare (self->date, date) == 0)
    return;

  g_clear_pointer (&self->date, g_date_time_unref);
  if (date != NULL) {
    self->date = g_date_time_ref (date);
  }
  phosh_timestamp_label_update (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TIMESTAMP]);
}
