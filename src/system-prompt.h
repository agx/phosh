/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "system-modal-dialog.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_SYSTEM_PROMPT (phosh_system_prompt_get_type())

G_DECLARE_FINAL_TYPE (PhoshSystemPrompt, phosh_system_prompt, PHOSH, SYSTEM_PROMPT, PhoshSystemModalDialog)

GtkWidget *phosh_system_prompt_new (void);

G_END_DECLS
