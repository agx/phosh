/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _PhoshTestCompositorState {
  GPid                        pid;
  struct wl_display          *display;
  struct wl_registry         *registry;
  struct wl_output           *output;
  struct zwlr_layer_shell_v1 *layer_shell;
  GdkDisplay                 *gdk_display;
} PhoshTestCompositorState;

PhoshTestCompositorState *phosh_test_compositor_new (void);
void                      phosh_test_compositor_free (PhoshTestCompositorState *state);

G_END_DECLS
