/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_MOUNT_OPERATION (phosh_mount_operation_get_type ())

G_DECLARE_FINAL_TYPE (PhoshMountOperation, phosh_mount_operation, PHOSH, MOUNT_OPERATION, GMountOperation)

PhoshMountOperation *phosh_mount_operation_new (void);

G_END_DECLS
