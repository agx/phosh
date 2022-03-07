/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <glib-object.h>
#include "phosh-wwan-iface.h"
#include "wwan-manager.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_WWAN_OFONO (phosh_wwan_ofono_get_type())

G_DECLARE_FINAL_TYPE (PhoshWWanOfono, phosh_wwan_ofono, PHOSH, WWAN_OFONO, PhoshWWanManager)

PhoshWWanOfono *phosh_wwan_ofono_new (void);

G_END_DECLS
