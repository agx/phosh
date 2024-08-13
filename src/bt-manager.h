/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <manager.h>

#include <glib-object.h>
#include <gio/gio.h>

/* Avoid including gnome-bluetooth headers here - they cause downstream issues for GIR consumers */
typedef struct _BluetoothDevice BluetoothDevice;

G_BEGIN_DECLS

#define PHOSH_TYPE_BT_MANAGER (phosh_bt_manager_get_type ())

G_DECLARE_FINAL_TYPE (PhoshBtManager, phosh_bt_manager, PHOSH, BT_MANAGER, PhoshManager)

PhoshBtManager    *phosh_bt_manager_new (void);
const char  *phosh_bt_manager_get_icon_name (PhoshBtManager *self);
gboolean     phosh_bt_manager_get_enabled (PhoshBtManager *self);
void         phosh_bt_manager_set_enabled (PhoshBtManager *self, gboolean enabled);
gboolean     phosh_bt_manager_get_present (PhoshBtManager *self);
GListModel  *phosh_bt_manager_get_connectable_devices (PhoshBtManager *self);
guint        phosh_bt_manager_get_n_connected (PhoshBtManager *self);
const char  *phosh_bt_manager_get_info (PhoshBtManager *self);
void         phosh_bt_manager_connect_device_async  (PhoshBtManager      *self,
                                                     BluetoothDevice     *device,
                                                     gboolean             connect,
                                                     GAsyncReadyCallback  callback,
                                                     GCancellable        *cancellable,
                                                     gpointer             user_data);
gboolean     phosh_bt_manager_connect_device_finish (PhoshBtManager      *self,
                                                     GAsyncResult        *result,
                                                     GError             **error);

G_END_DECLS
