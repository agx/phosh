/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Florian Loers <florianloers@mailbox.org>
 */

#pragma once

#include "dbus/portal-dbus.h"

#define PHOSH_TYPE_PORTAL_REQUEST (phosh_portal_request_get_type ())
G_DECLARE_FINAL_TYPE (PhoshPortalRequest, phosh_portal_request,
                      PHOSH, PORTAL_REQUEST,
                      PhoshDBusImplPortalRequestSkeleton);

PhoshPortalRequest *phosh_portal_request_new (const char *sender, const char *app_id, const char *id);
gboolean            phosh_portal_request_exported (PhoshPortalRequest *self);
void                phosh_portal_request_export (PhoshPortalRequest *self, GDBusConnection *connection);
void                phosh_portal_request_unexport (PhoshPortalRequest *self);
