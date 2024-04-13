/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "app-grid-base-button.h"
#include "folder-info.h"

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_APP_GRID_FOLDER_BUTTON phosh_app_grid_folder_button_get_type ()
G_DECLARE_FINAL_TYPE (PhoshAppGridFolderButton, phosh_app_grid_folder_button, PHOSH, APP_GRID_FOLDER_BUTTON, PhoshAppGridBaseButton)

GtkWidget *phosh_app_grid_folder_button_new_from_folder_info (PhoshFolderInfo *folder_info);

G_END_DECLS
