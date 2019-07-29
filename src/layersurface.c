/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-layer-surface"

#include "config.h"
#include "layersurface.h"
#include "phosh-wayland.h"

#include <gdk/gdkwayland.h>

enum {
  PHOSH_LAYER_SURFACE_PROP_0,
  PHOSH_LAYER_SURFACE_PROP_LAYER_SHELL,
  PHOSH_LAYER_SURFACE_PROP_WL_OUTPUT,
  PHOSH_LAYER_SURFACE_PROP_ANCHOR,
  PHOSH_LAYER_SURFACE_PROP_LAYER,
  PHOSH_LAYER_SURFACE_PROP_KBD_INTERACTIVITY,
  PHOSH_LAYER_SURFACE_PROP_EXCLUSIVE_ZONE,
  PHOSH_LAYER_SURFACE_PROP_LAYER_WIDTH,
  PHOSH_LAYER_SURFACE_PROP_LAYER_HEIGHT,
  PHOSH_LAYER_SURFACE_PROP_NAMESPACE,
  PHOSH_LAYER_SURFACE_PROP_LAST_PROP
};
static GParamSpec *props[PHOSH_LAYER_SURFACE_PROP_LAST_PROP];

enum {
  CONFIGURED,
  N_SIGNALS
};
static guint signals [N_SIGNALS];


typedef struct {
  struct wl_surface *wl_surface;
  struct zwlr_layer_surface_v1 *layer_surface;

  /* Properties */
  guint anchor;
  guint layer;
  gboolean kbd_interactivity;
  gint exclusive_zone;
  gint width, height;
  gchar *namespace;
  struct zwlr_layer_shell_v1 *layer_shell;
  struct wl_output *wl_output;
} PhoshLayerSurfacePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshLayerSurface, phosh_layer_surface, GTK_TYPE_WINDOW)

static void layer_surface_configure(void                         *data,
                                    struct zwlr_layer_surface_v1 *surface,
                                    uint32_t                      serial,
                                    uint32_t                      width,
                                    uint32_t                      height)
{
  PhoshLayerSurface *self = data;

  gtk_window_resize (GTK_WINDOW (self), width, height);
  zwlr_layer_surface_v1_ack_configure(surface, serial);

  g_signal_emit (self, signals[CONFIGURED], 0);
}


static void layer_surface_closed (void                         *data,
                                  struct zwlr_layer_surface_v1 *surface)
{
  PhoshLayerSurface *self = data;
  PhoshLayerSurfacePrivate *priv = phosh_layer_surface_get_instance_private (self);

  g_return_if_fail (priv->layer_surface == surface);
  zwlr_layer_surface_v1_destroy(priv->layer_surface);
  priv->layer_surface = NULL;
  gtk_widget_destroy (GTK_WIDGET (self));
}

static struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_configure,
	.closed = layer_surface_closed,
};

