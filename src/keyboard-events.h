/*
 * Copyright (C) 2020 Evangelos Ribeiro Tzaras
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Evangelos Ribeiro Tzaras <devrtz@fortysixandtwo.eu>
 */

#pragma once

#include <stdint.h>
#include <gtk/gtk.h>

#define PHOSH_TYPE_KEYBOARD_EVENTS (phosh_keyboard_events_get_type ())

G_DECLARE_FINAL_TYPE (PhoshKeyboardEvents,
                      phosh_keyboard_events,
                      PHOSH,
                      KEYBOARD_EVENTS,
                      GObject)

PhoshKeyboardEvents  *phosh_keyboard_events_new           (void);
void                  phosh_keyboard_events_register_keys (PhoshKeyboardEvents *self,
                                                           gchar              **keys,
                                                           size_t               len);
