/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "system-modal-dialog.h"

#define PHOSH_TYPE_POLKIT_AUTH_PROMPT (phosh_polkit_auth_prompt_get_type())

G_DECLARE_FINAL_TYPE (PhoshPolkitAuthPrompt, phosh_polkit_auth_prompt, PHOSH, POLKIT_AUTH_PROMPT, PhoshSystemModalDialog);

GtkWidget *phosh_polkit_auth_prompt_new (const char *action_id,
                                         const char *message,
                                         const char *icon_name,
                                         const char *cookie,
                                         GStrv user_names);


