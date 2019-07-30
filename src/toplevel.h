/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#pragma once

#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"
#include <gtk/gtk.h>

#define PHOSH_TYPE_TOPLEVEL (phosh_toplevel_get_type())

G_DECLARE_FINAL_TYPE (PhoshToplevel,
                      phosh_toplevel,
                      PHOSH,
                      TOPLEVEL,
                      GObject)

PhoshToplevel *phosh_toplevel_new_from_handle (struct zwlr_foreign_toplevel_handle_v1 *handle);
const gchar *phosh_toplevel_get_title (PhoshToplevel *self);
const gchar *phosh_toplevel_get_app_id (PhoshToplevel *self);
gboolean phosh_toplevel_is_configured (PhoshToplevel *self);
void phosh_toplevel_raise (PhoshToplevel *self, struct wl_seat *seat);
void phosh_toplevel_close (PhoshToplevel *self);
