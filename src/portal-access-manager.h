/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Florian Loers <florianloers@mailbox.org>
 */

#pragma once

#include "dbus/portal-dbus.h"

#define PHOSH_TYPE_PORTAL_ACCESS_MANAGER (phosh_portal_access_manager_get_type ())
G_DECLARE_FINAL_TYPE (PhoshPortalAccessManager, phosh_portal_access_manager,
                      PHOSH, PORTAL_ACCESS_MANAGER,
                      PhoshDBusImplPortalAccessSkeleton);

PhoshPortalAccessManager *phosh_portal_access_manager_new (void);
