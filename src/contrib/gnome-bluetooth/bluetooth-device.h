/*
 * Copyright (C) 2021 Bastien Nocera <hadess@hadess.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <glib-object.h>
#include <bluetooth-enums.h>

#define BLUETOOTH_TYPE_DEVICE (bluetooth_device_get_type())
G_DECLARE_FINAL_TYPE (BluetoothDevice, bluetooth_device, BLUETOOTH, DEVICE, GObject)

const char *bluetooth_device_get_object_path (BluetoothDevice *device);
void bluetooth_device_dump (BluetoothDevice *device);
char *bluetooth_device_to_string (BluetoothDevice *device);
