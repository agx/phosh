/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

/**
 * A monitor. Currently we just wrap GDK but this will change once we have
 * outputdevice handling in wlroots:
 * https://github.com/swaywm/wlr-protocols/issues/15
 */

#define G_LOG_DOMAIN "phosh-monitor"

#include "monitor.h"
#include <gdk/gdkwayland.h>

enum {
  PHOSH_MONITOR_PROP_0,
  PHOSH_MONITOR_PROP_WL_OUTPUT,
  PHOSH_MONITOR_PROP_LAST_PROP,
};
static GParamSpec *props[PHOSH_MONITOR_PROP_LAST_PROP];

G_DEFINE_TYPE (PhoshMonitor, phosh_monitor, G_TYPE_OBJECT)

static void
output_handle_geometry (void             *data,
                        struct wl_output *wl_output,
                        int               x,
                        int               y,
                        int               physical_width,
                        int               physical_height,
                        int               subpixel,
                        const char       *make,
                        const char       *model,
                        int32_t           transform)
{
  PhoshMonitor *self = PHOSH_MONITOR (data);

  g_debug ("handle geometry output %p, position %d %d, size %dx%d, subpixel layout %d, vendor %s, "
           "product %s, transform %d",
           self, x, y, physical_width, physical_height, subpixel, make, model, transform);

  self->x = x;
  self->y = y;
  self->width_mm = physical_width;
  self->height_mm = physical_height;
  self->subpixel = subpixel;
  self->vendor = g_strdup (make);
  self->product = g_strdup (model);
}


static void
output_handle_done (void             *data,
                    struct wl_output *wl_output)
{
  PhoshMonitor *self = PHOSH_MONITOR (data);

  self->wl_output_done = TRUE;
}


static void
output_handle_scale (void             *data,
                     struct wl_output *wl_output,
                     int32_t           scale)
{
  PhoshMonitor *self = PHOSH_MONITOR (data);

  self->scale = scale;
}


static void
output_handle_mode (void             *data,
                    struct wl_output *wl_output,
                    uint32_t          flags,
                    int               width,
                    int               height,
                    int               refresh)
{
  PhoshMonitor *self = PHOSH_MONITOR (data);
  PhoshMonitorMode mode;

  g_debug ("handle mode output %p: %dx%d@%d",
           self, width, height, refresh);

  mode.width = width;
  mode.height = height;
  mode.flags = flags;
  mode.refresh = refresh;

  g_array_append_val (self->modes, mode);

  if (width > self->width)
    self->width = width;

  if (height > self->height)
    self->height = height;

  if (flags & WL_OUTPUT_MODE_CURRENT)
    self->current_mode = self->modes->len - 1;

  if (flags & WL_OUTPUT_MODE_PREFERRED)
    self->preferred_mode = self->modes->len - 1;
}


static const struct wl_output_listener output_listener =
{
  output_handle_geometry,
  output_handle_mode,
  output_handle_done,
  output_handle_scale,
};


static void
xdg_output_v1_handle_logical_position (void *data,
                                       struct zxdg_output_v1 *zxdg_output_v1,
                                       int32_t x,
                                       int32_t y)
{
  /* TODO: use this */
}


static void
xdg_output_v1_handle_logical_size (void *data,
                                   struct zxdg_output_v1 *zxdg_output_v1,
                                   int32_t width,
                                   int32_t height)
{
  /* Nothing todo atm */
}

static void
xdg_output_v1_handle_done (void *data,
                           struct zxdg_output_v1 *zxdg_output_v1)
{
  PhoshMonitor *self = PHOSH_MONITOR (data);

  self->xdg_output_done = TRUE;
}


static void
xdg_output_v1_handle_name (void *data,
                           struct zxdg_output_v1 *zxdg_output_v1,
                           const char *name)
{
  PhoshMonitor *self = PHOSH_MONITOR (data);
  /* wlroots uses the connector's name as xdg_output name */
  g_debug("Connector name is %s", name);

  self->name = g_strdup (name);

  /* wlroots uses the connector's name as output name so
     try to derive the connector type from it */
  if (g_str_has_prefix (name, "LVDS-"))
    self->conn_type = PHOSH_MONITOR_CONNECTOR_TYPE_LVDS;
  else if (g_str_has_prefix (name, "HDMI-A-"))
    self->conn_type = PHOSH_MONITOR_CONNECTOR_TYPE_HDMIA;
  else if (g_str_has_prefix (name, "eDP-"))
      self->conn_type = PHOSH_MONITOR_CONNECTOR_TYPE_eDP;
  else if (g_str_has_prefix (name, "DSI-"))
    self->conn_type = PHOSH_MONITOR_CONNECTOR_TYPE_DSI;
  else
    self->conn_type = PHOSH_MONITOR_CONNECTOR_TYPE_Unknown;
}


static void
xdg_output_v1_handle_description(void *data,
                                 struct zxdg_output_v1 *zxdg_output_v1,
                                 const char *description)
{
  g_debug("Output description is %s", description);
}


static const struct zxdg_output_v1_listener xdg_output_v1_listener =
{
  xdg_output_v1_handle_logical_position,
  xdg_output_v1_handle_logical_size,
  xdg_output_v1_handle_done,
  xdg_output_v1_handle_name,
  xdg_output_v1_handle_description,
};


