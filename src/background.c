/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "background.h"
#include "phosh.h"
#include "panel.h"

#include <math.h>
#include <string.h>

enum {
  BACKGROUND_LOADED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];

typedef struct
{
  GdkPixbuf *pixbuf;
  GSettings *settings;
} PhoshBackgroundPrivate;


typedef struct _PhoshBackground
{
  GtkWindow parent;
} PhoshBackground;


G_DEFINE_TYPE_WITH_PRIVATE (PhoshBackground, phosh_background, GTK_TYPE_WINDOW)


static GdkPixbuf *
image_background (GdkPixbuf *image)
{
  gint orig_width, orig_height, width, height;
  gint final_width, final_height;
  gint x, y, off_x, off_y;
  gdouble ratio_horiz, ratio_vert, ratio;
  GdkPixbuf *bg, *scaled_bg;
  const gchar *xpm_data[] = {"1 1 1 1", "_ c WebGrey", "_"};

  phosh_shell_get_usable_area (phosh(), &x, &y, &width, &height);
  bg = gdk_pixbuf_new_from_xpm_data (xpm_data);
  scaled_bg = gdk_pixbuf_scale_simple (bg,
                                       width,
                                       /* since we can't offset the pixmap */
                                       height + y,
                                       GDK_INTERP_BILINEAR);
  g_object_unref (bg);

  /* FIXME: we should allow more modes
     none, wallpaper, centered, scaled, stretched, zoom
     I think GNOME calls this zoom:
  */
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
  PhoshBackgroundPrivate *priv = phosh_background_get_instance_private (self);
  GdkPixbuf *image = NULL;
  const gchar *xpm_data[] = {"1 1 1 1", "_ c WebGrey", "_"};
  GError *err = NULL;

  if (priv->pixbuf) {
    g_object_unref (priv->pixbuf);
    priv->pixbuf = NULL;
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

  priv->pixbuf = image_background (image);
  g_object_unref (image);

  /* force background redraw */
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_signal_emit(self, signals[BACKGROUND_LOADED], 0);
}


static gboolean
background_draw_cb (PhoshBackground *self,
                    cairo_t         *cr,
                    gpointer         data)
{
  PhoshBackgroundPrivate *priv = phosh_background_get_instance_private (self);
  gint x, y, width, height;

  phosh_shell_get_usable_area (phosh(), &x, &y, &width, &height);
  gdk_cairo_set_source_pixbuf (cr, priv->pixbuf, x, y);
  cairo_paint (cr);
  return TRUE;
}


static void
background_destroy_cb (GObject *object,
                       gpointer data)
{
  /* FIXME: does this make any sense ? */
  gtk_main_quit ();
}


static void
background_setting_changed_cb (GSettings       *settings,
                               const gchar     *key,
                               PhoshBackground *self)
{
  g_autofree gchar *uri = g_settings_get_string (settings, key);

  if (!uri)
    return;

  load_background (self, uri);
}


static void
rotation_notify_cb (PhoshBackground *self,
                    PhoshShell *shell)
{
  PhoshBackgroundPrivate *priv = phosh_background_get_instance_private (self);
  background_setting_changed_cb (priv->settings, "picture-uri", self);
}


static void
phosh_background_constructed (GObject *object)
{
  PhoshBackground *self = PHOSH_BACKGROUND (object);
  PhoshBackgroundPrivate *priv = phosh_background_get_instance_private (self);

  G_OBJECT_CLASS (phosh_background_parent_class)->constructed (object);

  g_signal_connect (self, "destroy", G_CALLBACK (background_destroy_cb), NULL);
  g_signal_connect (self, "draw", G_CALLBACK (background_draw_cb), NULL);

  priv->settings = g_settings_new ("org.gnome.desktop.background");
  g_signal_connect (priv->settings, "changed::picture-uri",
                    G_CALLBACK (background_setting_changed_cb), self);
  /* Load background initially */
  background_setting_changed_cb (priv->settings, "picture-uri", self);

  /* Window properties */
  gtk_window_set_title (GTK_WINDOW (self), "phosh background");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_widget_realize (GTK_WIDGET (self));

  g_signal_connect_swapped (phosh(),
                            "notify::rotation",
                            G_CALLBACK (rotation_notify_cb),
                            self);
}


static void
phosh_background_finalize (GObject *object)
{
  PhoshBackgroundPrivate *priv = phosh_background_get_instance_private (PHOSH_BACKGROUND(object));
  GObjectClass *parent_class = G_OBJECT_CLASS (phosh_background_parent_class);

  g_object_unref (priv->pixbuf);

  if (parent_class->finalize != NULL)
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
  PhoshBackgroundPrivate *priv = phosh_background_get_instance_private (PHOSH_BACKGROUND(self));

  priv->pixbuf = NULL;
  priv->settings = NULL;
}


GtkWidget *
phosh_background_new (void)
{
  return g_object_new (PHOSH_TYPE_BACKGROUND, NULL);
}
