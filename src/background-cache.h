/*
 * Copyright (C) 2024 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "background-image.h"

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_BACKGROUND_CACHE (phosh_background_cache_get_type ())

G_DECLARE_FINAL_TYPE (PhoshBackgroundCache, phosh_background_cache, PHOSH, BACKGROUND_CACHE, GObject)

PhoshBackgroundCache         *phosh_background_cache_get_default       (void);
void                          phosh_background_cache_fetch_background  (PhoshBackgroundCache *self,
                                                                        GFile                *file,
                                                                        GCancellable         *cancel);
PhoshBackgroundImage         *phosh_background_cache_lookup_background (PhoshBackgroundCache *self,
                                                                        GFile                *file);
void                          phosh_background_cache_clear_all         (PhoshBackgroundCache *self);

G_END_DECLS
