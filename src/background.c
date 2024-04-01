/*
 * Copyright (C) 2018-2022 Purism SPC
 *               2023-2024 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Derived in parts from GnomeBG which is
 *
 * Copyright (C) 2000 Eazel, Inc.
 * Copyright (C) 2007-2008 Red Hat, Inc.
 */

#define G_LOG_DOMAIN "phosh-background"

#include "background.h"
#include "background-cache.h"
#include "background-image.h"
#include "background-manager.h"
#include "shell.h"
#include "top-panel.h"
#include "util.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-bg.h>
#include <libgnome-desktop/gnome-bg-slide-show.h>

#include <gio/gio.h>

#include <math.h>
#include <string.h>

#define COLOR_TO_PIXEL(color)     ((((int)(color->red   * 255)) << 24) | \
                                   (((int)(color->green * 255)) << 16) | \
                                   (((int)(color->blue  * 255)) << 8)  | \
                                   (((int)(color->alpha * 255))))
/**
 * PhoshBackground:
 *
 * A [type@LayerSurface] representing the background drawn on a
 * [type@Monitor].
 *
 * The background is updated by [type@BackgroundManager] when needed.
 */

enum {
  PROP_0,
  PROP_PRIMARY,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


struct _PhoshBackground
{
  PhoshLayerSurface        parent;

  /* The cached background image */
  GFile                   *uri;
  PhoshBackgroundImage    *cached_bg_image;
  GCancellable            *cancel_load;
  /* How the background in rendered */
  GDesktopBackgroundStyle  style;
  GdkRGBA                  color;
  GdkPixbuf               *pixbuf;
  gboolean                 needs_update;

  /* The monitor backed by PhoshBackground */
  gboolean                 primary;
  gboolean                 configured;
};


G_DEFINE_TYPE (PhoshBackground, phosh_background, PHOSH_TYPE_LAYER_SURFACE);


void
phosh_background_data_free (PhoshBackgroundData *bd_data)
{
  g_clear_object (&bd_data->uri);
  g_free (bd_data);
}


static void
phosh_background_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  PhoshBackground *self = PHOSH_BACKGROUND (object);

