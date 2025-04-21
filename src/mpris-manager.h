/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "mpris-dbus.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_MPRIS_MANAGER (phosh_mpris_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshMprisManager, phosh_mpris_manager, PHOSH, MPRIS_MANAGER, GObject)

PhoshMprisManager *      phosh_mpris_manager_new                 (void);
PhoshMprisDBusMediaPlayer2Player *
                         phosh_mpris_manager_get_player          (PhoshMprisManager  *self);
gboolean                 phosh_mpris_manager_get_can_raise       (PhoshMprisManager  *self);
void                     phosh_mpris_manager_raise_async         (PhoshMprisManager  *self,
                                                                  GCancellable       *cancel,
                                                                  GAsyncReadyCallback callback,
                                                                  gpointer            user_data);
gboolean                 phosh_mpris_manager_raise_finish        (PhoshMprisManager  *self,
                                                                  GAsyncResult       *res,
                                                                  GError            **err);

G_END_DECLS
