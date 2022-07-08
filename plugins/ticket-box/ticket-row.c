/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-config.h"

#include "ticket-row.h"

#include <glib/gi18n.h>
#include <handy.h>

enum {
  PROP_0,
  PROP_TICKET,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];


struct _PhoshTicketRow {
  HdyActionRow parent;

  PhoshTicket *ticket;
};
G_DEFINE_TYPE (PhoshTicketRow, phosh_ticket_row, HDY_TYPE_ACTION_ROW)


static void
phosh_ticket_row_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  PhoshTicketRow *self = PHOSH_TICKET_ROW (object);

  switch (property_id) {
  case PROP_TICKET:
    self->ticket = g_value_dup_object (value);
    hdy_preferences_row_set_title (HDY_PREFERENCES_ROW (self),
                                   phosh_ticket_get_display_name (self->ticket));
/* TODO: by document type */
    hdy_action_row_set_icon_name (HDY_ACTION_ROW (self), "x-office-document-symbolic");
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_ticket_row_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PhoshTicketRow *self = PHOSH_TICKET_ROW (object);

  switch (property_id) {
  case PROP_TICKET:
    g_value_set_object (value, self->ticket);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_ticket_row_finalize (GObject *object)
{
  PhoshTicketRow *self = PHOSH_TICKET_ROW (object);

  g_clear_object (&self->ticket);

  G_OBJECT_CLASS (phosh_ticket_row_parent_class)->finalize (object);
}


static void
phosh_ticket_row_class_init (PhoshTicketRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_ticket_row_get_property;
  object_class->set_property = phosh_ticket_row_set_property;
  object_class->finalize = phosh_ticket_row_finalize;

  props[PROP_TICKET] =
    g_param_spec_object ("ticket", "", "",
                         PHOSH_TYPE_TICKET,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_css_name (widget_class, "phosh-ticket-row");
}


static void
phosh_ticket_row_init (PhoshTicketRow *self)
{
}


GtkWidget *
phosh_ticket_row_new (PhoshTicket *ticket)
{
  return GTK_WIDGET (g_object_new (PHOSH_TYPE_TICKET_ROW, "ticket", ticket, NULL));
}
