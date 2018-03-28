/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Based on maynard whish is
 * Copyright (C) 2013 Collabora Ltd.
 * Author: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>

 */

#include "config.h"

#include "favorites.h"

#include <gio/gdesktopappinfo.h>

enum {
  APP_LAUNCHED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

typedef struct
{
  GtkWidget *grid;
  GSettings *settings;
} PhoshFavoritesPrivate;


struct _PhoshFavorites
{
  PhoshMenuClass parent;
};

G_DEFINE_TYPE_WITH_PRIVATE(PhoshFavorites, phosh_favorites, PHOSH_TYPE_MENU) 


static void
term_btn_clicked (PhoshFavorites *self,
                  GtkButton *btn)
{
  GError *error = NULL;
  g_spawn_command_line_async ("weston-terminal", &error);
  if (error)
    g_warning ("Could not launch terminal");

  g_signal_emit(self, signals[APP_LAUNCHED], 0);
}


static void
favorite_clicked_cb (GtkWidget *widget,
                     GDesktopAppInfo *info)
{
  PhoshFavorites *self;

  g_app_info_launch (G_APP_INFO (info), NULL, NULL, NULL);

  self = g_object_get_data (G_OBJECT (widget), "favorites");
  g_assert (self);
  g_signal_emit (self, signals[APP_LAUNCHED], 0);
}


static GtkWidget*
add_favorite (PhoshFavorites *self,
              const gchar *favorite)
{
  GIcon *icon;
  GDesktopAppInfo *info;
  GtkWidget *image;
  GtkWidget *btn;

  info = g_desktop_app_info_new (favorite);

  icon = g_app_info_get_icon (G_APP_INFO (info));
  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_DIALOG);
  btn = gtk_button_new ();
  gtk_style_context_remove_class (gtk_widget_get_style_context (btn),
                                  "button");
  gtk_style_context_remove_class (gtk_widget_get_style_context (btn),
                                  "image-button");
  gtk_style_context_add_class (gtk_widget_get_style_context (btn),
                               "phosh-favorite");

  gtk_button_set_image (GTK_BUTTON (btn), image);
  g_object_set (image, "margin", 20, NULL);

  g_object_set_data (G_OBJECT (btn), "favorites", self);
  g_signal_connect (btn, "clicked", G_CALLBACK (favorite_clicked_cb), info);

  return btn;
}


static void
favorites_changed (GSettings *settings,
                   const gchar *key,
                   PhoshFavorites *self)
{
  PhoshFavoritesPrivate *priv = phosh_favorites_get_instance_private (self);
  GtkWidget *btn;
  gchar **favorites = g_settings_get_strv (settings, key);
  guint top = 1;

  /* Remove all favorites first */
  gtk_container_foreach (GTK_CONTAINER (priv->grid),
                         (GtkCallback) gtk_widget_destroy, NULL);

  /* Add weston terminal as band aid in case all else fails for now */
  btn = gtk_button_new_from_icon_name ("terminal", GTK_ICON_SIZE_DIALOG);
  gtk_grid_attach (GTK_GRID (priv->grid), btn, 1, 1, 1, 1);
  g_signal_connect_swapped (btn, "clicked", G_CALLBACK (term_btn_clicked), self);
  top++;

  for (gint i = 0; i < g_strv_length (favorites); i++) {
    gchar *fav = favorites[i];
    btn = add_favorite (self, fav);
    gtk_grid_attach (GTK_GRID (priv->grid), btn, 1, top++, 1, 1);
  }
  g_strfreev (favorites);
}


static void
phosh_favorites_constructed (GObject *object)
{
  PhoshFavorites *self = PHOSH_FAVORITES (object);
  PhoshFavoritesPrivate *priv = phosh_favorites_get_instance_private (self);

  G_OBJECT_CLASS (phosh_favorites_parent_class)->constructed (object);

  /* window properties */
  gtk_window_set_title (GTK_WINDOW (self), "phosh favorites");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_window_resize (GTK_WINDOW (self), 100, 100);
  gtk_widget_realize(GTK_WIDGET (self));

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "phosh-favorites");

  priv->grid = gtk_widget_new (GTK_TYPE_GRID, "halign",
                               GTK_ALIGN_CENTER, "valign",
                               GTK_ALIGN_CENTER, NULL);
  gtk_container_add (GTK_CONTAINER (self), priv->grid);

  priv->settings = g_settings_new ("sm.puri.phosh");
  g_signal_connect (priv->settings, "changed::favorites",
                    G_CALLBACK (favorites_changed), self);
  favorites_changed (priv->settings, "favorites", self);
}


static void
phosh_favorites_dispose (GObject *object)
{
  PhoshFavorites *self = PHOSH_FAVORITES (object);
  PhoshFavoritesPrivate *priv = phosh_favorites_get_instance_private (self);

  g_clear_object (&priv->settings);

  G_OBJECT_CLASS (phosh_favorites_parent_class)->dispose (object);
}


static void
phosh_favorites_class_init (PhoshFavoritesClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->dispose = phosh_favorites_dispose;
  object_class->constructed = phosh_favorites_constructed;

  signals[APP_LAUNCHED] = g_signal_new ("app-launched",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
}


static void
phosh_favorites_init (PhoshFavorites *self)
{
}


GtkWidget *
phosh_favorites_new (int position, const gpointer *shell)
{
  return g_object_new (PHOSH_TYPE_FAVORITES,
                       "name", "favorites",
                       "shell", shell,
                       "position", position,
                       NULL);
}
