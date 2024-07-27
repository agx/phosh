/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "bluetooth-device.h"

#include <handy.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_BT_DEVICE_ROW phosh_bt_device_row_get_type ()
G_DECLARE_FINAL_TYPE (PhoshBtDeviceRow, phosh_bt_device_row, PHOSH, BT_DEVICE_ROW, HdyActionRow)

GtkWidget *phosh_bt_device_row_new      (BluetoothDevice  *device);

G_END_DECLS
