/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-background"

#include "background.h"
#include "shell.h"
#include "panel.h"

#include <math.h>
#include <string.h>

enum {
  BACKGROUND_LOADED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];

struct _PhoshBackground
{
  PhoshLayerSurface parent;

  GdkPixbuf *pixbuf;
  GSettings *settings;
};


G_DEFINE_TYPE (PhoshBackground, phosh_background, PHOSH_TYPE_LAYER_SURFACE)


static GdkPixbuf *
image_background (GdkPixbuf *image, guint width, guint height)
{
  gint orig_width, orig_height;
  gint final_width, final_height;
  gint off_x, off_y;
  gdouble ratio_horiz, ratio_vert, ratio;
  GdkPixbuf *bg, *scaled_bg;
  const gchar *xpm_data[] = {"1 1 1 1", "_ c WebGrey", "_"};

  bg = gdk_pixbuf_new_from_xpm_data (xpm_data);
  scaled_bg = gdk_pixbuf_scale_simple (bg,
                                       width,
                                       height,
                                       GDK_INTERP_BILINEAR);
  g_object_unref (bg);

  /* FIXME: use libgnome-desktop's background handling instead */
  orig_width = gdk_pixbuf_get_width (image);
  orig_height = gdk_pixbuf_get_height (image);
  ratio_horiz = (double) width / orig_width;
  ratio_vert = (double) height / orig_height;
  ratio = ratio_horiz > ratio_vert ? ratio_vert : ratio_horiz;
  final_width = ceil (ratio * orig_width);
  final_height = ceil (ratio * orig_height);

  off_x = (width - final_width) / 2;
  off_y = (height - final_height) / 2;

  gdk_pixbuf_composite (image,
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


static void
load_background (PhoshBackground *self,
                 const char* uri)
{
  g_autoptr(GdkPixbuf) image = NULL;
  const gchar *xpm_data[] = {"1 1 1 1", "_ c WebGrey", "_"};
  GError *err = NULL;
  gint width, height;

  if (self->pixbuf) {
    g_object_unref (self->pixbuf);
    self->pixbuf = NULL;
  }

  /* FIXME: support GnomeDesktop.BGSlideShow as well */
  if (!g_str_has_prefix(uri, "file:///")) {
    g_warning ("Only file URIs supported for backgrounds not %s", uri);
  } else {
    image = gdk_pixbuf_new_from_file (&uri[strlen("file://")], &err);
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

  phosh_shell_get_usable_area (phosh_shell_get_default (), NULL, NULL, &width, &height);
  self->pixbuf = image_background (image, width, height);

  /* force background redraw */
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_signal_emit(self, signals[BACKGROUND_LOADED], 0);
}


static gboolean
background_draw_cb (PhoshBackground *self,
                    cairo_t         *cr,
                    gpointer         data)
{
  gint x, y, width, height;

  g_return_val_if_fail (PHOSH_IS_BACKGROUND (self), TRUE);

  phosh_shell_get_usable_area (phosh_shell_get_default (), &x, &y, &width, &height);
  gdk_cairo_set_source_pixbuf (cr, self->pixbuf, x, y);
  cairo_paint (cr);
  return TRUE;
}


static void
background_setting_changed_cb (PhoshBackground *self,
                               const gchar     *key,
                               GSettings       *settings)
{
  g_autofree gchar *uri = g_settings_get_string (settings, key);

  g_return_if_fail (PHOSH_IS_BACKGROUND (self));
  g_return_if_fail (G_IS_SETTINGS (settings));

  if (!uri)
    return;

  load_background (self, uri);
}


static void
rotation_notify_cb (PhoshBackground *self,
                    GParamSpec *pspec,
                    PhoshShell *shell)
{
  g_return_if_fail (PHOSH_IS_BACKGROUND (self));
  g_return_if_fail (PHOSH_IS_SHELL (shell));

  background_setting_changed_cb (self, "picture-uri", self->settings);
}


static void
on_phosh_background_configured (PhoshLayerSurface *surface)
{
  PhoshBackground *self = PHOSH_BACKGROUND (surface);

  /* Load background initially */
  background_setting_changed_cb (self,  "picture-uri", self->settings);
}


static void
phosh_background_constructed (GObject *object)
{
  PhoshBackground *self = PHOSH_BACKGROUND (object);

  G_OBJECT_CLASS (phosh_background_parent_class)->constructed (object);

  g_signal_connect (self, "draw", G_CALLBACK (background_draw_cb), NULL);

  self->settings = g_settings_new ("org.gnome.desktop.background");
  g_signal_connect_swapped (self->settings, "changed::picture-uri",
                            G_CALLBACK (background_setting_changed_cb), self);

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
}


static void
phosh_background_init (PhoshBackground *self)
{
}


GtkWidget *
phosh_background_new (gpointer layer_shell,
                      gpointer wl_output,
                      guint width,
                      guint height)
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
                       NULL);
}
