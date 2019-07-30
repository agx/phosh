/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3+
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#pragma once

#include "gamma-control-client-protocol.h"
#include "idle-client-protocol.h"
#include "phosh-private-client-protocol.h"
#include "wlr-input-inhibitor-unstable-v1-client-protocol.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"
#include "wlr-output-management-unstable-v1-client-protocol.h"
#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"

#include <glib-object.h>

#define PHOSH_TYPE_WAYLAND phosh_wayland_get_type()

G_DECLARE_FINAL_TYPE (PhoshWayland, phosh_wayland, PHOSH, WAYLAND, GObject)

PhoshWayland                         *phosh_wayland_get_default (void);
GHashTable                           *phosh_wayland_get_wl_outputs (PhoshWayland *self);
gboolean                              phosh_wayland_has_wl_output  (PhoshWayland *self,
                                                                    struct wl_output *wl_output);
struct gamma_control_manager         *phosh_wayland_get_gamma_control_manager (PhoshWayland *self);
struct org_kde_kwin_idle             *phosh_wayland_get_org_kde_kwin_idle (PhoshWayland *self);
struct phosh_private                 *phosh_wayland_get_phosh_private (PhoshWayland *self);
struct wl_seat                       *phosh_wayland_get_wl_seat (PhoshWayland *self);
struct xdg_wm_base                   *phosh_wayland_get_xdg_wm_base (PhoshWayland *self);
struct zwlr_input_inhibit_manager_v1 *phosh_wayland_get_zwlr_input_inhibit_manager_v1 (PhoshWayland *self);
struct zwlr_layer_shell_v1           *phosh_wayland_get_zwlr_layer_shell_v1 (PhoshWayland *self);
struct zxdg_output_manager_v1        *phosh_wayland_get_zxdg_output_manager_v1 (PhoshWayland *self);
struct zwlr_output_manager_v1        *phosh_wayland_get_zwlr_output_manager_v1 (PhoshWayland *self);
struct zwlr_foreign_toplevel_manager_v1 *phosh_wayland_get_zwlr_foreign_toplevel_manager_v1 (PhoshWayland *self);
void                                  phosh_wayland_roundtrip (PhoshWayland *self);
