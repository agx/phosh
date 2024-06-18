/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *             2020-2021 Purism SPC
 *             2023 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

/**
 * PhoshAppFilterModeFlags:
 * @PHOSH_APP_FILTER_MODE_FLAGS_NONE: No filtering
 * @PHOSH_APP_FILTER_MODE_FLAGS_ADAPTIVE: Only show apps in mobile mode that adapt
 *    to smalls screen sizes.
 *
 * Controls what kind of app filtering is done.
*/
typedef enum {
  PHOSH_APP_FILTER_MODE_FLAGS_NONE      = 0,
  PHOSH_APP_FILTER_MODE_FLAGS_ADAPTIVE  = (1 << 0),
} PhoshAppFilterModeFlags;

/**
 * PhoshShellLayout:
 * @PHOSH_SHELL_LAYOUT_NONE: Don't perform any additional layouting
 * @PHOSH_SHELL_LAYOUT_DEVICE: Use device information to optimize layout
 *
 * How the shell's UI elements are layed out.
 */
typedef enum {
  PHOSH_SHELL_LAYOUT_NONE     = 0,
  PHOSH_SHELL_LAYOUT_DEVICE   = 1,
} PhoshShellLayout;


typedef enum {
  PHOSH_LAYOUT_CLOCK_POS_CENTER = 0,
  PHOSH_LAYOUT_CLOCK_POS_LEFT   = 1,
  PHOSH_LAYOUT_CLOCK_POS_RIGHT  = 2,
} PhoshLayoutClockPosition;

/**
 * PhoshNotifyScreenWakeupFlags:
 * @PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_NONE: No wakeup
 * @PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_URGENCY: Wakeup screen based on notification urgency
 * @PHOSH_NOTIFY_SCREEN_WAKUP_FLAG_CATEGORY: Wakeup screen based on notification category
 * @PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_ANY: Wakeup screen on any notification
 *
 * What notification properties trigger screen wakeup
 */
typedef enum {
  PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_NONE     = 0, /*< skip >*/
  PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_ANY      = (1 << 0),
  PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_URGENCY  = (1 << 1),
  PHOSH_NOTIFY_SCREEN_WAKEUP_FLAG_CATEGORY = (1 << 2),
} PhoshNotifyScreenWakeupFlags;

/**
 * PhoshWWanBackend:
 * @PHOSH_WWAN_BACKEND_MM: Use ModemManager
 * @PHOSH_WWAN_BACKEND_OFONO: Use oFono
 *
 * Which WWAN backend to use.
 */
typedef enum /*< enum,prefix=PHOSH >*/
{
  PHOSH_WWAN_BACKEND_MM,    /*< nick=modemmanager >*/
  PHOSH_WWAN_BACKEND_OFONO, /*< nick=ofono >*/
} PhoshWWanBackend;