static void
phosh_layer_surface_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  PhoshLayerSurface *self = PHOSH_LAYER_SURFACE (object);
  PhoshLayerSurfacePrivate *priv = phosh_layer_surface_get_instance_private (self);

  switch (property_id) {
  case PHOSH_LAYER_SURFACE_PROP_LAYER_SHELL:
    priv->layer_shell = g_value_get_pointer (value);
    break;
  case PHOSH_LAYER_SURFACE_PROP_WL_OUTPUT:
    priv->wl_output = g_value_get_pointer (value);
    break;
  case PHOSH_LAYER_SURFACE_PROP_ANCHOR:
    priv->anchor = g_value_get_uint (value);
    break;
  case PHOSH_LAYER_SURFACE_PROP_LAYER:
    priv->layer = g_value_get_uint (value);
    break;
  case PHOSH_LAYER_SURFACE_PROP_KBD_INTERACTIVITY:
    priv->kbd_interactivity = g_value_get_boolean (value);
    break;
  case PHOSH_LAYER_SURFACE_PROP_EXCLUSIVE_ZONE:
    priv->exclusive_zone = g_value_get_int (value);
    break;
  case PHOSH_LAYER_SURFACE_PROP_LAYER_WIDTH:
    priv->width = g_value_get_uint (value);
    break;
  case PHOSH_LAYER_SURFACE_PROP_LAYER_HEIGHT:
    priv->height = g_value_get_uint (value);
    break;
  case PHOSH_LAYER_SURFACE_PROP_NAMESPACE:
    g_free (priv->namespace);
    priv->namespace = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_layer_surface_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  PhoshLayerSurface *self = PHOSH_LAYER_SURFACE (object);
  PhoshLayerSurfacePrivate *priv = phosh_layer_surface_get_instance_private (self);

  switch (property_id) {
  case PHOSH_LAYER_SURFACE_PROP_LAYER_SHELL:
    g_value_set_pointer (value, priv->layer_shell);
    break;
  case PHOSH_LAYER_SURFACE_PROP_WL_OUTPUT:
    g_value_set_pointer (value, priv->wl_output);
    break;
  case PHOSH_LAYER_SURFACE_PROP_ANCHOR:
    g_value_set_uint (value, priv->anchor);
    break;
  case PHOSH_LAYER_SURFACE_PROP_LAYER:
    g_value_set_uint (value, priv->layer);
    break;
  case PHOSH_LAYER_SURFACE_PROP_KBD_INTERACTIVITY:
    g_value_set_boolean (value, priv->kbd_interactivity);
    break;
  case PHOSH_LAYER_SURFACE_PROP_EXCLUSIVE_ZONE:
    g_value_set_int (value, priv->exclusive_zone);
    break;
  case PHOSH_LAYER_SURFACE_PROP_LAYER_WIDTH:
    g_value_set_uint (value, priv->width);
    break;
  case PHOSH_LAYER_SURFACE_PROP_LAYER_HEIGHT:
    g_value_set_uint (value, priv->height);
    break;
  case PHOSH_LAYER_SURFACE_PROP_NAMESPACE:
    g_value_set_string (value, priv->namespace);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
on_phosh_layer_surface_realized (PhoshLayerSurface *self, gpointer unused)
{
  PhoshLayerSurfacePrivate *priv;
  GdkWindow *gdk_window;

  g_return_if_fail (PHOSH_IS_LAYER_SURFACE (self));

  priv = phosh_layer_surface_get_instance_private (self);

  gdk_window = gtk_widget_get_window (GTK_WIDGET (self));
  gdk_wayland_window_set_use_custom_surface (gdk_window);
  priv->wl_surface = gdk_wayland_window_get_wl_surface (gdk_window);

  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
}


static void
on_phosh_layer_surface_mapped (PhoshLayerSurface *self, gpointer unused)
{
  PhoshLayerSurfacePrivate *priv;
  GdkWindow *gdk_window;

  g_return_if_fail (PHOSH_IS_LAYER_SURFACE (self));
  priv = phosh_layer_surface_get_instance_private (self);

  if (!priv->wl_surface) {
      gdk_window = gtk_widget_get_window (GTK_WIDGET (self));
      gdk_wayland_window_set_use_custom_surface (gdk_window);
      priv->wl_surface = gdk_wayland_window_get_wl_surface (gdk_window);
  }
  g_debug ("Mapped %p", priv->wl_surface);

  priv->layer_surface = zwlr_layer_shell_v1_get_layer_surface(priv->layer_shell,
                                                              priv->wl_surface,
                                                              priv->wl_output,
                                                              priv->layer,
                                                              priv->namespace);
  zwlr_layer_surface_v1_set_exclusive_zone(priv->layer_surface, priv->exclusive_zone);
  zwlr_layer_surface_v1_set_size(priv->layer_surface, priv->width, priv->height);
  zwlr_layer_surface_v1_set_anchor(priv->layer_surface, priv->anchor);
  zwlr_layer_surface_v1_set_keyboard_interactivity(priv->layer_surface, priv->kbd_interactivity);
  zwlr_layer_surface_v1_add_listener(priv->layer_surface,
                                     &layer_surface_listener,
                                     self);
  wl_surface_commit(priv->wl_surface);

  /* Process all pending events, otherwise we end up sending ack configure
   * to a not yet configured surface */
  wl_display_roundtrip (gdk_wayland_display_get_wl_display (gdk_display_get_default ()));
}

static void
on_phosh_layer_surface_unmapped (PhoshLayerSurface *self, gpointer unused)
{
  PhoshLayerSurfacePrivate *priv;

  g_return_if_fail (PHOSH_IS_LAYER_SURFACE (self));
  priv = phosh_layer_surface_get_instance_private (self);

  priv = phosh_layer_surface_get_instance_private (self);
  if (priv->layer_surface) {
    zwlr_layer_surface_v1_destroy(priv->layer_surface);
    priv->layer_surface = NULL;
  }
  priv->wl_surface = NULL;
}

static void
phosh_layer_surface_constructed (GObject *object)
{
  PhoshLayerSurface *self = PHOSH_LAYER_SURFACE (object);

  g_signal_connect (self, "realize",
                    G_CALLBACK (on_phosh_layer_surface_realized),
                    NULL);
  g_signal_connect (self, "map",
                    G_CALLBACK (on_phosh_layer_surface_mapped),
                    NULL);
  g_signal_connect (self, "unmap",
                    G_CALLBACK (on_phosh_layer_surface_unmapped),
                    NULL);
}


static void
phosh_layer_surface_dispose (GObject *object)
{
  PhoshLayerSurface *self = PHOSH_LAYER_SURFACE (object);
  PhoshLayerSurfacePrivate *priv = phosh_layer_surface_get_instance_private (self);

  if (priv->layer_surface) {
    zwlr_layer_surface_v1_destroy(priv->layer_surface);
    priv->layer_surface = NULL;
  }
  g_clear_pointer (&priv->namespace, g_free);

  G_OBJECT_CLASS (phosh_layer_surface_parent_class)->dispose (object);
}


static void
phosh_layer_surface_class_init (PhoshLayerSurfaceClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = phosh_layer_surface_constructed;
  object_class->dispose = phosh_layer_surface_dispose;

  object_class->set_property = phosh_layer_surface_set_property;
  object_class->get_property = phosh_layer_surface_get_property;

  props[PHOSH_LAYER_SURFACE_PROP_LAYER_SHELL] =
    g_param_spec_pointer (
      "layer-shell",
      "Wayland Layer Shell Global",
      "The layer shell wayland global",
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PHOSH_LAYER_SURFACE_PROP_WL_OUTPUT] =
    g_param_spec_pointer (
      "wl-output",
      "Wayland Output",
      "The wl_output associated with this surface",
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PHOSH_LAYER_SURFACE_PROP_ANCHOR] =
    g_param_spec_uint (
      "anchor",
      "Anchor edges",
      "The edges to anchor the surface to",
      0,
      G_MAXUINT,
      0,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PHOSH_LAYER_SURFACE_PROP_LAYER] =
    g_param_spec_uint (
      "layer",
      "Layer",
      "The layer the surface should be attached to",
      0,
      G_MAXUINT,
      0,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PHOSH_LAYER_SURFACE_PROP_KBD_INTERACTIVITY] =
    g_param_spec_boolean (
      "kbd-interactivity",
      "Keyboard interactivity",
      "Whether the surface interacts with the keyboard",
      FALSE,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PHOSH_LAYER_SURFACE_PROP_EXCLUSIVE_ZONE] =
    g_param_spec_int (
      "exclusive-zone",
      "Exclusive Zone",
      "Set area that is not occluded with other surfaces",
      -1,
      G_MAXINT,
      0,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PHOSH_LAYER_SURFACE_PROP_LAYER_WIDTH] =
    g_param_spec_uint (
      "width",
      "Width",
      "The width of the layer surface",
      0,
      G_MAXUINT,
      0,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PHOSH_LAYER_SURFACE_PROP_LAYER_HEIGHT] =
    g_param_spec_uint (
      "height",
      "Height",
      "The height of the layer surface",
      0,
      G_MAXUINT,
      0,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PHOSH_LAYER_SURFACE_PROP_NAMESPACE] =
    g_param_spec_string (
      "namespace",
      "Namespace",
      "Namespace of the layer surface",
      "",
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PHOSH_LAYER_SURFACE_PROP_LAST_PROP, props);

  /**
   * PhoshLayersurface::configured
   * @self: The #PhoshLayersurface instance.
   *
   * This signal is emitted once we received the configure event from the
   * compositor.
   */
  signals[CONFIGURED] =
    g_signal_new ("configured",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (PhoshLayerSurfaceClass, configured),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}


static void
phosh_layer_surface_init (PhoshLayerSurface *self)
{
}


GtkWidget *
phosh_layer_surface_new (gpointer layer_shell,
                         gpointer wl_output)
{
  return g_object_new (PHOSH_TYPE_LAYER_SURFACE,
                       "layer-shell", layer_shell,
                       "wl-output", wl_output);
}

/**
 * phosh_layer_surface_get_surface:
 *
 * Get the layer layer surface or #NULL if the window
 * is not yet realized.
 */
struct zwlr_layer_surface_v1 *
phosh_layer_surface_get_layer_surface(PhoshLayerSurface *self)
{
  PhoshLayerSurfacePrivate *priv;

  g_return_val_if_fail (PHOSH_IS_LAYER_SURFACE (self), NULL);
  priv = phosh_layer_surface_get_instance_private (self);
  return priv->layer_surface;
}


/**
 * phosh_layer_surface_get_wl_surface:
 *
 * Get the layer wayland surface or #NULL if the window
 * is not yet realized.
 */
struct wl_surface *
phosh_layer_surface_get_wl_surface(PhoshLayerSurface *self)
{
  PhoshLayerSurfacePrivate *priv;

  g_return_val_if_fail (PHOSH_IS_LAYER_SURFACE (self), NULL);
  priv = phosh_layer_surface_get_instance_private (self);
  return priv->wl_surface;
}

/**
 * phosh_layer_surface_set_size:
 *
 * Set the size of a layer surface. A value of '-1' indicates 'use old value'
 */
void
phosh_layer_surface_set_size(PhoshLayerSurface *self, gint width, gint height)
{
  PhoshLayerSurfacePrivate *priv;

  g_return_if_fail (PHOSH_IS_LAYER_SURFACE (self));
  priv = phosh_layer_surface_get_instance_private (self);

  if (priv->height == height && priv->width == width)
    return;

  if (width != -1)
    priv->width = width;

  if (height != -1)
    priv->height = height;

  zwlr_layer_surface_v1_set_size(priv->layer_surface, priv->width, priv->height);
}
