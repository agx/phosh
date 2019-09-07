/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_APP_GRID_BUTTON phosh_app_grid_button_get_type()

/**
 * PhoshAppGridButtonMode:
 * @PHOSH_APP_GRID_BUTTON_LAUNCHER: Standard mode used in drawer/search etc
 * @PHOSH_APP_GRID_BUTTON_FAVOURITES: Fovourites area (doesn't show label)
 * 
 * Display mode of a #PhoshAppGridButton
 */
typedef enum /*< enum,prefix=PHOSH >*/
{
  PHOSH_APP_GRID_BUTTON_LAUNCHER,    /*< nick=launcher >*/
  PHOSH_APP_GRID_BUTTON_FAVOURITES,  /*< nick=favourites >*/
} PhoshAppGridButtonMode;

G_DECLARE_DERIVABLE_TYPE (PhoshAppGridButton, phosh_app_grid_button, PHOSH, APP_GRID_BUTTON, GtkFlowBoxChild)

struct _PhoshAppGridButtonClass
{
  GtkFlowBoxChildClass parent_class;
};

GtkWidget             *phosh_app_grid_button_new           (GAppInfo               *info);
GtkWidget             *phosh_app_grid_button_new_favorite  (GAppInfo               *info);
void                   phosh_app_grid_button_set_app_info  (PhoshAppGridButton     *self,
                                                            GAppInfo               *info);
GAppInfo              *phosh_app_grid_button_get_app_info  (PhoshAppGridButton     *self);
gboolean               phosh_app_grid_button_is_favourite  (PhoshAppGridButton     *self);
void                   phosh_app_grid_button_set_mode      (PhoshAppGridButton     *self,
                                                            PhoshAppGridButtonMode  mode);
PhoshAppGridButtonMode phosh_app_grid_button_get_mode      (PhoshAppGridButton     *self);

G_END_DECLS
