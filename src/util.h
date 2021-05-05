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

#define phosh_async_error_warn(err, ...) \
  phosh_error_warnv (G_LOG_DOMAIN, err, G_IO_ERROR, G_IO_ERROR_CANCELLED, __VA_ARGS__)

void     phosh_cp_widget_destroy (void *widget);
char    *phosh_fix_app_id (const char *app_id);
gchar   *phosh_munge_app_id (const gchar *app_id);
gboolean phosh_find_systemd_session (char **session_id);
gboolean phosh_error_warnv (const char  *log_domain,
                            GError      *err,
                            GQuark       domain,
                            int          code,
                            const gchar *fmt,
                            ...) G_GNUC_PRINTF(5, 6);
