/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_KEYPAD (phosh_keypad_get_type ())

G_DECLARE_DERIVABLE_TYPE (PhoshKeypad, phosh_keypad, PHOSH, KEYPAD, GtkBin)

/**
 * PhoshKeypadClass:
 * @parent_class: The parent class
 */
struct _PhoshKeypadClass {
  GtkBinClass parent_class;

  /*< private >*/
  gpointer    padding[4];
};

GtkWidget *phosh_keypad_new                     (void);
void       phosh_keypad_set_entry               (PhoshKeypad *self,
                                                 GtkEntry    *entry);
GtkEntry  *phosh_keypad_get_entry               (PhoshKeypad *self);
void       phosh_keypad_set_start_action        (PhoshKeypad *self,
                                                 GtkWidget   *start_action);
GtkWidget *phosh_keypad_get_start_action        (PhoshKeypad *self);
void       phosh_keypad_set_end_action          (PhoshKeypad *self,
                                                 GtkWidget   *end_action);
GtkWidget *phosh_keypad_get_end_action          (PhoshKeypad *self);

G_END_DECLS
