/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "status-icon.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_CONNECTIVITY_INFO (phosh_connectivity_info_get_type())

G_DECLARE_FINAL_TYPE (PhoshConnectivityInfo, phosh_connectivity_info, PHOSH, CONNECTIVITY_INFO, PhoshStatusIcon)

GtkWidget * phosh_connectivity_info_new (void);

G_END_DECLS
