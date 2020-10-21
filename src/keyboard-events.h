/*
 * Copyright (C) 2020 Evangelos Ribeiro Tzaras
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
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
                      GSimpleActionGroup)

PhoshKeyboardEvents  *phosh_keyboard_events_new           (void);

