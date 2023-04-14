/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "dbus/phosh-screenshot-dbus.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_SCREENSHOT_MANAGER     phosh_screenshot_manager_get_type ()

G_DECLARE_FINAL_TYPE (PhoshScreenshotManager, phosh_screenshot_manager,
                      PHOSH, SCREENSHOT_MANAGER, PhoshDBusScreenshotSkeleton)

PhoshScreenshotManager *phosh_screenshot_manager_new (void);
gboolean                phosh_screenshot_manager_do_screenshot (PhoshScreenshotManager *self,
                                                                GdkRectangle           *area,
                                                                const char             *filename,
                                                                gboolean            include_cursor);

G_END_DECLS
