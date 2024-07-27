/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2005-2008  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#pragma once

#include <glib.h>

/**
 * SECTION:bluetooth-enums
 * @short_description: Bluetooth related enumerations
 * @stability: Stable
 * @include: bluetooth-enums.h
 *
 * Enumerations related to Bluetooth.
 **/

/**
 * BluetoothType:
 * @BLUETOOTH_TYPE_ANY: any device, or a device of an unknown type
 * @BLUETOOTH_TYPE_PHONE: a telephone (usually a cell/mobile phone)
 * @BLUETOOTH_TYPE_MODEM: a modem
 * @BLUETOOTH_TYPE_COMPUTER: a computer, can be a laptop, a wearable computer, etc.
 * @BLUETOOTH_TYPE_NETWORK: a network device, such as a router
 * @BLUETOOTH_TYPE_HEADSET: a headset (usually a hands-free device)
 * @BLUETOOTH_TYPE_HEADPHONES: headphones (covers two ears)
 * @BLUETOOTH_TYPE_OTHER_AUDIO: another type of audio device
 * @BLUETOOTH_TYPE_KEYBOARD: a keyboard
 * @BLUETOOTH_TYPE_MOUSE: a mouse
 * @BLUETOOTH_TYPE_CAMERA: a camera (still or moving)
 * @BLUETOOTH_TYPE_PRINTER: a printer
 * @BLUETOOTH_TYPE_JOYPAD: a joypad, joystick, or other game controller
 * @BLUETOOTH_TYPE_TABLET: a drawing tablet
 * @BLUETOOTH_TYPE_VIDEO: a video device, such as a webcam
 * @BLUETOOTH_TYPE_REMOTE_CONTROL: a remote control
 * @BLUETOOTH_TYPE_SCANNER: a scanner
 * @BLUETOOTH_TYPE_DISPLAY: a display
 * @BLUETOOTH_TYPE_WEARABLE: a wearable computer
 * @BLUETOOTH_TYPE_TOY: a toy or game
 * @BLUETOOTH_TYPE_SPEAKERS: audio speaker or speakers
 *
 * The type of a Bluetooth device. See also %BLUETOOTH_TYPE_INPUT and %BLUETOOTH_TYPE_AUDIO
 **/
typedef enum {
	BLUETOOTH_TYPE_ANY		= 1 << 0,
	BLUETOOTH_TYPE_PHONE		= 1 << 1,
	BLUETOOTH_TYPE_MODEM		= 1 << 2,
	BLUETOOTH_TYPE_COMPUTER		= 1 << 3,
	BLUETOOTH_TYPE_NETWORK		= 1 << 4,
	BLUETOOTH_TYPE_HEADSET		= 1 << 5,
	BLUETOOTH_TYPE_HEADPHONES	= 1 << 6,
	BLUETOOTH_TYPE_OTHER_AUDIO	= 1 << 7,
	BLUETOOTH_TYPE_KEYBOARD		= 1 << 8,
	BLUETOOTH_TYPE_MOUSE		= 1 << 9,
	BLUETOOTH_TYPE_CAMERA		= 1 << 10,
	BLUETOOTH_TYPE_PRINTER		= 1 << 11,
	BLUETOOTH_TYPE_JOYPAD		= 1 << 12,
	BLUETOOTH_TYPE_TABLET		= 1 << 13,
	BLUETOOTH_TYPE_VIDEO		= 1 << 14,
	BLUETOOTH_TYPE_REMOTE_CONTROL	= 1 << 15,
	BLUETOOTH_TYPE_SCANNER		= 1 << 16,
	BLUETOOTH_TYPE_DISPLAY		= 1 << 17,
	BLUETOOTH_TYPE_WEARABLE		= 1 << 18,
	BLUETOOTH_TYPE_TOY		= 1 << 19,
	BLUETOOTH_TYPE_SPEAKERS		= 1 << 20,
} BluetoothType;

#define _BLUETOOTH_TYPE_NUM_TYPES 21

/**
 * BLUETOOTH_TYPE_INPUT:
 *
 * Use this value to select any Bluetooth input device where a #BluetoothType enum is required.
 */
#define BLUETOOTH_TYPE_INPUT (BLUETOOTH_TYPE_KEYBOARD | BLUETOOTH_TYPE_MOUSE | BLUETOOTH_TYPE_TABLET | BLUETOOTH_TYPE_JOYPAD)
/**
 * BLUETOOTH_TYPE_AUDIO:
 *
 * Use this value to select any Bluetooth audio device where a #BluetoothType enum is required.
 */
#define BLUETOOTH_TYPE_AUDIO (BLUETOOTH_TYPE_HEADSET | BLUETOOTH_TYPE_HEADPHONES | BLUETOOTH_TYPE_OTHER_AUDIO | BLUETOOTH_TYPE_SPEAKERS)

/**
 * BluetoothBatteryType:
 * @BLUETOOTH_BATTERY_TYPE_NONE: no battery reporting
 * @BLUETOOTH_BATTERY_TYPE_PERCENTAGE: battery reported in percentage
 * @BLUETOOTH_BATTERY_TYPE_COARSE: battery reported coarsely
 *
 * The type of battery reporting supported by the device.
 **/
typedef enum {
	BLUETOOTH_BATTERY_TYPE_NONE,
	BLUETOOTH_BATTERY_TYPE_PERCENTAGE,
	BLUETOOTH_BATTERY_TYPE_COARSE
} BluetoothBatteryType;

/**
 * BluetoothAdapterState:
 * @BLUETOOTH_ADAPTER_STATE_ABSENT: Bluetooth adapter is missing.
 * @BLUETOOTH_ADAPTER_STATE_ON: Bluetooth adapter is on.
 * @BLUETOOTH_ADAPTER_STATE_TURNING_ON: Bluetooth adapter is being turned on.
 * @BLUETOOTH_ADAPTER_STATE_TURNING_OFF: Bluetooth adapter is being turned off.
 * @BLUETOOTH_ADAPTER_STATE_OFF: Bluetooth adapter is off.
 *
 * A more precise power state for a Bluetooth adapter.
 **/
typedef enum {
	BLUETOOTH_ADAPTER_STATE_ABSENT = 0,
	BLUETOOTH_ADAPTER_STATE_ON,
	BLUETOOTH_ADAPTER_STATE_TURNING_ON,
	BLUETOOTH_ADAPTER_STATE_TURNING_OFF,
	BLUETOOTH_ADAPTER_STATE_OFF,
} BluetoothAdapterState;
