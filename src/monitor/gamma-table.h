/*
 * Copyright (C) 2023 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

void phosh_gamma_table_fill (guint16 *table, guint32 ramp_size, guint32 temp);

G_END_DECLS
