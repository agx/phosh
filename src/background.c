/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh.h"
#include "background.h"

#include <math.h>

typedef struct
{
  GdkPixbuf *pixbuf;
} PhoshBackgroundPrivate;


typedef struct _PhoshBackground
{
  GtkWindow parent;
} PhoshBackground;


G_DEFINE_TYPE_WITH_PRIVATE (PhoshBackground, phosh_background, GTK_TYPE_WINDOW)


static GdkPixbuf *
scale_background (GdkPixbuf *original_pixbuf)
{
  GdkDisplay *display = gdk_display_get_default ();
  /* There's no primary monitor on nested wayland so just use the
     first one for now */
  GdkMonitor *monitor = gdk_display_get_monitor (display, 0);
  GdkRectangle geom;
  gint original_width, original_height;
  gint final_width, final_height;
  gdouble ratio_horizontal, ratio_vertical;

  /* FIXME: handle org.gnome.desktop.background gsettings */
  g_return_val_if_fail(monitor, NULL);

  gdk_monitor_get_geometry (monitor, &geom);

  original_width = gdk_pixbuf_get_width (original_pixbuf);
  original_height = gdk_pixbuf_get_height (original_pixbuf);

  ratio_horizontal = (double) geom.width / original_width;
  ratio_vertical = (double) geom.height / original_height;

  final_width = ceil (ratio_horizontal * original_width);
  final_height = ceil (ratio_vertical * original_height);

  return gdk_pixbuf_scale_simple (original_pixbuf,
      final_width, final_height, GDK_INTERP_BILINEAR);
}


static gboolean
background_draw_cb (PhoshBackground *self,
		    cairo_t         *cr,
		    gpointer         data)
{
  PhoshBackgroundPrivate *priv = phosh_background_get_instance_private (self);

  gdk_cairo_set_source_pixbuf (cr, priv->pixbuf, 0, 0);
  cairo_paint (cr);
  return TRUE;
}


static void
background_destroy_cb (GObject *object,
		       gpointer data)
{
  gtk_main_quit ();
}


static void
phosh_background_constructed (GObject *object)
{
  PhoshBackground *self = PHOSH_BACKGROUND (object);
  PhoshBackgroundPrivate *priv = phosh_background_get_instance_private (self);

  GdkPixbuf *unscaled_background;
  const gchar *xpm_data[] = {"1 1 1 1", "_ c WebGrey", "_"};

  unscaled_background = gdk_pixbuf_new_from_xpm_data (xpm_data);
  priv->pixbuf = scale_background (unscaled_background);
  g_object_unref (unscaled_background);

  g_signal_connect (self, "destroy", G_CALLBACK (background_destroy_cb), NULL);
  g_signal_connect (self, "draw", G_CALLBACK (background_draw_cb), NULL);

  /* Window properties */
  gtk_window_set_title (GTK_WINDOW (self), "phosh background");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_widget_realize (GTK_WIDGET (self));

  G_OBJECT_CLASS (phosh_background_parent_class)->constructed (object);
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

  object_class->constructed = phosh_background_constructed;
  object_class->finalize = phosh_background_finalize;
}


static void
phosh_background_init (PhoshBackground *self)
{
}


GtkWidget *
phosh_background_new (void)
{
  return g_object_new (PHOSH_TYPE_BACKGROUND, NULL);
}
