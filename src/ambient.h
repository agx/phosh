/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "calls-manager.h"
#include "sensor-proxy-manager.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_AMBIENT (phosh_ambient_get_type ())

G_DECLARE_FINAL_TYPE (PhoshAmbient, phosh_ambient, PHOSH, AMBIENT, GObject);

PhoshAmbient *phosh_ambient_new (PhoshSensorProxyManager *sensor_proxy_manager);

G_END_DECLS
