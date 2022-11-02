/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ticket.h"

#include <gtk/gtk.h>
#include <handy.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_TICKET_ROW (phosh_ticket_row_get_type ())

G_DECLARE_FINAL_TYPE (PhoshTicketRow, phosh_ticket_row, PHOSH, TICKET_ROW, HdyActionRow)

GtkWidget *phosh_ticket_row_new (PhoshTicket *ticket);

G_END_DECLS
