/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "status-icon.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_HKS_INFO (phosh_hks_info_get_type ())

G_DECLARE_FINAL_TYPE (PhoshHksInfo, phosh_hks_info, PHOSH, HKS_INFO, PhoshStatusIcon)

GtkWidget * phosh_hks_info_new (void);

G_END_DECLS
