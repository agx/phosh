/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-fader"

#include "config.h"
#include "fader.h"
#include "shell.h"

#include <handy.h>

/**
 * SECTION:fader
 * @short_description: A fader
 * @Title: PhoshFader
 *
 * A fullsreen surface that fades in or out.
 */

#define PHOSH_FADER_DEFAULT_STYLE_CLASS "phosh-fader-default-fade"

enum {
  PROP_0,
  PROP_MONITOR,
  PROP_STYLE_CLASS,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct _PhoshFader
{
  PhoshLayerSurface           parent;
  PhoshMonitor               *monitor;
  char                       *style_class;
} PhoshFader;
G_DEFINE_TYPE (PhoshFader, phosh_fader, PHOSH_TYPE_LAYER_SURFACE)


static void
phosh_fader_set_property (GObject      *obj,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  PhoshFader *self = PHOSH_FADER (obj);

  switch (prop_id) {
  case PROP_MONITOR:
    /* construct only */
    g_set_object (&self->monitor, g_value_get_object (value));
    break;
  case PROP_STYLE_CLASS:
    g_free (self->style_class);
    self->style_class = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    break;
  }
}


static void
phosh_fader_get_property (GObject    *obj,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  PhoshFader *self = PHOSH_FADER (obj);

  switch (prop_id) {
  case PROP_MONITOR:
    g_value_set_object (value, self->monitor);
    break;
  case PROP_STYLE_CLASS:
    g_value_set_string (value, self->style_class);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    break;
  }
}


static void
phosh_fader_show (GtkWidget *widget)
{
  PhoshFader *self = PHOSH_FADER (widget);
  gboolean enable_animations;
  GtkStyleContext *context;

  enable_animations = hdy_get_enable_animations (widget);

  if (enable_animations) {
    const char *style_class;

    style_class = self->style_class ?: PHOSH_FADER_DEFAULT_STYLE_CLASS;
    context = gtk_widget_get_style_context (widget);
    gtk_style_context_add_class (context, style_class);
  }

  GTK_WIDGET_CLASS (phosh_fader_parent_class)->show (widget);
}


static void
phosh_fader_dispose (GObject *object)
{
  PhoshFader *self = PHOSH_FADER (object);

  g_clear_object (&self->monitor);
  g_clear_pointer (&self->style_class, g_free);

  G_OBJECT_CLASS (phosh_fader_parent_class)->dispose (object);
}


static void
phosh_fader_constructed (GObject *object)
{
  PhoshFader *self = PHOSH_FADER (object);
  PhoshWayland *wl = phosh_wayland_get_default ();

  if (self->monitor == NULL)
    self->monitor = g_object_ref (phosh_shell_get_primary_monitor (phosh_shell_get_default ()));

  g_object_set (PHOSH_LAYER_SURFACE (self),
                "layer-shell", phosh_wayland_get_zwlr_layer_shell_v1 (wl),
                "wl-output", phosh_monitor_get_wl_output (self->monitor),
                "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                "layer", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
                "kbd-interactivity", FALSE,
                "exclusive-zone", -1,
                "namespace", "phosh fader",
                NULL);

  G_OBJECT_CLASS (phosh_fader_parent_class)->constructed (object);
}


static void
phosh_fader_class_init (PhoshFaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_fader_get_property;
  object_class->set_property = phosh_fader_set_property;
  object_class->constructed = phosh_fader_constructed;
  object_class->dispose = phosh_fader_dispose;
  widget_class->show = phosh_fader_show;

  props[PROP_MONITOR] = g_param_spec_object ("monitor",
                                             "",
                                             "",
                                             PHOSH_TYPE_MONITOR,
                                             G_PARAM_CONSTRUCT_ONLY |
                                             G_PARAM_READWRITE |
                                             G_PARAM_STATIC_STRINGS);

  /**
   * PhoshFader:style-class
   *
   * The CSS style class used for the animation
   */
  props[PROP_STYLE_CLASS] = g_param_spec_string ("style-class",
                                                 "",
                                                 "",
                                                 NULL,
                                                 G_PARAM_CONSTRUCT_ONLY |
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_fader_init (PhoshFader *self)
{
  self->style_class = g_strdup (PHOSH_FADER_DEFAULT_STYLE_CLASS);
}


PhoshFader *
phosh_fader_new (PhoshMonitor *monitor)
{
  return g_object_new (PHOSH_TYPE_FADER, "monitor", monitor, NULL);
}
