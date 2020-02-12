/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#pragma once

#include "dbus/geoclue-agent-dbus.h"
#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_LOCATION_MANAGER     (phosh_location_manager_get_type ())
G_DECLARE_FINAL_TYPE (PhoshLocationManager, phosh_location_manager, PHOSH, LOCATION_MANAGER,
                      PhoshGeoClueDBusOrgFreedesktopGeoClue2AgentSkeleton)

PhoshLocationManager * phosh_location_manager_new (void);

G_END_DECLS
