/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "dbus/phosh-gtk-mountoperation-dbus.h"
#include <glib-object.h>

#define PHOSH_TYPE_GTK_MOUNT_MANAGER             (phosh_gtk_mount_manager_get_type ())
G_DECLARE_FINAL_TYPE (PhoshGtkMountManager, phosh_gtk_mount_manager, PHOSH, GTK_MOUNT_MANAGER,
                      PhoshDBusMountOperationHandlerSkeleton)

PhoshGtkMountManager *phosh_gtk_mount_manager_new (void);
