/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include <gio/gio.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_SEARCH_CLIENT phosh_search_client_get_type()
G_DECLARE_DERIVABLE_TYPE (PhoshSearchClient, phosh_search_client, PHOSH, SEARCH_CLIENT, GObject)

struct _PhoshSearchClientClass
{
  GObjectClass parent_class;
};

char              *phosh_search_client_markup_string           (PhoshSearchClient    *self,
                                                                const char           *string);
void               phosh_search_client_new                     (GCancellable         *cancellable,
                                                                GAsyncReadyCallback   callback,
                                                                gpointer              user_data);
PhoshSearchClient *phosh_search_client_new_finish              (GObject              *source,
                                                                GAsyncResult         *result,
                                                                GError              **error);
void               phosh_search_client_query                   (PhoshSearchClient    *self,
                                                                const char           *query,
                                                                GAsyncReadyCallback   callback,
                                                                gpointer              callback_data);
gboolean           phosh_search_client_query_finish            (PhoshSearchClient  *self,
                                                                GAsyncResult       *res,
                                                                GError            **error);
void               phosh_search_client_get_last_results        (PhoshSearchClient *self,
                                                                GAsyncReadyCallback callback,
                                                                gpointer callback_data);
GVariant          *phosh_search_client_get_last_results_finish (PhoshSearchClient *self,
                                                                GAsyncResult *res,
                                                                GError **error);
void               phosh_search_client_activate_result         (PhoshSearchClient   *self,
                                                                const char          *source_id,
                                                                const char          *result_id,
                                                                GAsyncReadyCallback  callback,
                                                                gpointer             callback_data);
gboolean           phosh_search_client_activate_result_finish  (PhoshSearchClient  *self,
                                                                GAsyncResult       *res,
                                                                GError            **error);
void               phosh_search_client_launch_source           (PhoshSearchClient   *self,
                                                                const char          *source_id,
                                                                GAsyncReadyCallback  callback,
                                                                gpointer             callback_data);
gboolean           phosh_search_client_launch_source_finish    (PhoshSearchClient  *self,
                                                                GAsyncResult       *res,
                                                                GError            **error);

G_END_DECLS
