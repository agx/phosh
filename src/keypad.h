/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_KEYPAD (phosh_keypad_get_type ())

G_DECLARE_FINAL_TYPE (PhoshKeypad, phosh_keypad, PHOSH, KEYPAD, GtkGrid)

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
void       phosh_keypad_set_shuffle             (PhoshKeypad *self,
                                                 gboolean     shuffle);
gboolean   phosh_keypad_get_shuffle             (PhoshKeypad *self);
void       phosh_keypad_distribute              (PhoshKeypad *self);
G_END_DECLS
