/*
 * Copyright (C) 2024 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-top-panel-bg"

#include "phosh-config.h"

#include "top-panel-bg.h"

#define PHOSH_TOP_PANEL_BG_MAX_OPACITY 0.9

/**
 * PhoshTopPanelBgBg:
 *
 * The (semi transparent) background of the top panel
 */
enum {
  PROP_0,
  PROP_MAX_OPACITY,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshTopPanelBg {
  PhoshLayerSurface parent;

  double            max_opacity;
};
G_DEFINE_TYPE (PhoshTopPanelBg, phosh_top_panel_bg, PHOSH_TYPE_LAYER_SURFACE)


static void
phosh_top_panel_bg_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  PhoshTopPanelBg *self = PHOSH_TOP_PANEL_BG (object);

  switch (property_id) {
  case PROP_MAX_OPACITY:
    g_value_set_double (value, self->max_opacity);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_top_panel_bg_class_init (PhoshTopPanelBgClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_top_panel_bg_get_property;

  props[PROP_MAX_OPACITY] =
    g_param_spec_double ("max-opacity", "", "",
                         0.0, 1.0, PHOSH_TOP_PANEL_BG_MAX_OPACITY,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_css_name (widget_class, "phosh-top-panel-bg");
}


static void
phosh_top_panel_bg_init (PhoshTopPanelBg *self)
{
  self->max_opacity = PHOSH_TOP_PANEL_BG_MAX_OPACITY;
}


PhoshTopPanelBg *
phosh_top_panel_bg_new (struct zwlr_layer_shell_v1 *layer_shell,
                        PhoshMonitor               *monitor,
                        guint32                     layer)

{
  return g_object_new (PHOSH_TYPE_TOP_PANEL_BG,
                       "layer-shell", layer_shell,
                       "wl-output", monitor->wl_output,
                       "anchor", (ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                  ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                  ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                  ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT),
                       "layer", layer,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", -1,
                       "namespace", "phosh top-panel background",
                       NULL);
}

/**
 * phosh_top_panel_bg_set_transparency:
 * @self: The background
 * @transparency: the transparency
 *
 * Set's the backgrounds transparency between 1.0 (fully transparent) and
 * 0.0 (opaque). How opaque the surface actually is determined by
 * [property@TopPanelBg:max-opacity].
 */
void
phosh_top_panel_bg_set_transparency (PhoshTopPanelBg *self, double transparency)
{
  g_return_if_fail (PHOSH_IS_TOP_PANEL_BG (self));

  phosh_layer_surface_set_alpha (PHOSH_LAYER_SURFACE (self),
                                 self->max_opacity * (1.0 - transparency));
}
