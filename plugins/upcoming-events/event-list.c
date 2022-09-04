/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-config.h"

#include "calendar-event.h"
#include "event-list.h"
#include "gtkfilterlistmodel.h"
#include "upcoming-event.h"

#include <glib/gi18n.h>

enum {
  PROP_0,
  PROP_LABEL,
  PROP_DAY_OFFSET,
  PROP_TODAY,
  PROP_MODEL,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


/**
 * PhoshEventList:
 *
 * A widget that shows a list of events extracted from
 * `GListModel` of `PhoshCalendarEvents` that are valid
 * on `for_day`.
 *
 * TODO: we curently only support a single day but could
 * handle any time range rather by looking at time offsets.
 */
struct _PhoshEventList {
  GtkBox              parent;

  GtkListBox         *lb_events;
  GtkLabel           *label;

  GListModel         *model;
  GtkFilterListModel *filtered_model;
  GtkStack           *stack_events;

  GDateTime          *today;
  GDateTime          *for_day;
  gint                day_offset;
};
G_DEFINE_TYPE (PhoshEventList, phosh_event_list, GTK_TYPE_BOX)


static void
on_items_changed (PhoshEventList *self)
{
  const char *page = "no-events";

  if (g_list_model_get_n_items (G_LIST_MODEL (self->filtered_model)))
    page = "events";

  gtk_stack_set_visible_child_name (self->stack_events, page);
}


static GtkWidget *
create_upcoming_event_row (gpointer item, gpointer user_data)
{
  PhoshCalendarEvent *event = PHOSH_CALENDAR_EVENT (item);
  PhoshEventList *self = PHOSH_EVENT_LIST (user_data);
  GtkWidget *widget;

  widget =  phosh_upcoming_event_new (phosh_calendar_event_get_summary (event),
                                      phosh_calendar_event_get_begin (event),
                                      phosh_calendar_event_get_end (event),
                                      self->for_day,
                                      phosh_calendar_event_get_color (event),
                                      /* TODO: org.gnome.desktop.interface clock-format */
                                      TRUE);
  g_object_bind_property (event, "summary", widget, "summary", G_BINDING_DEFAULT);
  g_object_bind_property (event, "begin", widget, "begin", G_BINDING_DEFAULT);
  g_object_bind_property (event, "end", widget, "end", G_BINDING_DEFAULT);
  g_object_bind_property (event, "color", widget, "color", G_BINDING_DEFAULT);

  return widget;
}


static gboolean
filter_day (gpointer item, gpointer data)
{
  int begin_day_diff, end_day_diff;
  PhoshCalendarEvent *event = PHOSH_CALENDAR_EVENT (item);
  PhoshEventList *self = PHOSH_EVENT_LIST (data);
  GDateTime *begin = phosh_calendar_event_get_begin (event);
  GDateTime *end = phosh_calendar_event_get_end (event);
  g_autoptr (GDate) today = NULL;
  g_autoptr (GDate) begin_date = NULL;
  g_autoptr (GDate) end_date = NULL;

  today = g_date_new_dmy (g_date_time_get_day_of_month (self->today),
                          g_date_time_get_month (self->today),
                          g_date_time_get_year (self->today));
  begin_date = g_date_new_dmy (g_date_time_get_day_of_month (begin),
                               g_date_time_get_month (begin),
                               g_date_time_get_year (begin));
  end_date = g_date_new_dmy (g_date_time_get_day_of_month (end),
                             g_date_time_get_month (end),
                             g_date_time_get_year (end));

  begin_day_diff = g_date_days_between (today, begin_date);
  end_day_diff = g_date_days_between (today, end_date);

  /* Include events that start at the event-lists day */
  if (begin_day_diff == self->day_offset)
    return TRUE;

  /* Include events that started before the event-lists day but are
     ongoing at the lists day … */
  if (begin_day_diff < self->day_offset && end_day_diff >= self->day_offset) {
    /* …but prevent multi day events to leak to the next day as they end at midnight */
    if (end_day_diff == self->day_offset &&
        g_date_time_get_hour (end) == 0 &&
        g_date_time_get_minute (end) == 0) {
      return FALSE;
    } else {
      return TRUE;
    }
  }

  return FALSE;
}


static char *
get_label (PhoshEventList *self)
{
  switch (self->day_offset) {
  case 0:
    return g_strdup (_("Today"));
  case 1:
    return g_strdup (_("Tomorrow"));
  case 2 ... 7: {
    /* Translators: An event/appointment is happening on that day of the week (e.g. Tuesday) */
    return g_date_time_format (self->for_day, "%A");
  }
  default:
    return g_strdup_printf (ngettext ("In %d day", "In %d days", self->day_offset),
                            self->day_offset);
  }
}


static void
phosh_event_list_set_day_offset (PhoshEventList *self, int offset)
{
  g_autofree char *str = NULL;

  self->day_offset = offset;

  g_clear_pointer (&self->for_day, g_date_time_unref);
  self->for_day = g_date_time_add_days (self->today, self->day_offset);

  str = get_label (self);
  gtk_label_set_label (self->label, str);

  if (self->filtered_model)
    gtk_filter_list_model_refilter (self->filtered_model);
}


static void
phosh_event_list_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  PhoshEventList *self = PHOSH_EVENT_LIST (object);

