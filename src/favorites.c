/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Based on maynard's favorites whish is
 * Copyright (C) 2013 Collabora Ltd.
 * Author: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#define G_LOG_DOMAIN "phosh-favorites"

#include "config.h"

#include "favorites.h"
#include "phosh.h"

#include <gio/gdesktopappinfo.h>

enum {
  APP_LAUNCHED,
  SELECTION_ABORTED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

typedef struct
{
  GtkWidget *scroll;
  GtkWidget *evbox;
  GtkWidget *flowbox;
  GSettings *settings;
} PhoshFavoritesPrivate;


struct _PhoshFavorites
{
  GtkWindowClass parent;
};

G_DEFINE_TYPE_WITH_PRIVATE(PhoshFavorites, phosh_favorites, GTK_TYPE_WINDOW)


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
  if (!info)
    return NULL;

  icon = g_app_info_get_icon (G_APP_INFO (info));
  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_DIALOG);
  g_object_unref (icon);

  btn = gtk_button_new ();
  gtk_style_context_remove_class (gtk_widget_get_style_context (btn),
                                  "button");
  gtk_style_context_remove_class (gtk_widget_get_style_context (btn),
                                  "image-button");
  gtk_style_context_add_class (gtk_widget_get_style_context (btn),
                               "phosh-favorite");

  gtk_style_context_add_class (gtk_widget_get_style_context( GTK_WIDGET(btn) ),
                               "circular");

  gtk_button_set_image (GTK_BUTTON (btn), image);
  g_object_set (image, "margin", 20, NULL);

  g_object_set_data (G_OBJECT (btn), "favorites", self);
  g_signal_connect (btn, "clicked", G_CALLBACK (favorite_clicked_cb), info);

  return btn;
}


/* Add weston terminal as band aid in case all else fails for now */
static void
add_weston_terminal (PhoshFavorites *self)
{
  PhoshFavoritesPrivate *priv = phosh_favorites_get_instance_private (self);
  GIcon *icon;
  GtkWidget *image;
  GtkWidget *btn;
  char *names[] = {"utilities-terminal"};

  btn = gtk_button_new();
  icon = g_themed_icon_new_from_names (names, 1);
  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_DIALOG);
  g_object_unref (icon);

  g_object_set (image, "margin", 20, NULL);
  gtk_button_set_image (GTK_BUTTON (btn), image);

  gtk_style_context_add_class (gtk_widget_get_style_context( GTK_WIDGET(btn) ),
                               "circular");

  g_signal_connect_swapped (btn, "clicked", G_CALLBACK (term_btn_clicked), self);
  gtk_flow_box_insert (GTK_FLOW_BOX (priv->flowbox), btn, -1);
}


static void
favorites_changed (GSettings *settings,
                   const gchar *key,
                   PhoshFavorites *self)
{
  PhoshFavoritesPrivate *priv = phosh_favorites_get_instance_private (self);
  gchar **favorites = g_settings_get_strv (settings, key);
  GtkWidget *btn;

  /* Remove all favorites first */
  gtk_container_foreach (GTK_CONTAINER (priv->flowbox),
                         (GtkCallback) gtk_widget_destroy, NULL);

  for (gint i = 0; i < g_strv_length (favorites); i++) {
    gchar *fav = favorites[i];
    btn = add_favorite (self, fav);
    if (btn)
      gtk_flow_box_insert (GTK_FLOW_BOX (priv->flowbox), btn, -1);
  }
  g_strfreev (favorites);
  add_weston_terminal (self);
}


static gboolean
draw_cb (GtkWidget *widget, cairo_t *cr, gpointer unused)
{
  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.1);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  return FALSE;
}


static gboolean
evbox_button_press_event_cb (PhoshFavorites *self, GdkEventButton *event)
{
  g_signal_emit(self, signals[SELECTION_ABORTED], 0);
  return FALSE;
}


static void
phosh_favorites_constructed (GObject *object)
{
  PhoshFavorites *self = PHOSH_FAVORITES (object);
  PhoshFavoritesPrivate *priv = phosh_favorites_get_instance_private (self);
  gint width, height;

  G_OBJECT_CLASS (phosh_favorites_parent_class)->constructed (object);

  /* window properties */
  phosh_shell_get_usable_area (phosh(), NULL, NULL, &width, &height);
  gtk_window_set_title (GTK_WINDOW (self), "phosh favorites");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_window_resize (GTK_WINDOW (self), width, height);
  gtk_widget_realize(GTK_WIDGET (self));
  gtk_widget_set_app_paintable(GTK_WIDGET (self), TRUE);

  g_signal_connect (G_OBJECT(self),
                    "draw",
                    G_CALLBACK(draw_cb),
                    NULL);

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "phosh-favorites");

  /* Flowbox */
  priv->flowbox = gtk_widget_new (GTK_TYPE_FLOW_BOX,
                                    "halign", GTK_ALIGN_START,
                                    "valign", GTK_ALIGN_CENTER,
                                    "selection-mode", GTK_SELECTION_NONE,
                                    "orientation", GTK_ORIENTATION_VERTICAL,
                                    NULL);
  gtk_flow_box_set_max_children_per_line (GTK_FLOW_BOX(priv->flowbox), G_MAXINT);
  gtk_flow_box_set_homogeneous (GTK_FLOW_BOX(priv->flowbox), TRUE);

  /* Scrolled window */
  priv->scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_NEVER);
  gtk_container_add (GTK_CONTAINER (priv->scroll), priv->flowbox);

  /* Eventbox */
  priv->evbox = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (priv->evbox), priv->scroll);
  g_signal_connect_swapped (priv->evbox, "button_press_event",
                            G_CALLBACK (evbox_button_press_event_cb),
                            self);
  gtk_widget_set_events (priv->evbox, GDK_BUTTON_PRESS_MASK);

  gtk_container_add (GTK_CONTAINER (self), priv->evbox);

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
  signals[SELECTION_ABORTED] = g_signal_new ("selection-aborted",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
}


static void
phosh_favorites_init (PhoshFavorites *self)
{
}


GtkWidget *
phosh_favorites_new ()
{
  return g_object_new (PHOSH_TYPE_FAVORITES, NULL);
}
