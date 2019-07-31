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
#include "activity.h"
#include "shell.h"
#include "util.h"
#include "toplevel-manager.h"
#include "phosh-private-client-protocol.h"
#include "phosh-wayland.h"

#include <gio/gdesktopappinfo.h>

#define FAVORITES_ICON_SIZE 64

enum {
  ACTIVITY_LAUNCHED,
  ACTIVITY_RAISED,
  ACTIVITY_CLOSED,
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

  /* Running activities */
  GtkWidget *evbox_running_activities;
  GtkWidget *box_running_activities;

} PhoshFavoritesPrivate;


struct _PhoshFavorites
{
  GtkWindowClass parent;
};

G_DEFINE_TYPE_WITH_PRIVATE(PhoshFavorites, phosh_favorites, GTK_TYPE_WINDOW)


static void
on_activity_clicked (PhoshFavorites *self, PhoshActivity *activity)
{
  PhoshToplevel *toplevel;
  g_return_if_fail (PHOSH_IS_FAVORITES (self));
  g_return_if_fail (PHOSH_IS_ACTIVITY (activity));

  toplevel = g_object_get_data(G_OBJECT (activity), "toplevel");
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));

  g_debug("Will raise %s (%s)",
          phosh_activity_get_app_id (activity),
          phosh_activity_get_title (activity));

  phosh_toplevel_raise (toplevel, phosh_wayland_get_wl_seat (phosh_wayland_get_default ()));
  g_signal_emit(self, signals[ACTIVITY_RAISED], 0);
}


static void
on_activity_closed (PhoshFavorites *self, PhoshActivity *activity)
{
  PhoshToplevel *toplevel;
  g_return_if_fail (PHOSH_IS_FAVORITES (self));
  g_return_if_fail (PHOSH_IS_ACTIVITY (activity));

  toplevel = g_object_get_data(G_OBJECT (activity), "toplevel");
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));

  g_debug("Will close %s (%s)",
          phosh_activity_get_app_id (activity),
          phosh_activity_get_title (activity));

  phosh_toplevel_close (toplevel);
  g_signal_emit(self, signals[ACTIVITY_CLOSED], 0);
}

static void
on_toplevel_closed (PhoshToplevel *toplevel, PhoshActivity *activity)
{
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));
  g_return_if_fail (PHOSH_IS_ACTIVITY (activity));
  gtk_widget_destroy (GTK_WIDGET (activity));
}

static void add_activity (PhoshFavorites *self, PhoshToplevel *toplevel)
{
  PhoshMonitor *monitor = phosh_shell_get_primary_monitor (phosh_shell_get_default ());
  PhoshFavoritesPrivate *priv;
  GtkWidget *activity;
  int max_width = 0;
  const gchar *app_id, *title;

  g_return_if_fail (PHOSH_IS_FAVORITES (self));
  priv = phosh_favorites_get_instance_private (self);

  max_width = ((gdouble) monitor->width / (gdouble) monitor->scale) * 0.75;
  max_width = MIN (max_width, 450);

  app_id = phosh_toplevel_get_app_id (toplevel);
  title = phosh_toplevel_get_title (toplevel);

  g_debug ("Building activator for '%s' (%s)", app_id, title);
  activity = phosh_activity_new (app_id, title);
  g_object_set (activity,
                "win-width", 360,  // TODO: Get the real size somehow
                "win-height", 640,
                "max-height", 445,
                "max-width", max_width,
                NULL);
  g_object_set_data (G_OBJECT(activity), "toplevel", toplevel);
  gtk_box_pack_end (GTK_BOX (priv->box_running_activities), activity, FALSE, FALSE, 0);
  gtk_widget_show (activity);

  g_signal_connect_swapped (activity, "clicked", G_CALLBACK (on_activity_clicked), self);
  g_signal_connect_swapped (activity, "activity-closed", G_CALLBACK (on_activity_closed), self);

  g_signal_connect_object (toplevel, "closed", G_CALLBACK (on_toplevel_closed), activity, 0);
}

static void
get_running_activities (PhoshFavorites *self)
{
  PhoshToplevelManager *toplevel_manager = phosh_shell_get_toplevel_manager (phosh_shell_get_default ());
  guint toplevels_num = phosh_toplevel_manager_get_num_toplevels (toplevel_manager);

  for (guint i = 0; i < toplevels_num; i++) {
    PhoshToplevel *toplevel = phosh_toplevel_manager_get_toplevel (toplevel_manager, i);
    add_activity (self, toplevel);
  }
}

static void
toplevel_added_cb (PhoshFavorites *self, PhoshToplevel *toplevel, PhoshToplevelManager *manager)
{
  g_return_if_fail (PHOSH_IS_FAVORITES (self));
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));
  g_return_if_fail (PHOSH_IS_TOPLEVEL_MANAGER (manager));
  add_activity (self, toplevel);
}


static void
set_max_height (GtkWidget *widget,
                gpointer   user_data)
{
  int height = GPOINTER_TO_INT (user_data);

  if (!PHOSH_IS_ACTIVITY (widget)) {
    return;
  }

  g_object_set (widget,
                "max-height", height,
                NULL);
}

static void
running_activities_resized (GtkWidget     *widget,
                      GtkAllocation *alloc,
                      gpointer       data)
{
  PhoshFavorites *self = PHOSH_FAVORITES (data);
  PhoshFavoritesPrivate *priv = phosh_favorites_get_instance_private (self);
  int height = alloc->height * 0.8;

  gtk_container_foreach (GTK_CONTAINER (priv->box_running_activities),
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
  g_signal_emit (self, signals[ACTIVITY_LAUNCHED], 0);
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
  PhoshToplevelManager *toplevel_manager =
      phosh_shell_get_toplevel_manager (phosh_shell_get_default ());
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
  g_signal_connect_swapped (priv->evbox_running_activities, "button_press_event",
                            G_CALLBACK (evbox_button_press_event_cb),
                            self);
  gtk_widget_set_events (priv->evbox_running_activities, GDK_BUTTON_PRESS_MASK);
  get_running_activities (self);

  g_signal_connect_object (toplevel_manager, "toplevel-added",
                           G_CALLBACK (toplevel_added_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect (priv->evbox_running_activities,
                    "size-allocate",
                    G_CALLBACK (running_activities_resized),
                    self);
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
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = phosh_favorites_dispose;
  object_class->constructed = phosh_favorites_constructed;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/favorites.ui");

  gtk_widget_class_bind_template_child_private (widget_class, PhoshFavorites, evbox_favorites);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshFavorites, fb_favorites);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshFavorites, evbox_running_activities);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshFavorites, box_running_activities);

  signals[ACTIVITY_LAUNCHED] = g_signal_new ("activity-launched",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
  signals[ACTIVITY_RAISED] = g_signal_new ("activity-raised",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
  signals[SELECTION_ABORTED] = g_signal_new ("selection-aborted",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
  signals[ACTIVITY_CLOSED] = g_signal_new ("activity-closed",
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
