/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-wayland.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _PhoshTestCompositorState {
  GPid                        pid;
  PhoshWayland               *wl;
  struct wl_output           *output;
  GdkDisplay                 *gdk_display;
} PhoshTestCompositorState;

PhoshTestCompositorState *phosh_test_compositor_new (void);
void                      phosh_test_compositor_free (PhoshTestCompositorState *state);

G_END_DECLS
