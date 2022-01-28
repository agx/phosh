/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib.h>
#include <pango/pango.h>

G_BEGIN_DECLS

PangoDirection phosh_find_base_dir (const gchar *text,
                                    gint         length);

G_END_DECLS
