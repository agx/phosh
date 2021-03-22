/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "system-modal-dialog.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_END_SESSION_DIALOG (phosh_end_session_dialog_get_type ())

G_DECLARE_FINAL_TYPE (PhoshEndSessionDialog, phosh_end_session_dialog, PHOSH,
                      END_SESSION_DIALOG, PhoshSystemModalDialog)

/**
 * PhoshLogoutAction:
 * @PHOSH_END_SESSION_ACTION_LOGOUT: Loguout
 * @PHOSH_END_SESSION_ACTION_SHUTDOWN: Shutdown
 * @PHOSH_END_SESSION_ACTION_REBOOT: Reboot
 *
 * The requested action the #PhoshEndSessionDialog should display. This matches
 * the values of the DBus protocols 'open' request..
 */
typedef enum {
  PHOSH_END_SESSION_ACTION_LOGOUT,
  PHOSH_END_SESSION_ACTION_SHUTDOWN,
  PHOSH_END_SESSION_ACTION_REBOOT,
  /* Not used by gnome-session */
  /*<private >*/
  PHOSH_END_SESSION_ACTION_HIBERNATE,
  PHOSH_END_SESSION_ACTION_SUSPEND,
  PHOSH_END_SESSION_ACTION_HYBRID_SLEEP,
} PhoshLogoutAction;

GtkWidget *phosh_end_session_dialog_new                  (gint                action,
                                                          gint                seconds,
                                                          const char *const * paths);
gboolean   phosh_end_session_dialog_get_action_confirmed (PhoshEndSessionDialog *self);
gint       phosh_end_session_dialog_get_action           (PhoshEndSessionDialog *self);

G_END_DECLS
