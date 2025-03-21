/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "layersurface-priv.h"
#include "background-image.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_LOCKSCREEN_BG (phosh_lockscreen_bg_get_type ())

G_DECLARE_FINAL_TYPE (PhoshLockscreenBg, phosh_lockscreen_bg, PHOSH, LOCKSCREEN_BG,
                      PhoshLayerSurface)

PhoshLockscreenBg *     phosh_lockscreen_bg_new (struct zwlr_layer_shell_v1 *layer_shell,
                                                 struct wl_output           *wl_output);
void                    phosh_lockscreen_bg_set_image (PhoshLockscreenBg    *self,
                                                       PhoshBackgroundImage *image);

G_END_DECLS
