/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_FAVOURITE_LIST_MODEL phosh_favourite_list_model_get_type()
G_DECLARE_DERIVABLE_TYPE (PhoshFavouriteListModel, phosh_favourite_list_model, PHOSH, FAVOURITE_LIST_MODEL, GObject)

struct _PhoshFavouriteListModelClass
{
  GObjectClass parent_class;
};

PhoshFavouriteListModel *phosh_favourite_list_model_get_default      (void);
gboolean                 phosh_favourite_list_model_app_is_favourite (PhoshFavouriteListModel *self,
                                                                      GAppInfo                *app);
void                     phosh_favourite_list_model_add_app          (PhoshFavouriteListModel *self,
                                                                      GAppInfo                *app);
void                     phosh_favourite_list_model_remove_app       (PhoshFavouriteListModel *self,
                                                                      GAppInfo                *app);

G_END_DECLS
