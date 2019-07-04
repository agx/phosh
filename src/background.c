/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3+
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Derived in parts from GnomeBG which is
 *
 * Copyright (C) 2000 Eazel, Inc.
 * Copyright (C) 2007-2008 Red Hat, Inc.
 */

#define G_LOG_DOMAIN "phosh-background"

#include "background.h"
#include "shell.h"
#include "panel.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-bg.h>

#include <math.h>
#include <string.h>

#define BG_KEY_PRIMARY_COLOR      "primary-color"
#define BG_KEY_SECONDARY_COLOR    "secondary-color"
#define BG_KEY_COLOR_TYPE         "color-shading-type"
#define BG_KEY_PICTURE_OPTIONS    "picture-options"
#define BG_KEY_PICTURE_OPACITY    "picture-opacity"
#define BG_KEY_PICTURE_URI        "picture-uri"

enum {
  PROP_0,
  PROP_PRIMARY,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


enum {
  BACKGROUND_LOADED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];

struct _PhoshBackground
{
  PhoshLayerSurface parent;

  gchar *uri;
  GDesktopBackgroundStyle style;

  gboolean primary;
  GdkPixbuf *pixbuf;
  GSettings *settings;
};


G_DEFINE_TYPE (PhoshBackground, phosh_background, PHOSH_TYPE_LAYER_SURFACE);


static void
phosh_background_set_property (GObject *object,
                               guint property_id,
                               const GValue *value,
                               GParamSpec *pspec)
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
phosh_background_get_property (GObject *object,
                               guint property_id,
                               GValue *value,
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
pb_scale_to_fit (GdkPixbuf *src, int width, int height)
{
  gint orig_width, orig_height;
  gint final_width, final_height;
  gint off_x, off_y;
  gdouble ratio_horiz, ratio_vert, ratio;
  g_autoptr(GdkPixbuf) bg = NULL;
  GdkPixbuf *scaled_bg;
  const gchar *xpm_data[] = {"1 1 1 1", "_ c WebGrey", "_"};
  /* todo: use correct color */

  bg = gdk_pixbuf_new_from_xpm_data (xpm_data);
  scaled_bg = gdk_pixbuf_scale_simple (bg,
                                       width,
                                       height,
                                       GDK_INTERP_BILINEAR);

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
                        scaled_bg,
                        off_x, off_y, /* dest x,y */
                        final_width,
                        final_height,
                        off_x, off_y, /* offset x, y */
                        ratio,
                        ratio,
                        GDK_INTERP_BILINEAR,
                        255);
  return scaled_bg;
}

static GdkPixbuf *
image_background (GdkPixbuf *image, guint width, guint height, GDesktopBackgroundStyle style)
{
  GdkPixbuf *scaled_bg;

  switch (style) {
  case G_DESKTOP_BACKGROUND_STYLE_SCALED:
    scaled_bg = pb_scale_to_fit (image, width, height);
    break;
  case G_DESKTOP_BACKGROUND_STYLE_NONE:
  case G_DESKTOP_BACKGROUND_STYLE_WALLPAPER:
  case G_DESKTOP_BACKGROUND_STYLE_CENTERED:
  case G_DESKTOP_BACKGROUND_STYLE_STRETCHED:
  case G_DESKTOP_BACKGROUND_STYLE_SPANNED:
    g_warning ("Unimplemented style %d, using zoom", style);
    /* fallthrough */
  case G_DESKTOP_BACKGROUND_STYLE_ZOOM:
  default:
    scaled_bg = pb_scale_to_min (image, width, height);
    break;
  }

  return scaled_bg;
}


static void
color_from_string (const char *string,
                   GdkColor   *colorp)
{
        /* If all else fails use black */
        gdk_color_parse ("black", colorp);

        if (!string)
                return;

        gdk_color_parse (string, colorp);
}

#if 0
        ctype = g_settings_get_enum (settings, BG_KEY_COLOR_TYPE);


