/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "layersurface.h"

#define PHOSH_TYPE_POLKIT_AUTH_PROMPT (phosh_polkit_auth_prompt_get_type())

G_DECLARE_FINAL_TYPE (PhoshPolkitAuthPrompt, phosh_polkit_auth_prompt, PHOSH, POLKIT_AUTH_PROMPT, PhoshLayerSurface);

GtkWidget *phosh_polkit_auth_prompt_new (const gchar *action_id,
                                         const gchar *message,
                                         const gchar *icon_name,
                                         const gchar *cookie,
                                         GStrv user_names,
                                         gpointer layer_shell,
                                         gpointer wl_output);


