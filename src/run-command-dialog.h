/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Florian Loers <florianloers@mailbox.org>
 */

#pragma once

#include "system-modal-dialog.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_RUN_COMMAND_DIALOG (phosh_run_command_dialog_get_type ())

G_DECLARE_FINAL_TYPE (PhoshRunCommandDialog, phosh_run_command_dialog, PHOSH, RUN_COMMAND_DIALOG, PhoshSystemModalDialog)

GtkWidget *phosh_run_command_dialog_new         (void);
void       phosh_run_command_dialog_set_message (PhoshRunCommandDialog *self,
                                                 const gchar           *message);

G_END_DECLS
