/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "calendar.h"

/**
 * PhoshCalendar
 *
 * Display the calenadar.
 */
struct _PhoshCalendar {
  GtkBox        parent;
};

G_DEFINE_TYPE (PhoshCalendar, phosh_calendar, GTK_TYPE_BOX);

static void
phosh_calendar_class_init (PhoshCalendarClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  g_type_ensure (GTK_TYPE_CALENDAR);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/plugins/calendar/calendar.ui");
}

static void
phosh_calendar_init (PhoshCalendar *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
