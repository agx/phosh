/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "layersurface.h"

#define PHOSH_TYPE_SYSTEM_PROMPT (phosh_system_prompt_get_type())

G_DECLARE_FINAL_TYPE (PhoshSystemPrompt, phosh_system_prompt, PHOSH, SYSTEM_PROMPT, PhoshLayerSurface)

GtkWidget *phosh_system_prompt_new (gpointer layer_shell,
                                    gpointer wl_output);
