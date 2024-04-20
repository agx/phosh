/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

#define PHOSH_FOLDERS_SCHEMA_ID "org.gnome.desktop.app-folders"

#define PHOSH_TYPE_FOLDER_INFO phosh_folder_info_get_type ()
G_DECLARE_FINAL_TYPE (PhoshFolderInfo, phosh_folder_info, PHOSH, FOLDER_INFO, GObject)

PhoshFolderInfo *phosh_folder_info_new_from_folder_path (char *path);

char       *phosh_folder_info_get_name (PhoshFolderInfo *self);
void        phosh_folder_info_set_name (PhoshFolderInfo *self, const char *name);
GListModel *phosh_folder_info_get_app_infos (PhoshFolderInfo *self);
gboolean    phosh_folder_info_contains (PhoshFolderInfo *self, GAppInfo *app_info);
gboolean    phosh_folder_info_refilter (PhoshFolderInfo *self, const char *search);
void        phosh_folder_info_add_app_info (PhoshFolderInfo *self, GAppInfo *app_info);
gboolean    phosh_folder_info_remove_app_info (PhoshFolderInfo *self, GAppInfo *app_info);

G_END_DECLS
