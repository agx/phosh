/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "system-modal-dialog.h"

#define PHOSH_TYPE_CELL_BROADCAST_PROMPT (phosh_cell_broadcast_prompt_get_type ())

G_DECLARE_FINAL_TYPE (PhoshCellBroadcastPrompt, phosh_cell_broadcast_prompt, PHOSH,
                      CELL_BROADCAST_PROMPT, PhoshSystemModalDialog)

GtkWidget        *phosh_cell_broadcast_prompt_new (const char *message, const char *title);
