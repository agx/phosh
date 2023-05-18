/*
 * Copyright (C) 2021-2023 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "system-modal.h"

#include <gtk/gtk.h>

#define PHOSH_TYPE_SYSTEM_MODAL_DIALOG (phosh_system_modal_dialog_get_type ())

G_DECLARE_DERIVABLE_TYPE (PhoshSystemModalDialog, phosh_system_modal_dialog, PHOSH, SYSTEM_MODAL_DIALOG, PhoshSystemModal)

/**
 * PhoshSystemModalDialogClass
 * @parent_class: The parent class
 */
struct _PhoshSystemModalDialogClass {
  PhoshSystemModalClass parent_class;
};


GtkWidget *phosh_system_modal_dialog_new (void);
void       phosh_system_modal_dialog_set_content (PhoshSystemModalDialog *self, GtkWidget *content);
void       phosh_system_modal_dialog_add_button (PhoshSystemModalDialog *self, GtkWidget *button, gint position);
void       phosh_system_modal_dialog_set_title (PhoshSystemModalDialog *self, const gchar *title);
void       phosh_system_modal_dialog_remove_button (PhoshSystemModalDialog *self, GtkWidget *button);
GList     *phosh_system_modal_dialog_get_buttons (PhoshSystemModalDialog *self);
void       phosh_system_modal_dialog_close (PhoshSystemModalDialog *self);
