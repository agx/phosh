/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#pragma once

#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>

#define phosh_async_error_warn(err, ...) \
  phosh_error_warnv (G_LOG_DOMAIN, err, G_IO_ERROR, G_IO_ERROR_CANCELLED, __VA_ARGS__)

#define phosh_dbus_service_error_warn(err, ...) \
  phosh_error_warnv (G_LOG_DOMAIN, err, G_IO_ERROR, G_IO_ERROR_NOT_FOUND, __VA_ARGS__)

void             phosh_cp_widget_destroy (void *widget);
GDesktopAppInfo *phosh_get_desktop_app_info_for_app_id (const char *app_id);
gchar           *phosh_munge_app_id (const gchar *app_id);
char            *phosh_strip_suffix_from_app_id (const char *app_id);
gboolean         phosh_find_systemd_session (char **session_id);
gboolean         phosh_error_warnv (const char  *log_domain,
                                    GError      *err,
                                    GQuark       domain,
                                    int          code,
                                    const gchar *fmt,
                                    ...) G_GNUC_PRINTF(5, 6);
int              phosh_create_shm_file (off_t size);
char            *phosh_util_escape_markup (const char *markup, gboolean allow_markup);
gboolean         phosh_util_gesture_is_touch (GtkGestureSingle *gesture);
gboolean         phosh_util_have_gnome_software (gboolean scan);
void             phosh_util_toggle_style_class (GtkWidget *widget, const char *style_class, gboolean toggle);
const char      *phosh_util_get_stylesheet (const char *theme_name);
gboolean         phosh_clear_fd (int *fd, GError **err);
const char      *phosh_util_get_icon_by_wifi_strength (guint strength, gboolean is_connecting);
gboolean         phosh_util_file_equal (GFile *file1, GFile *file2);
GdkPixbuf       *phosh_util_data_uri_to_pixbuf (const char *uri, GError **error);
gboolean         phosh_util_matches_app_info (GAppInfo *info, const char *search);
GStrv            phosh_util_append_to_strv (GStrv array, const char *element);
GStrv            phosh_util_remove_from_strv (GStrv array, const char *element);
