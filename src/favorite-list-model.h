/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_FAVORITE_LIST_MODEL phosh_favorite_list_model_get_type()
G_DECLARE_DERIVABLE_TYPE (PhoshFavoriteListModel, phosh_favorite_list_model, PHOSH, FAVORITE_LIST_MODEL, GObject)

struct _PhoshFavoriteListModelClass
{
  GObjectClass parent_class;
};

PhoshFavoriteListModel *phosh_favorite_list_model_get_default     (void);
gboolean                phosh_favorite_list_model_app_is_favorite (PhoshFavoriteListModel *self,
                                                                   GAppInfo                *app);
void                    phosh_favorite_list_model_add_app         (PhoshFavoriteListModel *self,
                                                                   GAppInfo                *app);
void                    phosh_favorite_list_model_remove_app      (PhoshFavoriteListModel *self,
                                                                   GAppInfo                *app);

G_END_DECLS
