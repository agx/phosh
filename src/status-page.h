/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_STATUS_PAGE phosh_status_page_get_type ()
G_DECLARE_DERIVABLE_TYPE (PhoshStatusPage, phosh_status_page, PHOSH, STATUS_PAGE, GtkBin)

struct _PhoshStatusPageClass {
  GtkBinClass parent_class;

  /* Padding for future expansion */
  void        (*_phosh_reserved0) (void);
  void        (*_phosh_reserved1) (void);
  void        (*_phosh_reserved2) (void);
  void        (*_phosh_reserved3) (void);
  void        (*_phosh_reserved4) (void);
  void        (*_phosh_reserved5) (void);
  void        (*_phosh_reserved6) (void);
  void        (*_phosh_reserved7) (void);
  void        (*_phosh_reserved8) (void);
  void        (*_phosh_reserved9) (void);
};

PhoshStatusPage *phosh_status_page_new (void);

void             phosh_status_page_set_title (PhoshStatusPage *self, const char *title);
const char      *phosh_status_page_get_title (PhoshStatusPage *self);
void             phosh_status_page_set_header (PhoshStatusPage *self, GtkWidget *header);
GtkWidget       *phosh_status_page_get_header (PhoshStatusPage *self);
void             phosh_status_page_set_content (PhoshStatusPage *self, GtkWidget *content);
GtkWidget       *phosh_status_page_get_content (PhoshStatusPage *self);
void             phosh_status_page_set_footer (PhoshStatusPage *self, GtkWidget *footer);
GtkWidget       *phosh_status_page_get_footer (PhoshStatusPage *self);

G_END_DECLS
