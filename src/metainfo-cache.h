/*
 * Copyright (C) 2025 The Phosh Authors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_METAINFO_CACHE phosh_metainfo_cache_get_type ()

G_DECLARE_FINAL_TYPE (PhoshMetainfoCache, phosh_metainfo_cache,
                      PHOSH, METAINFO_CACHE, GObject)

PhoshMetainfoCache *phosh_metainfo_cache_get_default (void);
char               *phosh_metainfo_get_data_id (PhoshMetainfoCache *self, const char *cid);

G_END_DECLS
