/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "app-tracker.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_SPLASH_MANAGER (phosh_splash_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshSplashManager, phosh_splash_manager, PHOSH, SPLASH_MANAGER, GObject)

PhoshSplashManager *phosh_splash_manager_new (PhoshAppTracker *app_tracker);

G_END_DECLS
