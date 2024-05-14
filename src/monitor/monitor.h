/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#pragma once

#include "phosh-enums.h"
#include "phosh-wayland.h"

#include <gdk/gdk.h>
#include <glib-object.h>
#include <glib/gi18n.h>

G_BEGIN_DECLS

/**
 * PhoshMonitorConnectorType:
 * @PHOSH_MONITOR_CONNECTOR_TYPE_Unknown: unknown connector type
 * @PHOSH_MONITOR_CONNECTOR_TYPE_VGA: a VGA connector
 * @PHOSH_MONITOR_CONNECTOR_TYPE_DVII: a DVII connector
 * @PHOSH_MONITOR_CONNECTOR_TYPE_DVID: a DVID connector
 * @PHOSH_MONITOR_CONNECTOR_TYPE_DVIA: a DVIA connector
 * @PHOSH_MONITOR_CONNECTOR_TYPE_Composite: a Composite connector
 * @PHOSH_MONITOR_CONNECTOR_TYPE_SVIDEO: a SVIDEO connector
 * @PHOSH_MONITOR_CONNECTOR_TYPE_LVDS: a LVDS connector
 * @PHOSH_MONITOR_CONNECTOR_TYPE_Component: a Component connector
 * @PHOSH_MONITOR_CONNECTOR_TYPE_9PinDIN: a 9PinDIN connector
 * @PHOSH_MONITOR_CONNECTOR_TYPE_DisplayPort: a DisplayPort connector
 * @PHOSH_MONITOR_CONNECTOR_TYPE_HDMIA: a HDMIA connector
 * @PHOSH_MONITOR_CONNECTOR_TYPE_HDMIB: a HDMIB connector
 * @PHOSH_MONITOR_CONNECTOR_TYPE_TV: a TV connector
 * @PHOSH_MONITOR_CONNECTOR_TYPE_eDP: a eDP connector
 * @PHOSH_MONITOR_CONNECTOR_TYPE_VIRTUAL: a Virtual connector
 * @PHOSH_MONITOR_CONNECTOR_TYPE_DSI: a DSI connector
 *
 * This matches the values in drm_mode.h
 */
typedef enum _PhoshMonitorConnectorType
{
  PHOSH_MONITOR_CONNECTOR_TYPE_Unknown = 0,
  PHOSH_MONITOR_CONNECTOR_TYPE_VGA = 1,
  PHOSH_MONITOR_CONNECTOR_TYPE_DVII = 2,
  PHOSH_MONITOR_CONNECTOR_TYPE_DVID = 3,
  PHOSH_MONITOR_CONNECTOR_TYPE_DVIA = 4,
  PHOSH_MONITOR_CONNECTOR_TYPE_Composite = 5,
  PHOSH_MONITOR_CONNECTOR_TYPE_SVIDEO = 6,
  PHOSH_MONITOR_CONNECTOR_TYPE_LVDS = 7,
  PHOSH_MONITOR_CONNECTOR_TYPE_Component = 8,
  PHOSH_MONITOR_CONNECTOR_TYPE_9PinDIN = 9,
  PHOSH_MONITOR_CONNECTOR_TYPE_DisplayPort = 10,
  PHOSH_MONITOR_CONNECTOR_TYPE_HDMIA = 11,
  PHOSH_MONITOR_CONNECTOR_TYPE_HDMIB = 12,
  PHOSH_MONITOR_CONNECTOR_TYPE_TV = 13,
  PHOSH_MONITOR_CONNECTOR_TYPE_eDP = 14,
  PHOSH_MONITOR_CONNECTOR_TYPE_VIRTUAL = 15,
  PHOSH_MONITOR_CONNECTOR_TYPE_DSI = 16,
} PhoshMonitorConnectorType;

/**
 * PhoshMonitorTransform:
 * @PHOSH_MONITOR_TRANSFORM_NORMAL: normal
 * @PHOSH_MONITOR_TRANSFORM_90: 90 degree clockwise
 * @PHOSH_MONITOR_TRANSFORM_180: 180 degree clockwise
 * @PHOSH_MONITOR_TRANSFORM_270: 270 degree clockwise
 * @PHOSH_MONITOR_TRANSFORM_FLIPPED: flipped clockwise
 * @PHOSH_MONITOR_TRANSFORM_FLIPPED_90: flipped and 90 deg
 * @PHOSH_MONITOR_TRANSFORM_FLIPPED_180: flipped and 180 deg
 * @PHOSH_MONITOR_TRANSFORM_FLIPPED_270: flipped and 270 deg
 *
 * the monitors rotation. This corresponds to the values in
 * the org.gnome.Mutter.DisplayConfig DBus protocol.
 */
