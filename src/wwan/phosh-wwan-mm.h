/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <glib-object.h>
#include "phosh-wwan-iface.h"
#include "wwan-manager.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_WWAN_MM (phosh_wwan_mm_get_type())

G_DECLARE_FINAL_TYPE (PhoshWWanMM, phosh_wwan_mm, PHOSH, WWAN_MM, PhoshWWanManager)

PhoshWWanMM * phosh_wwan_mm_new                (void);

G_END_DECLS
