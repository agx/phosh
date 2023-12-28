/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "dbus/phosh-end-session-dialog-dbus.h"

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_SESSION_MANAGER     phosh_session_manager_get_type ()

G_DECLARE_FINAL_TYPE (PhoshSessionManager, phosh_session_manager,
                      PHOSH, SESSION_MANAGER, PhoshDBusEndSessionDialogSkeleton)

PhoshSessionManager *phosh_session_manager_new (void);
gboolean phosh_session_manager_is_active (PhoshSessionManager *self);
void     phosh_session_manager_register (PhoshSessionManager *self, const gchar *app_id, const gchar *startup_id);
void     phosh_session_manager_logout (PhoshSessionManager *self);
void     phosh_session_manager_shutdown (PhoshSessionManager *self);
void     phosh_session_manager_reboot (PhoshSessionManager *self);
guint    phosh_session_manager_inhibit_suspend (PhoshSessionManager *self, const char *reason);
void     phosh_session_manager_uninhibit_suspend (PhoshSessionManager *self, guint cookie);

void     phosh_session_manager_export_end_session (PhoshSessionManager *self,
                                                   GDBusConnection *connection);

G_END_DECLS
