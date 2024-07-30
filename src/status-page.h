/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_STATUS_PAGE phosh_status_page_get_type ()
G_DECLARE_DERIVABLE_TYPE (PhoshStatusPage, phosh_status_page, PHOSH, STATUS_PAGE, GtkRevealer)

struct _PhoshStatusPageClass {
  GtkBinClass parent_class;
};

PhoshStatusPage *phosh_status_page_new (void);

void             phosh_status_page_set_title (PhoshStatusPage *self, const char *title);
const char      *phosh_status_page_get_title (PhoshStatusPage *self);
void             phosh_status_page_set_header (PhoshStatusPage *self, GtkWidget *header);
GtkWidget       *phosh_status_page_get_header (PhoshStatusPage *self);
void             phosh_status_page_set_footer (PhoshStatusPage *self, GtkWidget *footer);
GtkWidget       *phosh_status_page_get_footer (PhoshStatusPage *self);

G_END_DECLS
