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
#include "app.h"
#include "shell.h"
#include "util.h"
#include "phosh-private-client-protocol.h"
#include "phosh-wayland.h"

#include <gio/gdesktopappinfo.h>

#define FAVORITES_ICON_SIZE 64

enum {
  APP_LAUNCHED,
  APP_RAISED,
  APP_CLOSED,
  SELECTION_ABORTED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

typedef struct
{
  /* Favorites */
  GtkWidget *evbox_favorites;
  GtkWidget *fb_favorites;
  GSettings *settings;

  /* Running apps */
  GtkWidget *evbox_running_apps;
  GtkWidget *box_running_apps;
  struct phosh_private_xdg_switcher *xdg_switcher;

} PhoshFavoritesPrivate;


struct _PhoshFavorites
{
  GtkWindowClass parent;
};

G_DEFINE_TYPE_WITH_PRIVATE(PhoshFavorites, phosh_favorites, GTK_TYPE_WINDOW)


static void
app_clicked_cb (GtkButton *btn, gpointer user_data)
{
  PhoshFavorites *self = PHOSH_FAVORITES (user_data);
  PhoshFavoritesPrivate *priv;
  PhoshApp *app = PHOSH_APP (btn);

  g_return_if_fail (PHOSH_IS_FAVORITES (self));
  priv = phosh_favorites_get_instance_private (self);
  g_return_if_fail (priv->xdg_switcher);

  g_debug("Will raise %s (%s)",
          phosh_app_get_app_id (app),
          phosh_app_get_title (app));

  phosh_private_xdg_switcher_raise_xdg_surface (priv->xdg_switcher,
                                                phosh_app_get_app_id (app),
                                                phosh_app_get_title (app));
  g_signal_emit(self, signals[APP_RAISED], 0);
}


static void
on_app_closed (PhoshFavorites *self, PhoshApp *app)
{
  PhoshFavoritesPrivate *priv;

  g_return_if_fail (PHOSH_IS_FAVORITES (self));
  g_return_if_fail (PHOSH_IS_APP (app));

  priv = phosh_favorites_get_instance_private (self);
  g_return_if_fail (priv->xdg_switcher);

  g_debug("Will close %s (%s)",
          phosh_app_get_app_id (app),
          phosh_app_get_title (app));
  phosh_private_xdg_switcher_close_xdg_surface (priv->xdg_switcher,
                                                phosh_app_get_app_id (app),
                                                phosh_app_get_title (app));
  gtk_widget_destroy (GTK_WIDGET (app));
  g_signal_emit(self, signals[APP_CLOSED], 0);
}


static void
handle_xdg_switcher_xdg_surface (
  void *data,
  struct phosh_private_xdg_switcher *phosh_private_xdg_switcher,
  const char *app_id,
  const char *title)
{
  PhoshMonitor *monitor = phosh_shell_get_primary_monitor (phosh_shell_get_default());
  PhoshFavorites *self = data;
  PhoshFavoritesPrivate *priv;
  GtkWidget *app;
  int max_width = 0;

  g_return_if_fail (PHOSH_IS_FAVORITES (self));
  priv = phosh_favorites_get_instance_private (self);

  max_width = ((gdouble) monitor->width / (gdouble) monitor->scale) * 0.75;
  max_width = MIN (max_width, 450);

  g_debug ("Building activator for '%s' (%s)", app_id, title);
  app = phosh_app_new (app_id, title);
  g_object_set (app,
                "win-width", 360,  // TODO: Get the real size somehow
                "win-height", 640,
                "max-height", 445,
                "max-width", max_width,
                NULL);
  gtk_box_pack_end (GTK_BOX (priv->box_running_apps), app, FALSE, FALSE, 0);
  gtk_widget_show (app);

  g_signal_connect (app, "clicked", G_CALLBACK (app_clicked_cb), self);
  g_signal_connect_swapped (app, "app-closed", G_CALLBACK (on_app_closed), self);

  gtk_widget_show (GTK_WIDGET (self));
}


static void
handle_xdg_switcher_list_xdg_surfaces_done(
  void *data,
  struct phosh_private_xdg_switcher *phosh_private_xdg_switcher)
{
  g_debug ("Got all apps");
}


static const struct phosh_private_xdg_switcher_listener xdg_switcher_listener = {
  handle_xdg_switcher_xdg_surface,
  handle_xdg_switcher_list_xdg_surfaces_done,
};


static void
get_running_apps (PhoshFavorites *self)
{
  PhoshFavoritesPrivate *priv = phosh_favorites_get_instance_private (self);
  struct phosh_private *phosh_private;

  phosh_private = phosh_wayland_get_phosh_private (
    phosh_wayland_get_default ());

  if (!phosh_private) {
    g_debug ("Skipping app list due to missing phosh_private protocol extension");
    return;
  }

  priv->xdg_switcher = phosh_private_get_xdg_switcher (phosh_private);
  phosh_private_xdg_switcher_add_listener (priv->xdg_switcher, &xdg_switcher_listener, self);
  phosh_private_xdg_switcher_list_xdg_surfaces (priv->xdg_switcher);
}

static void
set_max_height (GtkWidget *widget,
                gpointer   user_data)
{
  int height = GPOINTER_TO_INT (user_data);

  if (!PHOSH_IS_APP (widget)) {
    return;
  }

  g_object_set (widget,
                "max-height", height,
                NULL);
}

static void
running_apps_resized (GtkWidget     *widget,
                      GtkAllocation *alloc,
                      gpointer       data)
{
  PhoshFavorites *self = PHOSH_FAVORITES (data);
  PhoshFavoritesPrivate *priv = phosh_favorites_get_instance_private (self);
  int height = alloc->height * 0.8;

  gtk_container_foreach (GTK_CONTAINER (priv->box_running_apps),
                         set_max_height,
                         GINT_TO_POINTER (height));
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
              const gchar *favorite,
              gint scale)
{
  g_autoptr (GIcon) icon = NULL;
  GDesktopAppInfo *info;
  GtkImage *image;
  GtkWidget *btn;

  info = g_desktop_app_info_new (favorite);
  if (!info)
    return NULL;

  icon = g_app_info_get_icon (G_APP_INFO (info));

  image = GTK_IMAGE (gtk_image_new ());
  /* Setting pixel size makes gtk ignore the size argument
   * gtk_image_set_from_gicon () */
  gtk_image_set_pixel_size (image, FAVORITES_ICON_SIZE);
  gtk_image_set_from_gicon (image, icon, -1);

  btn = gtk_button_new ();
  gtk_style_context_remove_class (gtk_widget_get_style_context (btn),
                                  "button");
  gtk_style_context_remove_class (gtk_widget_get_style_context (btn),
                                  "image-button");
  gtk_style_context_add_class (gtk_widget_get_style_context (btn),
                               "phosh-favorite");

  gtk_button_set_image (GTK_BUTTON (btn), GTK_WIDGET (image));
  g_object_set (image, "margin", 10, NULL);

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
  gchar **favorites = g_settings_get_strv (settings, key);
  GtkWidget *btn;

  /* Remove all favorites first */
  gtk_container_foreach (GTK_CONTAINER (priv->fb_favorites),
                         (GtkCallback) gtk_widget_destroy, NULL);

  for (gint i = 0; i < g_strv_length (favorites); i++) {
    gchar *fav = favorites[i];
    btn = add_favorite (self, fav, 1);
    if (btn)
      gtk_flow_box_insert (GTK_FLOW_BOX (priv->fb_favorites), btn, -1);
  }
  g_strfreev (favorites);
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
  phosh_shell_get_usable_area (phosh_shell_get_default (), NULL, NULL, &width, &height);
  gtk_window_set_title (GTK_WINDOW (self), "phosh favorites");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_window_resize (GTK_WINDOW (self), width, height);
  gtk_widget_realize(GTK_WIDGET (self));

  /* Close on click */
  g_signal_connect_swapped (priv->evbox_favorites, "button_press_event",
                            G_CALLBACK (evbox_button_press_event_cb),
                            self);
  gtk_widget_set_events (priv->evbox_favorites, GDK_BUTTON_PRESS_MASK);

  priv->settings = g_settings_new ("sm.puri.phosh");
  g_signal_connect (priv->settings, "changed::favorites",
                    G_CALLBACK (favorites_changed), self);
  favorites_changed (priv->settings, "favorites", self);

  /* Close on click */
  g_signal_connect_swapped (priv->evbox_running_apps, "button_press_event",
                            G_CALLBACK (evbox_button_press_event_cb),
                            self);
  gtk_widget_set_events (priv->evbox_running_apps, GDK_BUTTON_PRESS_MASK);
  get_running_apps (self);

  g_signal_connect (priv->evbox_running_apps,
                    "size-allocate",
                    G_CALLBACK (running_apps_resized),
                    self);
}


static void
phosh_favorites_dispose (GObject *object)
{
  PhoshFavorites *self = PHOSH_FAVORITES (object);
  PhoshFavoritesPrivate *priv = phosh_favorites_get_instance_private (self);

  g_clear_object (&priv->settings);

  if (priv->xdg_switcher) {
    phosh_private_xdg_switcher_destroy (priv->xdg_switcher);
    priv->xdg_switcher = NULL;
  }

  G_OBJECT_CLASS (phosh_favorites_parent_class)->dispose (object);
}


static void
phosh_favorites_class_init (PhoshFavoritesClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = phosh_favorites_dispose;
  object_class->constructed = phosh_favorites_constructed;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/favorites.ui");

  gtk_widget_class_bind_template_child_private (widget_class, PhoshFavorites, evbox_favorites);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshFavorites, fb_favorites);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshFavorites, evbox_running_apps);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshFavorites, box_running_apps);

  signals[APP_LAUNCHED] = g_signal_new ("app-launched",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
  signals[APP_RAISED] = g_signal_new ("app-raised",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
  signals[SELECTION_ABORTED] = g_signal_new ("selection-aborted",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
  signals[APP_CLOSED] = g_signal_new ("app-closed",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
}


static void
phosh_favorites_init (PhoshFavorites *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_favorites_new (void)
{
  return g_object_new (PHOSH_TYPE_FAVORITES, NULL);
}
