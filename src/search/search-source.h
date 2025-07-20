/*
 * Copyright © 2019-2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_SEARCH_SOURCE (phosh_search_source_get_type ())

typedef struct _PhoshSearchSource PhoshSearchSource;

PhoshSearchSource *phosh_search_source_new                (const char        *id,
                                                           GAppInfo          *app_info);
GType              phosh_search_source_get_type           (void);
PhoshSearchSource *phosh_search_source_ref                (PhoshSearchSource *self);
void               phosh_search_source_unref              (PhoshSearchSource *self);
const char        *phosh_search_source_get_id             (PhoshSearchSource *self);
GAppInfo          *phosh_search_source_get_app_info       (PhoshSearchSource *self);
guint              phosh_search_source_get_position       (PhoshSearchSource *self);
void               phosh_search_source_set_position       (PhoshSearchSource *self,
                                                           guint              position);
GVariant          *phosh_search_source_serialise          (PhoshSearchSource *self);
PhoshSearchSource *phosh_search_source_deserialise        (GVariant          *variant);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PhoshSearchSource, phosh_search_source_unref)

G_END_DECLS
