/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

void brightness_init (GtkScale *scale, gulong handler_id);
void brightness_dispose (void);
void brightness_set (int brightness);
