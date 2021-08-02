/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "calls-manager.h"
#include "layersurface.h"

G_BEGIN_DECLS

/**
 * PhoshLockscreenPage:
 * @PHOSH_LOCKSCREEN_PAGE_DEFAULT: The default locked page
 * @PHOSH_LOCKSCREEN_PAGE_UNLOCK: The unlock page (where PIN is entered)
 *
 * This enum indicates which page is shown on the lockscreen.
 * This helps #PhoshGnomeShellManager to decide when to emit
 * AcceleratorActivated events over DBus
 */
typedef enum {
  PHOSH_LOCKSCREEN_PAGE_DEFAULT,
  PHOSH_LOCKSCREEN_PAGE_UNLOCK,
} PhoshLockscreenPage;

#define PHOSH_TYPE_LOCKSCREEN (phosh_lockscreen_get_type ())

G_DECLARE_FINAL_TYPE (PhoshLockscreen, phosh_lockscreen, PHOSH, LOCKSCREEN,
                      PhoshLayerSurface)

GtkWidget * phosh_lockscreen_new (gpointer layer_shell, gpointer wl_output, PhoshCallsManager *calls_manager);
PhoshLockscreenPage phosh_lockscreen_get_page (PhoshLockscreen *self);

G_END_DECLS
