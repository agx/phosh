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
                                               "/mobi/phosh/plugins/calendar/calendar.ui");

  gtk_widget_class_set_css_name (widget_class, "phosh-calendar");
}

static void
phosh_calendar_init (PhoshCalendar *self)
{
  g_autoptr (GtkCssProvider) css_provider = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  css_provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (css_provider,
                                       "/mobi/phosh/plugins/calendar/stylesheet/common.css");
  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (self)),
                                  GTK_STYLE_PROVIDER (css_provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}