  switch (property_id) {
  case PROP_DAY_OFFSET:
    phosh_event_list_set_day_offset (self, g_value_get_int (value));
    break;
  case PROP_TODAY:
    phosh_event_list_set_today (self, g_value_get_boxed (value));
    break;
  case PROP_MODEL:
    phosh_event_list_bind_model (self, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_event_list_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PhoshEventList *self = PHOSH_EVENT_LIST (object);

  switch (property_id) {
  case PROP_LABEL:
    g_value_set_string (value, gtk_label_get_label (self->label));
    break;
  case PROP_DAY_OFFSET:
    g_value_set_int (value, self->day_offset);
    break;
  case PROP_TODAY:
    g_value_set_boxed (value, self->model);
    break;
  case PROP_MODEL:
    g_value_set_object (value, self->model);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_event_list_dispose (GObject *object)
{
  PhoshEventList *self = PHOSH_EVENT_LIST (object);

  phosh_event_list_bind_model (self, NULL);

  G_OBJECT_CLASS (phosh_event_list_parent_class)->dispose (object);
}


static void
phosh_event_list_finalize (GObject *object)
{
  PhoshEventList *self = PHOSH_EVENT_LIST (object);

  g_clear_pointer (&self->today, g_date_time_unref);
  g_clear_pointer (&self->for_day, g_date_time_unref);

  G_OBJECT_CLASS (phosh_event_list_parent_class)->finalize (object);
}


static void
phosh_event_list_class_init (PhoshEventListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_event_list_get_property;
  object_class->set_property = phosh_event_list_set_property;
  object_class->dispose = phosh_event_list_dispose;
  object_class->finalize = phosh_event_list_finalize;

  /**
   * PhoshEventList:label:
   *
   * The label (e.g. "tomorrow") of this event list.
   */
  props[PROP_LABEL] =
    g_param_spec_string ("label", "", "",
                         NULL,
                         G_PARAM_READABLE);
  /**
   * PhoshEventList:day-offset:
   *
   * the offset in days from the reference date. Events on that day
   * Will be displayed shown in the list.
   */
  props[PROP_DAY_OFFSET] =
    g_param_spec_int ("day-offset", "", "",
                      0,
                      7,
                      0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  /**
   * PhoshEventList:today:
   *
   * The reference data used as current base.
   */
  props[PROP_TODAY] =
    g_param_spec_boxed ("today", "", "",
                        G_TYPE_DATE_TIME,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * PhoshEventList:model:
   *
   *
   * The model to extract the calendar events from.
   */
  props[PROP_MODEL] =
    g_param_spec_object ("model", "", "",
                         G_TYPE_LIST_MODEL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/plugins/upcoming-events/event-list.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshEventList, label);
  gtk_widget_class_bind_template_child (widget_class, PhoshEventList, lb_events);
  gtk_widget_class_bind_template_child (widget_class, PhoshEventList, stack_events);
}


static void
phosh_event_list_init (PhoshEventList *self)
{
  self->today = g_date_time_new_now_local ();
  /* Not initialized */
  self->day_offset = G_MAXINT;

  gtk_widget_init_template (GTK_WIDGET (self));
}


void
phosh_event_list_bind_model (PhoshEventList *self, GListModel *model)
{
  g_return_if_fail (PHOSH_IS_EVENT_LIST (self));
  g_return_if_fail (G_IS_LIST_MODEL (model) || model == NULL);
  g_return_if_fail (self->today != NULL);
  g_return_if_fail (self->day_offset != G_MAXINT);

  if (self->model == model)
    return;

  g_set_object (&self->model, model);
  if (self->filtered_model)
    g_signal_handlers_disconnect_by_data (self->filtered_model, self);
  g_clear_object (&self->filtered_model);

  if (self->model) {
    self->filtered_model = gtk_filter_list_model_new (self->model,
                                                      filter_day,
                                                      self,
                                                      NULL);
    gtk_list_box_bind_model (self->lb_events,
                             G_LIST_MODEL (self->filtered_model),
                             create_upcoming_event_row,
                             self, NULL);

    g_signal_connect_swapped (self->filtered_model,
                              "items-changed",
                              G_CALLBACK (on_items_changed),
                              self);

  } else {
    gtk_list_box_bind_model (self->lb_events,
                             NULL, NULL, NULL, NULL);
  }


  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_MODEL]);
}


void
phosh_event_list_set_today (PhoshEventList *self, GDateTime *today)
{
  g_return_if_fail (PHOSH_IS_EVENT_LIST (self));

  g_clear_pointer (&self->today, g_date_time_unref);

  if (today == NULL)
    return;

  self->today = g_date_time_ref (today);
  /* Refresh label and events */
  phosh_event_list_set_day_offset (self, self->day_offset);
}
