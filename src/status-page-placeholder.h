/*
 * Copyright (C) 2024 Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_STATUS_PAGE_PLACEHOLDER (phosh_status_page_placeholder_get_type ())

G_DECLARE_FINAL_TYPE (PhoshStatusPagePlaceholder, phosh_status_page_placeholder, PHOSH, STATUS_PAGE_PLACEHOLDER, GtkBin)

PhoshStatusPagePlaceholder *phosh_status_page_placeholder_new           (void);
void                        phosh_status_page_placeholder_set_title     (PhoshStatusPagePlaceholder *self,
                                                                         const char                 *title);
const char                 *phosh_status_page_placeholder_get_title     (PhoshStatusPagePlaceholder *self);
void                        phosh_status_page_placeholder_set_icon_name (PhoshStatusPagePlaceholder *self,
                                                                         const char                 *icon_name);
const char                 *phosh_status_page_placeholder_get_icon_name (PhoshStatusPagePlaceholder *self);

G_END_DECLS
