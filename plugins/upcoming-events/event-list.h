/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_EVENT_LIST (phosh_event_list_get_type ())

G_DECLARE_FINAL_TYPE (PhoshEventList, phosh_event_list, PHOSH, EVENT_LIST, GtkBox)

void phosh_event_list_bind_model (PhoshEventList *self, GListModel *model);
void phosh_event_list_set_today (PhoshEventList *self, GDateTime *today);

G_END_DECLS
