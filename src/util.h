/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#pragma once

#include <gtk/gtk.h>

#define STR_IS_NULL_OR_EMPTY(x) ((x) == NULL || (x)[0] == '\0')

void     phosh_cp_widget_destroy (void *widget);
char    *phosh_fix_app_id (const char *app_id);
gchar   *phosh_munge_app_id (const gchar *app_id);
gboolean phosh_find_systemd_session (char **session_id);
