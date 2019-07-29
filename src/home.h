/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */
#pragma once

#include <gtk/gtk.h>
#include "layersurface.h"

#define PHOSH_TYPE_HOME (phosh_home_get_type())

#define PHOSH_HOME_BUTTON_HEIGHT 40

typedef enum {
  PHOSH_HOME_STATE_FOLDED,    /* Only home button is visiable */
  PHOSH_HOME_STATE_UNFOLDED,  /* Home screen takes the whole screen except the top panel */
} PhoshHomeState;

G_DECLARE_FINAL_TYPE (PhoshHome, phosh_home, PHOSH, HOME, PhoshLayerSurface)

GtkWidget * phosh_home_new (struct zwlr_layer_shell_v1 *layer_shell,
                            struct wl_output *wl_output);
void phosh_home_set_state (PhoshHome *self, PhoshHomeState state);
