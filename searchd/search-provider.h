/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_SEARCH_PROVIDER phosh_search_provider_get_type()
G_DECLARE_DERIVABLE_TYPE (PhoshSearchProvider, phosh_search_provider, PHOSH, SEARCH_PROVIDER, GObject)

struct _PhoshSearchProviderClass
{
  GObjectClass parent_class;
};

PhoshSearchProvider *phosh_search_provider_new                    (const char          *desktop_app_id,
                                                                   GCancellable        *parent_cancellable,
                                                                   const char          *bus_path,
                                                                   const char          *bus_name,
                                                                   gboolean             autostart,
                                                                   gboolean             default_disabled);
GPtrArray           *phosh_search_provider_limit_results          (GStrv                results,
                                                                   int                  max);
void                 phosh_search_provider_activate_result        (PhoshSearchProvider *self,
                                                                   const char          *result,
                                                                   const char *const   *terms,
                                                                   guint                timestamp);
void                 phosh_search_provider_launch_search          (PhoshSearchProvider *self,
                                                                   const char *const   *terms,
                                                                   guint                timestamp);
void                 phosh_search_provider_get_result_meta        (PhoshSearchProvider *self,
                                                                   GStrv                results,
                                                                   GAsyncReadyCallback  callback,
                                                                   gpointer             callback_data);
GPtrArray           *phosh_search_provider_get_result_meta_finish (PhoshSearchProvider  *self,
                                                                   GAsyncResult         *res,
                                                                   GError              **error);
void                 phosh_search_provider_get_initial            (PhoshSearchProvider *self,
                                                                   const char *const   *terms,
                                                                   GAsyncReadyCallback  callback,
                                                                   gpointer             callback_data);
GStrv                phosh_search_provider_get_initial_finish     (PhoshSearchProvider  *self,
                                                                   GAsyncResult         *res,
                                                                   GError              **error);
void                 phosh_search_provider_get_subsearch          (PhoshSearchProvider *self,
                                                                   const char *const   *results,
                                                                   const char *const   *terms,
                                                                   GAsyncReadyCallback  callback,
                                                                   gpointer             callback_data);
GStrv                phosh_search_provider_get_subsearch_finish   (PhoshSearchProvider  *self,
                                                                   GAsyncResult         *res,
                                                                   GError              **error);
gboolean             phosh_search_provider_get_ready              (PhoshSearchProvider  *self);
const char          *phosh_search_provider_get_bus_path           (PhoshSearchProvider *self);

G_END_DECLS
