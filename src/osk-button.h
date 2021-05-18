/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_OSK_BUTTON (phosh_osk_button_get_type())

G_DECLARE_FINAL_TYPE (PhoshOskButton, phosh_osk_button, PHOSH, OSK_BUTTON, GtkToggleButton)

GtkWidget * phosh_osk_button_new (void);

G_END_DECLS
