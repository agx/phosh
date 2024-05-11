/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "dbus/calls-dbus.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_CALL (phosh_call_get_type())

G_DECLARE_FINAL_TYPE (PhoshCall, phosh_call, PHOSH, CALL, GObject)

PhoshCall * phosh_call_new                (PhoshCallsDBusCallsCall *proxy);

G_END_DECLS
