/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#pragma once

#include <glib-object.h>
#include <gio/gio.h>

#define PHOSH_TYPE_AUTH (phosh_auth_get_type())

G_DECLARE_FINAL_TYPE (PhoshAuth, phosh_auth, PHOSH, AUTH, GObject)

GObject *phosh_auth_new (void);

void     phosh_auth_authenticate_async_start  (PhoshAuth           *self,
                                               const char          *number,
                                               GCancellable        *cancellable,
                                               GAsyncReadyCallback  callback,
                                               gpointer             user_data);
gboolean phosh_auth_authenticate_async_finish (PhoshAuth     *self,
                                               GAsyncResult  *result,
                                               GError       **error);