  switch (property_id) {
  case PROP_PRIMARY:
    phosh_background_set_primary (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_background_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PhoshBackground *self = PHOSH_BACKGROUND (object);

  switch (property_id) {
  case PROP_PRIMARY:
    g_value_set_boolean (value, self->primary);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static GdkPixbuf *
pb_scale_to_min (GdkPixbuf *src, int min_width, int min_height)
{
  double factor;
  int src_width, src_height;
  int new_width, new_height;
  GdkPixbuf *dest;

  g_return_val_if_fail (GDK_IS_PIXBUF (src), NULL);

  src_width = gdk_pixbuf_get_width (src);
  src_height = gdk_pixbuf_get_height (src);

  factor = MAX (min_width / (double) src_width, min_height / (double) src_height);

  new_width = floor (src_width * factor + 0.5);
  new_height = floor (src_height * factor + 0.5);

  dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                         gdk_pixbuf_get_has_alpha (src),
                         8, min_width, min_height);
  if (!dest)
    return NULL;

  /* crop the result */
  gdk_pixbuf_scale (src, dest,
                    0, 0,
                    min_width, min_height,
                    (new_width - min_width) / -2,
                    (new_height - min_height) / -2,
                    factor,
                    factor,
                    GDK_INTERP_BILINEAR);
  return dest;
}


static GdkPixbuf *
pb_fill_color (int width, int height, GdkRGBA *color)
{
  GdkPixbuf *bg;

  bg = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width, height);
  gdk_pixbuf_fill (bg, COLOR_TO_PIXEL(color));

  return bg;
}


static GdkPixbuf *
pb_scale_to_fit (GdkPixbuf *src, int width, int height, GdkRGBA *color)
{
  int orig_width, orig_height;
  int final_width, final_height;
  int off_x, off_y;
  double ratio_horiz, ratio_vert, ratio;
  GdkPixbuf *bg;

  bg = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width, height);
  gdk_pixbuf_fill (bg, COLOR_TO_PIXEL(color));

  orig_width = gdk_pixbuf_get_width (src);
  orig_height = gdk_pixbuf_get_height (src);
  ratio_horiz = (double) width / orig_width;
  ratio_vert = (double) height / orig_height;

  ratio = ratio_horiz > ratio_vert ? ratio_vert : ratio_horiz;
  final_width = ceil (ratio * orig_width);
  final_height = ceil (ratio * orig_height);

  off_x = (width - final_width) / 2;
  off_y = (height - final_height) / 2;
  gdk_pixbuf_composite (src,
                        bg,
                        off_x, off_y, /* dest x,y */
                        final_width,
                        final_height,
                        off_x, off_y, /* offset x, y */
                        ratio,
                        ratio,
                        GDK_INTERP_BILINEAR,
                        255);
  return bg;
}


static GdkPixbuf *
image_background (PhoshBackgroundImage    *image,
                  guint                    width,
                  guint                    height,
                  GDesktopBackgroundStyle  style,
                  GdkRGBA                 *color)
{
  GdkPixbuf *scaled_bg;

  if (image == NULL) {
    g_debug ("No image, using 'none' desktop style");
    style = G_DESKTOP_BACKGROUND_STYLE_NONE;
  }

  switch (style) {
  case G_DESKTOP_BACKGROUND_STYLE_SCALED:
    scaled_bg = pb_scale_to_fit (phosh_background_image_get_pixbuf (image), width, height, color);
    break;
  case G_DESKTOP_BACKGROUND_STYLE_NONE:
    scaled_bg = pb_fill_color (width, height, color);
    break;
  case G_DESKTOP_BACKGROUND_STYLE_WALLPAPER:
  case G_DESKTOP_BACKGROUND_STYLE_CENTERED:
  case G_DESKTOP_BACKGROUND_STYLE_STRETCHED:
  case G_DESKTOP_BACKGROUND_STYLE_SPANNED:
    g_warning ("Unimplemented style %d, using zoom", style);
    /* fallthrough */
  case G_DESKTOP_BACKGROUND_STYLE_ZOOM:
  default:
    scaled_bg = pb_scale_to_min (phosh_background_image_get_pixbuf (image), width, height);
    break;
  }

  return scaled_bg;
}


static gboolean
phosh_background_draw (GtkWidget *widget, cairo_t *cr)
{
  PhoshBackground *self = PHOSH_BACKGROUND (widget);
  int x = 0, y = 0;

  g_return_val_if_fail (PHOSH_IS_BACKGROUND (self), GDK_EVENT_PROPAGATE);

  if (!self->configured || !self->pixbuf)
    return GDK_EVENT_PROPAGATE;

  g_assert (GDK_IS_PIXBUF (self->pixbuf));

  if (self->primary)
    phosh_shell_get_usable_area (phosh_shell_get_default (), &x, &y, NULL, NULL);

  cairo_save (cr);
  gdk_cairo_set_source_pixbuf (cr, self->pixbuf, x, y);
  cairo_paint (cr);
  cairo_restore (cr);

  return GDK_EVENT_PROPAGATE;
}


static void
update_image (PhoshBackground *self)
{
  int width, height;

  if (!self->configured)
    return;

  if (self->primary) {
    phosh_shell_get_usable_area (phosh_shell_get_default (), NULL, NULL, &width, &height);
  } else {
    width = phosh_layer_surface_get_configured_width (PHOSH_LAYER_SURFACE (self));
    height = phosh_layer_surface_get_configured_height (PHOSH_LAYER_SURFACE (self));
  }

  g_return_if_fail (width > 0 && height > 0);

  g_debug ("Scaling background %p to %dx%d", self, width, height);

  g_clear_object (&self->pixbuf);
  self->pixbuf = image_background (self->cached_bg_image, width, height,
                                   self->style, &self->color);

  self->needs_update = FALSE;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}


static void
on_background_image_present (PhoshBackground        *self,
                             PhoshBackgroundImage   *image,
                             PhoshBackgroundCache   *cache)
{
  g_assert (PHOSH_IS_BACKGROUND (self));
  g_assert (PHOSH_IS_BACKGROUND_CACHE (cache));

  g_set_object (&self->cached_bg_image, image);
  update_image (self);
}


static void
trigger_update (PhoshBackground *self)
{
  PhoshBackgroundCache *cache = phosh_background_cache_get_default ();
  PhoshBackgroundManager *manager = phosh_shell_get_background_manager (phosh_shell_get_default ());
  g_autoptr (PhoshBackgroundData) bg_data = NULL;

  g_debug ("Updating Background %p", self);
  bg_data = phosh_background_manager_get_data (manager, self);

  self->needs_update = TRUE;
  self->style = bg_data->style;
  self->color = bg_data->color;
  g_set_object (&self->uri, bg_data->uri);

  g_cancellable_cancel (self->cancel_load);
  g_clear_object (&self->cancel_load);
  self->cancel_load = g_cancellable_new ();

  if (self->uri) {
    phosh_background_cache_fetch_background (cache, self->uri, self->cancel_load);
  } else {
    g_clear_object (&self->cached_bg_image);
    update_image (self);
  }
}


static void
phosh_background_configured (PhoshLayerSurface *layer_surface)
{
  PhoshBackground *self = PHOSH_BACKGROUND (layer_surface);

  PHOSH_LAYER_SURFACE_CLASS (phosh_background_parent_class)->configured (layer_surface);

  self->configured = TRUE;
  trigger_update (self);
}


static void
phosh_background_finalize (GObject *object)
{
  PhoshBackground *self = PHOSH_BACKGROUND (object);

  g_cancellable_cancel (self->cancel_load);
  g_clear_object (&self->cancel_load);
  g_clear_object (&self->pixbuf);
  g_clear_object (&self->cached_bg_image);

  G_OBJECT_CLASS (phosh_background_parent_class)->finalize (object);
}


static void
phosh_background_class_init (PhoshBackgroundClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  PhoshLayerSurfaceClass *layer_surface_class = PHOSH_LAYER_SURFACE_CLASS (klass);

  object_class->finalize = phosh_background_finalize;
  object_class->set_property = phosh_background_set_property;
  object_class->get_property = phosh_background_get_property;

  widget_class->draw = phosh_background_draw;

  layer_surface_class->configured = phosh_background_configured;

  /**
   * PhoshBackground:primary:
   *
   * Whether this is the background for the primary monitor.
   */
  props[PROP_PRIMARY] =
    g_param_spec_boolean ("primary",
                          "Primary",
                          "Primary monitor",
                          FALSE,
                          G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_CONSTRUCT);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_background_init (PhoshBackground *self)
{
  g_signal_connect_object (phosh_background_cache_get_default (),
                           "image-present",
                           G_CALLBACK (on_background_image_present),
                           self,
                           G_CONNECT_SWAPPED);
}


GtkWidget *
phosh_background_new (gpointer     layer_shell,
                      PhoshMonitor *monitor,
                      gboolean     primary)
{
  return g_object_new (PHOSH_TYPE_BACKGROUND,
                       "layer-shell", layer_shell,
                       "wl-output", monitor->wl_output,
                       "anchor", (ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                  ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                  ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                  ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT),
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", -1,
                       "namespace", "phosh background",
                       "primary", primary,
                       NULL);
}


void
phosh_background_set_primary (PhoshBackground *self, gboolean primary)
{
  if (self->primary == primary)
    return;

  self->primary = primary;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRIMARY]);

  trigger_update (self);
}

/**
 * phosh_background_needs_update:
 * @self: The background
 *
 * Marks the background's data as dirty, needing an update. This will make the update
 * it's background image.
 */
void
phosh_background_needs_update (PhoshBackground *self)
{
  g_return_if_fail (PHOSH_IS_BACKGROUND (self));

  trigger_update (self);
}
