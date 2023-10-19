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

#define PHOSH_TYPE_SEARCH_RESULT_META (phosh_search_result_meta_get_type ())

typedef struct _PhoshSearchResultMeta PhoshSearchResultMeta;

PhoshSearchResultMeta *phosh_search_result_meta_new                (const char            *id,
                                                                    const char            *title,
                                                                    const char            *description,
                                                                    GIcon                 *icon,
                                                                    const char            *clipboard);
GType                  phosh_search_result_meta_get_type           (void);
PhoshSearchResultMeta *phosh_search_result_meta_ref                (PhoshSearchResultMeta *self);
void                   phosh_search_result_meta_unref              (PhoshSearchResultMeta *self);
const char            *phosh_search_result_meta_get_id             (PhoshSearchResultMeta *self);
const char            *phosh_search_result_meta_get_title          (PhoshSearchResultMeta *self);
const char            *phosh_search_result_meta_get_description    (PhoshSearchResultMeta *self);
GIcon                 *phosh_search_result_meta_get_icon           (PhoshSearchResultMeta *self);
const char            *phosh_search_result_meta_get_clipboard_text (PhoshSearchResultMeta *self);
GVariant              *phosh_search_result_meta_serialise          (PhoshSearchResultMeta *self);
PhoshSearchResultMeta *phosh_search_result_meta_deserialise        (GVariant              *variant);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PhoshSearchResultMeta,
                               phosh_search_result_meta_unref)

G_END_DECLS
