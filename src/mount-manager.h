/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_MOUNT_MANAGER     phosh_mount_manager_get_type ()

G_DECLARE_FINAL_TYPE (PhoshMountManager, phosh_mount_manager,
                      PHOSH, MOUNT_MANAGER, GObject)

PhoshMountManager *phosh_mount_manager_new (void);

G_END_DECLS
