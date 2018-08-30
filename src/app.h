/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */
#pragma once

#include <gtk/gtk.h>

#define PHOSH_TYPE_APP (phosh_app_get_type())

G_DECLARE_FINAL_TYPE (PhoshApp, phosh_app, PHOSH, APP, GtkButton)

GtkWidget * phosh_app_new (const char *app_id, const char *title);
const char *phosh_app_get_app_id (PhoshApp *self);
const char *phosh_app_get_title (PhoshApp *self);

