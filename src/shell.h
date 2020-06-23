/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "bt-manager.h"
#include "docked-manager.h"
#include "feedback-manager.h"
#include "hks-manager.h"
#include "location-manager.h"
#include "lockscreen-manager.h"
#include "monitor-manager.h"
#include "monitor/monitor.h"
#include "osk-manager.h"
#include "rotation-manager.h"
#include "session-manager.h"
#include "toplevel-manager.h"
#include "torch-manager.h"
#include "wifimanager.h"
#include "wwan/phosh-wwan-iface.h"

#include <gtk/gtk.h>

/**
 * PhoshShellStateFlags:
 * @PHOSH_STATE_NONE: No other state
 * @PHOSH_STATE_MODAL_SYSTEM_PROMPT: any modal prompt shown
 * @PHOSH_STATE_BLANKED: built-in display off
 * @PHOSH_STATE_LOCKED: displays locked
 * @PHOSH_STATE_SETTINGS: settings menu unfolded from top bar
 * @PHOSH_STATE_OVERVIEW: overview unfolded from bottom bar
 *
 * These flags are used to keep track of the state
 * the #PhoshShell is in.
 */
typedef enum {
  PHOSH_STATE_NONE                = 0,
  PHOSH_STATE_MODAL_SYSTEM_PROMPT = 1 << 0,
  PHOSH_STATE_BLANKED             = 1 << 1,
  PHOSH_STATE_LOCKED              = 1 << 2,
  PHOSH_STATE_SETTINGS            = 1 << 3,
  PHOSH_STATE_OVERVIEW            = 1 << 4,
} PhoshShellStateFlags;

G_BEGIN_DECLS

#define PHOSH_APP_ID "sm.puri.Phosh"

#define PHOSH_TYPE_SHELL phosh_shell_get_type()


G_DECLARE_FINAL_TYPE (PhoshShell, phosh_shell, PHOSH, SHELL, GObject)

PhoshShell          *phosh_shell_get_default     (void);
void                 phosh_shell_set_transform   (PhoshShell *self, PhoshMonitorTransform transform);
PhoshMonitorTransform phosh_shell_get_transform   (PhoshShell *self);
void                 phosh_shell_get_usable_area (PhoshShell *self,
                                                  int        *x,
                                                  int        *y,
                                                  int        *width,
                                                  int        *height);
void                 phosh_shell_set_locked      (PhoshShell *self, gboolean locked);
gboolean             phosh_shell_get_locked      (PhoshShell *self);
void                 phosh_shell_lock            (PhoshShell *self);
void                 phosh_shell_unlock          (PhoshShell *self);
void                 phosh_shell_set_primary_monitor (PhoshShell *self, PhoshMonitor *monitor);
PhoshMonitor        *phosh_shell_get_primary_monitor (PhoshShell *self);
PhoshMonitor        *phosh_shell_get_builtin_monitor (PhoshShell *self);
PhoshLockscreenManager *phosh_shell_get_lockscreen_manager (PhoshShell *self);
PhoshModeManager    *phosh_shell_get_mode_manager    (PhoshShell *self);
PhoshMonitorManager *phosh_shell_get_monitor_manager (PhoshShell *self);
PhoshOskManager     *phosh_shell_get_osk_manager     (PhoshShell *self);
PhoshToplevelManager *phosh_shell_get_toplevel_manager (PhoshShell *self);
PhoshWifiManager    *phosh_shell_get_wifi_manager    (PhoshShell *self);
PhoshFeedbackManager *phosh_shell_get_feedback_manager (PhoshShell *self);
PhoshBtManager      *phosh_shell_get_bt_manager      (PhoshShell *self);
PhoshWWan           *phosh_shell_get_wwan        (PhoshShell *self);
PhoshRotationManager *phosh_shell_get_rotation_manager (PhoshShell *self);
PhoshTorchManager   *phosh_shell_get_torch_manager (PhoshShell *self);
PhoshDockedManager  *phosh_shell_get_docked_manager (PhoshShell *self);
PhoshHksManager *    phosh_shell_get_hks_manager     (PhoshShell *self);
PhoshLocationManager *phosh_shell_get_location_manager (PhoshShell *self);
PhoshSessionManager *phosh_shell_get_session_manager (PhoshShell *self);
void                 phosh_shell_fade_out (PhoshShell *self, guint timeout);
void                 phosh_shell_enable_power_save (PhoshShell *self, gboolean enable);
gboolean             phosh_shell_started_by_display_manager(PhoshShell *self);
gboolean             phosh_shell_is_startup_finished (PhoshShell *self);
void                 phosh_shell_add_global_keyboard_action_entries (PhoshShell *self,
                                                                     const GActionEntry *actions,
                                                                     gint n_entries,
                                                                     gpointer user_data);
void                 phosh_shell_remove_global_keyboard_action_entries (PhoshShell *self,
                                                                        const GActionEntry *actions,
                                                                        gint n_entries);
gboolean             phosh_shell_is_session_active (PhoshShell *self);
GdkAppLaunchContext *phosh_shell_get_app_launch_context (PhoshShell *self);
PhoshShellStateFlags phosh_shell_get_state (PhoshShell *self);
void                 phosh_shell_set_state (PhoshShell *self, PhoshShellStateFlags state, gboolean enabled);

G_END_DECLS
