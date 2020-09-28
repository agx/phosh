/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "bt-manager.h"
#include "feedback-manager.h"
#include "lockscreen-manager.h"
#include "monitor-manager.h"
#include "monitor/monitor.h"
#include "osk-manager.h"
#include "toplevel-manager.h"
#include "torch-manager.h"
#include "wifimanager.h"
#include "wwan/phosh-wwan-iface.h"

#include <gtk/gtk.h>

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
PhoshMonitorManager *phosh_shell_get_monitor_manager (PhoshShell *self);
PhoshOskManager     *phosh_shell_get_osk_manager     (PhoshShell *self);
PhoshToplevelManager *phosh_shell_get_toplevel_manager (PhoshShell *self);
PhoshWifiManager    *phosh_shell_get_wifi_manager    (PhoshShell *self);
PhoshFeedbackManager *phosh_shell_get_feedback_manager (PhoshShell *self);
PhoshBtManager      *phosh_shell_get_bt_manager      (PhoshShell *self);
PhoshWWan           *phosh_shell_get_wwan        (PhoshShell *self);
PhoshTorchManager   *phosh_shell_get_torch_manager (PhoshShell *self);
void                 phosh_shell_fade_out (PhoshShell *self, guint timeout);
void                 phosh_shell_enable_power_save (PhoshShell *self, gboolean enable);
gboolean             phosh_shell_started_by_display_manager(PhoshShell *self);
gboolean             phosh_shell_is_startup_finished (PhoshShell *self);

G_END_DECLS
