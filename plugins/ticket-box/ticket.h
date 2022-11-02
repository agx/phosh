/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_TICKET (phosh_ticket_get_type ())

G_DECLARE_FINAL_TYPE (PhoshTicket, phosh_ticket, PHOSH, TICKET, GObject)

PhoshTicket       *phosh_ticket_new (GFile *file, GFileInfo *info);
GFile             *phosh_ticket_get_file (PhoshTicket *self);
const char        *phosh_ticket_get_display_name (PhoshTicket *self);
GIcon             *phosh_ticket_get_icon (PhoshTicket *self);
GDateTime         *phosh_ticket_get_mod_time (PhoshTicket *self);

G_END_DECLS
