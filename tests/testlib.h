/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-wayland.h"
#include "monitor/monitor.h"

#include <gtk/gtk.h>

#include <linux/input-event-codes.h>

#pragma once

G_BEGIN_DECLS

typedef struct _PhoshTestCompositorState {
  GPid                        pid;
  PhoshWayland               *wl;
  GdkDisplay                 *gdk_display;
  PhoshMonitor               *monitor;
} PhoshTestCompositorState;

PhoshTestCompositorState *phosh_test_compositor_new (gboolean heads_stub);
void                      phosh_test_compositor_free (PhoshTestCompositorState *state);
PhoshMonitor*             phosh_test_get_monitor (PhoshTestCompositorState *state);
struct zwp_virtual_keyboard_v1 * phosh_test_keyboard_new (PhoshWayland *wl);
void                      phosh_test_keyboard_press_keys (struct zwp_virtual_keyboard_v1 *keyboard,
                                                          GTimer *timer, ...) G_GNUC_NULL_TERMINATED;
void                      phosh_test_keyboard_press_modifiers   (struct zwp_virtual_keyboard_v1 *keyboard,
                                                                 guint                           modifiers);
void                      phosh_test_keyboard_release_modifiers (struct zwp_virtual_keyboard_v1 *keyboard);
void                      phosh_test_keyboard_press_timeout (struct zwp_virtual_keyboard_v1 *keyboard,
                                                             GTimer                         *timer,
                                                             guint                           key,
                                                             guint                           sleep);
void                      phosh_test_remove_tree (GFile *file);

G_END_DECLS