                static void
draw_color_area (GnomeBG *bg,
                 GdkPixbuf *dest,
                 GdkRectangle *rect)
{
        guint32 pixel;
        GdkRectangle extent;

        extent.x = 0;
        extent.y = 0;
        extent.width = gdk_pixbuf_get_width (dest);
        extent.height = gdk_pixbuf_get_height (dest);

        gdk_rectangle_intersect (rect, &extent, rect);

        switch (bg->color_type) {
        case G_DESKTOP_BACKGROUND_SHADING_SOLID:
                /* not really a big deal to ignore the area of interest */
                pixel = ((bg->primary.red >> 8) << 24)      |
                        ((bg->primary.green >> 8) << 16)    |
                        ((bg->primary.blue >> 8) << 8)      |
                        (0xff);

                gdk_pixbuf_fill (dest, pixel);
                break;

        case G_DESKTOP_BACKGROUND_SHADING_HORIZONTAL:
                pixbuf_draw_gradient (dest, TRUE, &(bg->primary), &(bg->secondary), rect);
                break;

        case G_DESKTOP_BACKGROUND_SHADING_VERTICAL:
                pixbuf_draw_gradient (dest, FALSE, &(bg->primary), &(bg->secondary), rect);
                break;

        default:
                break;
        }
}

#endif


static void
load_background (PhoshBackground *self)
{
  g_autoptr(GdkPixbuf) image = NULL;
  /* todo: use correct color */
  const gchar *xpm_data[] = {"1 1 1 1", "_ c WebGrey", "_"};
  GError *err = NULL;
  gint width, height;

  g_clear_object (&self->pixbuf);

  /* FIXME: support GnomeDesktop.BGSlideShow as well */
  if (!g_str_has_prefix(self->uri, "file:///")) {
    g_warning ("Only file URIs supported for backgrounds not %s", self->uri);
  } else {
    image = gdk_pixbuf_new_from_file (&self->uri[strlen("file://")], &err);
    if (!image) {
      const char *reason = err ? err->message : "unknown error";
      g_warning ("Failed to load background: %s", reason);
      if (err)
        g_clear_error (&err);
    }
  }

  /* Fallback if image can't be loaded */
  if (!image)
    image = gdk_pixbuf_new_from_xpm_data (xpm_data);

  if (self->primary)
    phosh_shell_get_usable_area (phosh_shell_get_default (), NULL, NULL, &width, &height);
  else
    g_object_get (self, "width", &width, "height", &height, NULL);
  
  self->pixbuf = image_background (image, width, height, self->style);

  /* force background redraw */
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_signal_emit(self, signals[BACKGROUND_LOADED], 0);
}


static gboolean
background_draw_cb (PhoshBackground *self,
                    cairo_t         *cr,
                    gpointer         data)
{
  gint x = 0, y = 0;

  g_return_val_if_fail (PHOSH_IS_BACKGROUND (self), TRUE);
  g_return_val_if_fail (GDK_IS_PIXBUF (self->pixbuf), TRUE);

  if (self->primary)
    phosh_shell_get_usable_area (phosh_shell_get_default (), &x, &y, NULL, NULL);

  gdk_cairo_set_source_pixbuf (cr, self->pixbuf, x, y);
  cairo_paint (cr);
  return TRUE;
}


static void
on_background_setting_changed (PhoshBackground *self,
                               const gchar     *key,
                               GSettings       *settings)
{
  g_return_if_fail (PHOSH_IS_BACKGROUND (self));
  g_return_if_fail (G_IS_SETTINGS (settings));

  g_free (self->uri);
  self->uri = g_settings_get_string (settings, BG_KEY_PICTURE_URI);
  self->style = g_settings_get_enum (settings, BG_KEY_PICTURE_OPTIONS);

  load_background (self);
}


static void
rotation_notify_cb (PhoshBackground *self,
                    GParamSpec *pspec,
                    PhoshShell *shell)
{
  g_return_if_fail (PHOSH_IS_BACKGROUND (self));
  g_return_if_fail (PHOSH_IS_SHELL (shell));

  on_background_setting_changed (self, NULL, self->settings);
}


static void
on_phosh_background_configured (PhoshLayerSurface *surface)
{
  PhoshBackground *self = PHOSH_BACKGROUND (surface);

  /* Load background initially */
  on_background_setting_changed (self, NULL, self->settings);
}


static void
phosh_background_constructed (GObject *object)
{
  PhoshBackground *self = PHOSH_BACKGROUND (object);

  G_OBJECT_CLASS (phosh_background_parent_class)->constructed (object);

  g_signal_connect (self, "draw", G_CALLBACK (background_draw_cb), NULL);

  self->settings = g_settings_new ("org.gnome.desktop.background");
  g_object_connect (self->settings,
                    "swapped_signal::changed::" BG_KEY_PICTURE_URI,
                    G_CALLBACK (on_background_setting_changed), self,
                    "swapped_signal::changed::" BG_KEY_PICTURE_OPTIONS,
                    G_CALLBACK (on_background_setting_changed), self,
                    NULL);

  g_signal_connect_swapped (phosh_shell_get_default (),
                            "notify::rotation",
                            G_CALLBACK (rotation_notify_cb),
                            self);

  g_signal_connect (self, "configured", G_CALLBACK (on_phosh_background_configured), self);
}


static void
phosh_background_finalize (GObject *object)
{
  GObjectClass *parent_class = G_OBJECT_CLASS (phosh_background_parent_class);
  PhoshBackground *self = PHOSH_BACKGROUND (object);

  g_object_unref (self->pixbuf);
  g_clear_pointer (&self->uri, g_free);
  g_clear_object (&self->settings);

  parent_class->finalize (object);
}


static void
phosh_background_class_init (PhoshBackgroundClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  signals[BACKGROUND_LOADED] = g_signal_new ("background-loaded",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  object_class->constructed = phosh_background_constructed;
  object_class->finalize = phosh_background_finalize;

  object_class->set_property = phosh_background_set_property;
  object_class->get_property = phosh_background_get_property;

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
}


GtkWidget *
phosh_background_new (gpointer layer_shell,
                      gpointer wl_output,
                      guint width,
                      guint height,
                      gboolean primary)
{
  return g_object_new (PHOSH_TYPE_BACKGROUND,
                       "layer-shell", layer_shell,
                       "wl-output", wl_output,
                       "width", width,
                       "height", height,
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
  if (self->uri)
    load_background (self);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRIMARY]);
}


