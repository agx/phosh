/*
 * Copyright © 2020 Lugsole
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib.h>


G_BEGIN_DECLS

char *phosh_time_diff_in_words (GDateTime *dt, GDateTime *dt_now);

G_END_DECLS
