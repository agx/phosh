/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "util.h"
#include <gtk/gtk.h>

/* Just wraps gtk_widget_destroy so we can use it with g_clear_pointer */
void
phosh_cp_widget_destroy (void *widget)
{
  gtk_widget_destroy (GTK_WIDGET (widget));
}


/* In GTK+4 we can just call gtk_image_new_from_gicon since it got rid of the
   restrictive size element */
GtkWidget *
phosh_get_image_from_gicon (GIcon *icon, int size, int scale)
{
  GtkIconTheme *theme = gtk_icon_theme_get_for_screen (gdk_screen_get_default ());
  GError *err = NULL;
  g_autoptr(GdkPixbuf) pixbuf = NULL;
  g_autoptr(GtkIconInfo) info = NULL;
  g_autofree gchar *name = NULL;

  g_return_val_if_fail (G_IS_ICON (icon), NULL);

  info = gtk_icon_theme_lookup_by_gicon_for_scale(theme, icon, size, scale, 0);
  if (!info) {
    g_warning ("Failed to lookup icon for %s", g_icon_to_string (icon));
    return NULL;
  }
  pixbuf = gtk_icon_info_load_icon (info, &err);
  if (!pixbuf) {
    g_warning ("Failed to load icon for %s: %s", g_icon_to_string (icon), err->message);
    return NULL;
  }
  return gtk_image_new_from_pixbuf (pixbuf);
}