static void
phosh_monitor_set_property (GObject *object,
                          guint property_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
  PhoshMonitor *self = PHOSH_MONITOR (object);

  switch (property_id) {
  case PHOSH_MONITOR_PROP_WL_OUTPUT:
    self->wl_output = g_value_get_pointer (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_monitor_get_property (GObject *object,
                          guint property_id,
                          GValue *value,
                          GParamSpec *pspec)
{
  PhoshMonitor *self = PHOSH_MONITOR (object);

  switch (property_id) {
  case PHOSH_MONITOR_PROP_WL_OUTPUT:
    g_value_set_pointer (value, self->wl_output);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_monitor_dispose (GObject *object)
{
  PhoshMonitor *self = PHOSH_MONITOR (object);

  g_array_free (self->modes, TRUE);
  self->modes = NULL;

  g_clear_pointer (&self->vendor, g_free);
  g_clear_pointer (&self->product, g_free);
  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->xdg_output, zxdg_output_v1_destroy);

  G_OBJECT_CLASS (phosh_monitor_parent_class)->dispose (object);
}


static void
phosh_monitor_constructed (GObject *object)
{
  PhoshMonitor *self = PHOSH_MONITOR (object);

  wl_output_add_listener (self->wl_output, &output_listener, self);

  self->xdg_output =
    zxdg_output_manager_v1_get_xdg_output(phosh_wayland_get_zxdg_output_manager_v1(phosh_wayland_get_default()),
                                          self->wl_output);
  g_return_if_fail (self->xdg_output);
  zxdg_output_v1_add_listener (self->xdg_output, &xdg_output_v1_listener, self);
}


static void
phosh_monitor_class_init (PhoshMonitorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_monitor_constructed;
  object_class->dispose = phosh_monitor_dispose;

  object_class->set_property = phosh_monitor_set_property;
  object_class->get_property = phosh_monitor_get_property;

  props[PHOSH_MONITOR_PROP_WL_OUTPUT] =
    g_param_spec_pointer ("wl-output",
                          "wl-output",
                          "The wayland output associated with this monitor",
                          G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (object_class, PHOSH_MONITOR_PROP_LAST_PROP, props);
}


static void
phosh_monitor_init (PhoshMonitor *self)
{
  self->scale = 1.0;
  self->modes = g_array_new (FALSE, FALSE, sizeof(PhoshMonitorMode));
}


PhoshMonitor *
phosh_monitor_new_from_wl_output (gpointer wl_output)
{
  return g_object_new (PHOSH_TYPE_MONITOR, "wl-output", wl_output, NULL);
}


PhoshMonitorMode *
phosh_monitor_get_current_mode (PhoshMonitor *self)
{
  g_return_val_if_fail (PHOSH_IS_MONITOR (self), NULL);
  g_return_val_if_fail (self->current_mode < self->modes->len, NULL);
  return &g_array_index (self->modes, PhoshMonitorMode, self->current_mode);
}

/** phosh_monitor_is_configured:
 *
 * Is the monitor fully configured (did we receive all data from the compositor)?
 */
gboolean
phosh_monitor_is_configured (PhoshMonitor *self)
{
  g_return_val_if_fail (PHOSH_IS_MONITOR (self), FALSE);
  return self->wl_output_done && self->xdg_output_done;
}


/** phosh_monitor_is_builtin
 *
 * Is the monitor built in panel (e.g. laptop panel or phone LCD)
 */
gboolean
phosh_monitor_is_builtin (PhoshMonitor *self)
{
  g_return_val_if_fail (PHOSH_IS_MONITOR (self), FALSE);

  switch (self->conn_type) {
  case PHOSH_MONITOR_CONNECTOR_TYPE_eDP:
  case PHOSH_MONITOR_CONNECTOR_TYPE_LVDS:
  case PHOSH_MONITOR_CONNECTOR_TYPE_DSI:
    return TRUE;
  case PHOSH_MONITOR_CONNECTOR_TYPE_Unknown:
  case PHOSH_MONITOR_CONNECTOR_TYPE_VGA:
  case PHOSH_MONITOR_CONNECTOR_TYPE_DVII:
  case PHOSH_MONITOR_CONNECTOR_TYPE_DVID:
  case PHOSH_MONITOR_CONNECTOR_TYPE_DVIA:
  case PHOSH_MONITOR_CONNECTOR_TYPE_Composite:
  case PHOSH_MONITOR_CONNECTOR_TYPE_SVIDEO:
  case PHOSH_MONITOR_CONNECTOR_TYPE_Component:
  case PHOSH_MONITOR_CONNECTOR_TYPE_9PinDIN:
  case PHOSH_MONITOR_CONNECTOR_TYPE_DisplayPort:
  case PHOSH_MONITOR_CONNECTOR_TYPE_HDMIA:
  case PHOSH_MONITOR_CONNECTOR_TYPE_HDMIB:
  case PHOSH_MONITOR_CONNECTOR_TYPE_TV:
  case PHOSH_MONITOR_CONNECTOR_TYPE_VIRTUAL:
  default:
    return FALSE;
  }
}
