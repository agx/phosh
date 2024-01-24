/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "background.h"
#include "manager.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_BACKGROUND_MANAGER (phosh_background_manager_get_type())

G_DECLARE_FINAL_TYPE (PhoshBackgroundManager,
                      phosh_background_manager,
                      PHOSH,
                      BACKGROUND_MANAGER,
                      PhoshManager)

PhoshBackgroundManager *phosh_background_manager_new      (void);
GList                  *phosh_background_manager_get_backgrounds (PhoshBackgroundManager *self);
PhoshBackgroundData    *phosh_background_manager_get_data (PhoshBackgroundManager *self,
                                                           PhoshBackground        *background);

G_END_DECLS