typedef enum _PhoshMonitorTransform
{
  PHOSH_MONITOR_TRANSFORM_NORMAL,
  PHOSH_MONITOR_TRANSFORM_90,
  PHOSH_MONITOR_TRANSFORM_180,
  PHOSH_MONITOR_TRANSFORM_270,
  PHOSH_MONITOR_TRANSFORM_FLIPPED,
  PHOSH_MONITOR_TRANSFORM_FLIPPED_90,
  PHOSH_MONITOR_TRANSFORM_FLIPPED_180,
  PHOSH_MONITOR_TRANSFORM_FLIPPED_270,
} PhoshMonitorTransform;


typedef struct _PhoshMonitorMode
{
  int width, height;
  int refresh;
  guint32 flags;
} PhoshMonitorMode;

/**
 * PhoshMonitorPowerSaveMode:
 * @PHOSH_MONITOR_POWER_SAVE_MODE_ON: The monitor is on
 * @PHOSH_MONITOR_POWER_SAVE_MODE_OFF: The monitor is off (saving power)
 *
 * The power save mode of a monitor
 */
typedef enum _PhoshMonitorPowerSaveMode {
  PHOSH_MONITOR_POWER_SAVE_MODE_OFF = 0,
  PHOSH_MONITOR_POWER_SAVE_MODE_ON  = 1,
} PhoshMonitorPowerSaveMode;

#define PHOSH_TYPE_MONITOR                 (phosh_monitor_get_type ())

struct _PhoshMonitor {
  GObject parent;

  struct wl_output *wl_output;
  struct zxdg_output_v1 *xdg_output;
  struct zwlr_output_power_v1 *wlr_output_power;
  PhoshMonitorPowerSaveMode power_mode;

  int x, y, width, height;
  int subpixel;
  gint32 transform;

  struct PhoshLogicalSize {
    gint32 x, y, width, height;
  } logical;

  int width_mm;
  int height_mm;

  char *vendor;
  char *product;
  char *description;

  GArray *modes;
  guint current_mode;
  guint preferred_mode;

  char *name;
  PhoshMonitorConnectorType conn_type;

  gboolean wl_output_done;

  struct zwlr_gamma_control_v1 *gamma_control;
  guint32 n_gamma_entries;
};

G_DECLARE_FINAL_TYPE (PhoshMonitor, phosh_monitor, PHOSH, MONITOR, GObject)

PhoshMonitor     * phosh_monitor_new_from_wl_output (gpointer wl_output);
PhoshMonitorMode * phosh_monitor_get_current_mode (PhoshMonitor *self);
gboolean           phosh_monitor_is_configured (PhoshMonitor *self);
gboolean           phosh_monitor_is_builtin (PhoshMonitor *self);
gboolean           phosh_monitor_is_flipped (PhoshMonitor *self);
gboolean           phosh_monitor_has_gamma (PhoshMonitor *self);
gboolean           phosh_monitor_set_color_temp (PhoshMonitor *self, guint32 temp);

guint              phosh_monitor_get_transform (PhoshMonitor *self);
void               phosh_monitor_set_power_save_mode (PhoshMonitor *self,
                                                      PhoshMonitorPowerSaveMode mode);
PhoshMonitorPowerSaveMode phosh_monitor_get_power_save_mode (PhoshMonitor *self);
PhoshMonitorConnectorType phosh_monitor_connector_type_from_name (const char *name);
gboolean           phosh_monitor_connector_is_builtin (PhoshMonitorConnectorType type);
struct wl_output * phosh_monitor_get_wl_output (PhoshMonitor *self);
float              phosh_monitor_get_fractional_scale (PhoshMonitor *self);
gboolean           phosh_monitor_is_preferred_mode (PhoshMonitor *self);

gboolean           phosh_monitor_transform_is_tilted (PhoshMonitorTransform transform);


G_END_DECLS
