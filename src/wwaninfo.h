/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_WWAN_INFO (phosh_wwan_info_get_type())

G_DECLARE_FINAL_TYPE (PhoshWWanInfo, phosh_wwan_info, PHOSH, WWAN_INFO, GtkImage)

GtkWidget * phosh_wwan_info_new (void);
void phosh_wwan_info_set_size (PhoshWWanInfo *self, gint size);
gint phosh_wwan_info_get_size (PhoshWWanInfo *self);

G_END_DECLS
