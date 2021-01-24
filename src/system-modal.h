/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "layersurface.h"
#include "monitor/monitor.h"

#define PHOSH_TYPE_SYSTEM_MODAL (phosh_system_modal_get_type ())

G_DECLARE_DERIVABLE_TYPE (PhoshSystemModal, phosh_system_modal, PHOSH, SYSTEM_MODAL, PhoshLayerSurface)

/**
 * PhoshSystemModalClass
 * @parent_class: The parent class
 */
struct _PhoshSystemModalClass {
  PhoshLayerSurfaceClass parent_class;
};


GtkWidget *phosh_system_modal_new (PhoshMonitor *monitor);
