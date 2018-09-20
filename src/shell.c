/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Based on maynard's panel which is
 * Copyright (C) 2014 Collabora Ltd. *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#define G_LOG_DOMAIN "phosh-shell"

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>
#include <glib-unix.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkwayland.h>

#include "config.h"
#include "shell.h"

#include "phosh-wayland.h"
#include "monitor/monitor.h"
#include "background.h"
#include "lockscreen-manager.h"
#include "monitor-manager.h"
#include "panel.h"
#include "home.h"
#include "favorites.h"
#include "settings.h"
#include "session.h"
#include "system-prompter.h"
#include "util.h"


enum {
  PHOSH_SHELL_PROP_0,
  PHOSH_SHELL_PROP_ROTATION,
  PHOSH_SHELL_PROP_LOCKED,
  PHOSH_SHELL_PROP_LAST_PROP
};
static GParamSpec *props[PHOSH_SHELL_PROP_LAST_PROP];


struct popup {
  GtkWidget *window;
  struct wl_surface *wl_surface;
  struct xdg_popup *popup;
};

typedef struct
{
  gint rotation;

  PhoshLayerSurface *panel;
  PhoshLayerSurface *home;
  PhoshLayerSurface *background;
  struct popup *favorites;
  struct popup *settings;

  PhoshMonitorManager *monitor_manager;
  PhoshLockscreenManager *lockscreen_manager;
} PhoshShellPrivate;


typedef struct _PhoshShell
{
  GObject parent;
} PhoshShell;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshShell, phosh_shell, G_TYPE_OBJECT)


static struct popup**
get_popup_from_xdg_popup (PhoshShell *self, struct xdg_popup *xdg_popup)
{
  PhoshShellPrivate *priv;
  struct popup **popup = NULL;

  g_return_val_if_fail (PHOSH_IS_SHELL (self), NULL);

  priv = phosh_shell_get_instance_private (self);
  if (priv->favorites && xdg_popup == priv->favorites->popup) {
    popup = &priv->favorites;
  } else if (priv->settings && xdg_popup == priv->settings->popup) {
    popup = &priv->settings;
  }
  g_return_val_if_fail (popup, NULL);
  return popup;
}


static void
close_menu (struct popup **popup)
{
  if (*popup == NULL)
    return;

  gtk_window_close (GTK_WINDOW ((*popup)->window));
  gtk_widget_destroy (GTK_WIDGET ((*popup)->window));
  free (*popup);
  *popup = NULL;
}


static void
close_favorites_menu_cb (PhoshShell *self,
                         PhoshFavorites *favorites)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  g_return_if_fail (priv->favorites);
  close_menu (&priv->favorites);
}


static void
xdg_surface_handle_configure(void *data,
                             struct xdg_surface *xdg_surface,
                             uint32_t serial)
{
  xdg_surface_ack_configure(xdg_surface, serial);
  // Whatever
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_handle_configure,
};


static void
xdg_popup_configure(void *data, struct xdg_popup *xdg_popup,
                    int32_t x, int32_t y, int32_t w, int32_t h)
{
  PhoshShell *self = data;
  struct popup *popup = *get_popup_from_xdg_popup(self, xdg_popup);

  g_return_if_fail (popup);
  g_debug("Popup configured %dx%d@%d,%d\n", w, h, x, y);
  gtk_window_resize (GTK_WINDOW (popup->window), w, h);
  gtk_widget_show_all (popup->window);
}

static void xdg_popup_done(void *data, struct xdg_popup *xdg_popup) {
  PhoshShell *self = data;
  struct popup **popup = get_popup_from_xdg_popup(self, xdg_popup);

  g_return_if_fail (popup);
  xdg_popup_destroy((*popup)->popup);
  gtk_widget_destroy ((*popup)->window);
  *popup = NULL;
}

static const struct xdg_popup_listener xdg_popup_listener = {
	.configure = xdg_popup_configure,
	.popup_done = xdg_popup_done,
};


static void
favorites_activated_cb (PhoshShell *self,
                        PhoshPanel *window)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  GdkWindow *gdk_window;
  struct popup *favorites;
  struct xdg_surface *xdg_surface;
  struct xdg_positioner *xdg_positioner;
  gint width, height;
  PhoshWayland *wl = phosh_wayland_get_default();
  struct xdg_wm_base* xdg_wm_base = phosh_wayland_get_xdg_wm_base (wl);
  struct zwlr_layer_surface_v1 *panel_surface;

  close_menu (&priv->settings);
  if (priv->favorites) {
    close_menu (&priv->favorites);
    return;
  }

  favorites = calloc (1, sizeof *favorites);
  favorites->window = phosh_favorites_new ();

  gdk_window = gtk_widget_get_window (favorites->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);
  favorites->wl_surface = gdk_wayland_window_get_wl_surface (gdk_window);

  xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, favorites->wl_surface);
  g_return_if_fail (xdg_surface);
  xdg_positioner = xdg_wm_base_create_positioner(xdg_wm_base);
  gtk_window_get_size (GTK_WINDOW (favorites->window), &width, &height);
  xdg_positioner_set_size(xdg_positioner, width, height);
  xdg_positioner_set_offset(xdg_positioner, 0, PHOSH_PANEL_HEIGHT-1);
  xdg_positioner_set_anchor_rect(xdg_positioner, 0, 0, 1, 1);
  xdg_positioner_set_anchor(xdg_positioner, XDG_POSITIONER_ANCHOR_BOTTOM_LEFT);
  xdg_positioner_set_gravity(xdg_positioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);

  favorites->popup = xdg_surface_get_popup(xdg_surface, NULL, xdg_positioner);
  g_return_if_fail (favorites->popup);
  priv->favorites = favorites;

  panel_surface = phosh_layer_surface_get_layer_surface(priv->panel);
  /* TODO: how to get meaningful serial from gdk? */
  xdg_popup_grab(favorites->popup, phosh_wayland_get_wl_seat (wl), 1);
  zwlr_layer_surface_v1_get_popup(panel_surface, favorites->popup);
  xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
  xdg_popup_add_listener(favorites->popup, &xdg_popup_listener, self);

  wl_surface_commit(favorites->wl_surface);
  xdg_positioner_destroy(xdg_positioner);

  g_signal_connect_swapped (priv->favorites->window,
                            "app-launched",
                            G_CALLBACK(close_favorites_menu_cb),
                            self);
  g_signal_connect_swapped (priv->favorites->window,
                            "app-raised",
                            G_CALLBACK(close_favorites_menu_cb),
                            self);
  g_signal_connect_swapped (priv->favorites->window,
                            "selection-aborted",
                            G_CALLBACK(close_favorites_menu_cb),
                            self);
}


static void
setting_done_cb (PhoshShell *self,
                 PhoshSettings *settings)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  g_return_if_fail (priv->settings);
  close_menu (&priv->settings);
}


static void
settings_activated_cb (PhoshShell *self,
                       PhoshPanel *window)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  GdkWindow *gdk_window;
  struct popup *settings;
  struct xdg_surface *xdg_surface;
  struct xdg_positioner *xdg_positioner;
  gint width, height, panel_width;
  PhoshWayland *wl = phosh_wayland_get_default ();
  gpointer xdg_wm_base = phosh_wayland_get_xdg_wm_base(wl);
  struct zwlr_layer_surface_v1 *panel_surface;

  close_menu (&priv->favorites);
  if (priv->settings) {
    close_menu (&priv->settings);
    return;
  }

  settings = calloc (1, sizeof *settings);
  settings->window = phosh_settings_new ();

  gdk_window = gtk_widget_get_window (settings->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);
  settings->wl_surface = gdk_wayland_window_get_wl_surface (gdk_window);

  xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, settings->wl_surface);
  g_return_if_fail (xdg_surface);
  xdg_positioner = xdg_wm_base_create_positioner(xdg_wm_base);
  gtk_window_get_size (GTK_WINDOW (settings->window), &width, &height);
  xdg_positioner_set_size(xdg_positioner, width, height);
  phosh_shell_get_usable_area (self, NULL, NULL, &panel_width, NULL);
  xdg_positioner_set_offset(xdg_positioner, -width+1, PHOSH_PANEL_HEIGHT-1);
  xdg_positioner_set_anchor_rect(xdg_positioner, panel_width-1, 0, panel_width-2, 1);
  xdg_positioner_set_anchor(xdg_positioner, XDG_POSITIONER_ANCHOR_BOTTOM_LEFT);
  xdg_positioner_set_gravity(xdg_positioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);

  settings->popup = xdg_surface_get_popup(xdg_surface, NULL, xdg_positioner);
  g_return_if_fail (settings->popup);
  priv->settings = settings;

  panel_surface = phosh_layer_surface_get_layer_surface(priv->panel);
  /* TODO: how to get meaningful serial from GDK? */
  xdg_popup_grab(settings->popup, phosh_wayland_get_wl_seat (wl), 1);
  zwlr_layer_surface_v1_get_popup(panel_surface, settings->popup);
  xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
  xdg_popup_add_listener(settings->popup, &xdg_popup_listener, self);

  wl_surface_commit(settings->wl_surface);
  xdg_positioner_destroy(xdg_positioner);

  g_signal_connect_swapped (priv->settings->window,
                            "setting-done",
                            G_CALLBACK(setting_done_cb),
                            self);
}


void
phosh_shell_lock (PhoshShell *self)
{
  phosh_shell_set_locked (self, TRUE);
}


void
phosh_shell_unlock (PhoshShell *self)
{
  phosh_shell_set_locked (self, FALSE);
}


void
phosh_shell_set_locked (PhoshShell *self, gboolean state)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  phosh_lockscreen_manager_set_locked (priv->lockscreen_manager, state);
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_SHELL_PROP_LOCKED]);
}


static void
panels_create (PhoshShell *self)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  PhoshMonitor *monitor;
  PhoshWayland *wl = phosh_wayland_get_default ();

  monitor = phosh_shell_get_primary_monitor (self);
  g_return_if_fail (monitor);

  priv->panel = PHOSH_LAYER_SURFACE(phosh_panel_new (phosh_wayland_get_zwlr_layer_shell_v1(wl),
                                                     monitor->wl_output));
  priv->home = PHOSH_LAYER_SURFACE(phosh_home_new (phosh_wayland_get_zwlr_layer_shell_v1(wl),
                                                    monitor->wl_output));
  g_signal_connect_swapped (
    priv->panel,
    "favorites-activated",
    G_CALLBACK(favorites_activated_cb),
    self);

  g_signal_connect_swapped (
    priv->panel,
    "settings-activated",
    G_CALLBACK(settings_activated_cb),
    self);

  g_signal_connect_swapped (
    priv->home,
    "home-activated",
    G_CALLBACK(favorites_activated_cb),
    self);

}


static void
background_create (PhoshShell *self)
{

#ifdef WITH_PHOSH_BACKGROUND
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  PhoshMonitor *monitor;
  gint width, height;

  monitor = phosh_shell_get_primary_monitor (self);
  g_return_if_fail (monitor);
  phosh_shell_get_usable_area (self, NULL, NULL, &width, &height);

  /* set it up as the background */
  priv->background = phosh_background_new (
    priv->layer_shell, monitor->wl_output, width, height);
#endif
}

static void
css_setup (PhoshShell *self)
{
  GtkCssProvider *provider;
  GFile *file;
  GError *error = NULL;

  provider = gtk_css_provider_new ();
  file = g_file_new_for_uri ("resource:///sm/puri/phosh/style.css");

  if (!gtk_css_provider_load_from_file (provider, file, &error)) {
    g_warning ("Failed to load CSS file: %s", error->message);
    g_clear_error (&error);
    g_object_unref (file);
    return;
  }
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (provider), 600);
  g_object_unref (file);
}


static void
env_setup (void)
{
  g_setenv ("XDG_CURRENT_DESKTOP", "GNOME", TRUE);
}


static void
phosh_shell_set_property (GObject *object,
                          guint property_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
  PhoshShell *self = PHOSH_SHELL (object);

  switch (property_id) {
  case PHOSH_SHELL_PROP_ROTATION:
    phosh_shell_rotate_display (self, g_value_get_uint (value));
    break;
  case PHOSH_SHELL_PROP_LOCKED:
    phosh_shell_set_locked (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_shell_get_property (GObject *object,
                          guint property_id,
                          GValue *value,
                          GParamSpec *pspec)
{
  PhoshShell *self = PHOSH_SHELL (object);
  PhoshShellPrivate *priv = phosh_shell_get_instance_private(self);

  switch (property_id) {
  case PHOSH_SHELL_PROP_ROTATION:
    g_value_set_uint (value, priv->rotation);
    break;
  case PHOSH_SHELL_PROP_LOCKED:
    g_value_set_boolean (value,
                         phosh_lockscreen_manager_get_locked (priv->lockscreen_manager));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_shell_dispose (GObject *object)
{
  PhoshShell *self = PHOSH_SHELL (object);
  PhoshShellPrivate *priv = phosh_shell_get_instance_private(self);

  if (priv->background) {
    gtk_widget_destroy (GTK_WIDGET (priv->background));
    priv->background = NULL;
  }

  g_clear_pointer (&priv->panel, phosh_cp_widget_destroy);
  g_clear_object (&priv->lockscreen_manager);
  g_clear_object (&priv->monitor_manager);
  phosh_system_prompter_unregister ();
  phosh_session_unregister ();

  G_OBJECT_CLASS (phosh_shell_parent_class)->dispose (object);
}


static void
phosh_shell_constructed (GObject *object)
{
  PhoshShell *self = PHOSH_SHELL (object);
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  PhoshWayland *wl = phosh_wayland_get_default();
  GPtrArray *outputs;

  G_OBJECT_CLASS (phosh_shell_parent_class)->constructed (object);

  priv->monitor_manager = phosh_monitor_manager_new ();
  /* Add all initial outputs */
  outputs = phosh_wayland_get_wl_outputs (wl);
  for (int i = 0; i < outputs->len; i++) {
     phosh_monitor_manager_add_monitor (
       priv->monitor_manager,
       phosh_monitor_new_from_wl_output(outputs->pdata[i]));
  }

  gtk_icon_theme_add_resource_path (gtk_icon_theme_get_default (),
                                    "/sm/puri/phosh/icons");
  env_setup ();
  css_setup (self);
  panels_create (self);
  /* Create background after panel since it needs the panel's size */
  background_create (self);
  priv->lockscreen_manager = phosh_lockscreen_manager_new ();
  phosh_session_register ("sm.puri.Phosh");
  phosh_system_prompter_register ();
}


static void
phosh_shell_class_init (PhoshShellClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_shell_constructed;
  object_class->dispose = phosh_shell_dispose;

  object_class->set_property = phosh_shell_set_property;
  object_class->get_property = phosh_shell_get_property;

  props[PHOSH_SHELL_PROP_ROTATION] =
    g_param_spec_string ("rotation",
                         "Rotation",
                         "Clockwise display rotation in degree",
                         "",
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  props[PHOSH_SHELL_PROP_LOCKED] =
    g_param_spec_boolean ("locked",
                          "Locked",
                          "Whether the screen is locked",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PHOSH_SHELL_PROP_LAST_PROP, props);
}


static void
phosh_shell_init (PhoshShell *self)
{
}


gint
phosh_shell_get_rotation (PhoshShell *self)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  return priv->rotation;
}


void
phosh_shell_rotate_display (PhoshShell *self,
                            guint degree)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  PhoshWayland *wl = phosh_wayland_get_default();

  g_return_if_fail (phosh_wayland_get_phosh_private (wl));
  priv->rotation = degree;
  phosh_private_rotate_display (phosh_wayland_get_phosh_private (wl),
                                phosh_layer_surface_get_wl_surface (priv->panel),
                                degree);
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_SHELL_PROP_ROTATION]);
}


PhoshMonitor *
phosh_shell_get_primary_monitor (PhoshShell *self)
{
  PhoshShellPrivate *priv;
  PhoshMonitor *monitor;

  g_return_val_if_fail (PHOSH_IS_SHELL (self), NULL);
  priv = phosh_shell_get_instance_private (self);

  monitor = phosh_monitor_manager_get_monitor (priv->monitor_manager, 0);
  g_return_val_if_fail (monitor, NULL);

  return monitor;
}


PhoshMonitorManager *
phosh_shell_get_monitor_manager (PhoshShell *self)
{
  PhoshShellPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_SHELL (self), NULL);
  priv = phosh_shell_get_instance_private (self);

  g_return_val_if_fail (PHOSH_IS_MONITOR_MANAGER (priv->monitor_manager), NULL);
  return priv->monitor_manager;
}


/**
 * Returns the usable area in pixels usable by a client on the phone
 * display
 */
void
phosh_shell_get_usable_area (PhoshShell *self, gint *x, gint *y, gint *width, gint *height)
{
  PhoshMonitor *monitor;
  PhoshMonitorMode *mode;
  PhoshShellPrivate *priv;
  gint w, h;
  gint scale;

  g_return_if_fail (PHOSH_IS_SHELL (self));
  priv = phosh_shell_get_instance_private (self);

  monitor = phosh_shell_get_primary_monitor (self);
  g_return_if_fail(monitor);
  mode = phosh_monitor_get_current_mode (monitor);

  scale = monitor->scale ? monitor->scale : 1;
  if (priv->rotation) {
    w = mode->height / scale;
    h = mode->width / scale - PHOSH_PANEL_HEIGHT - PHOSH_HOME_HEIGHT;
  } else {
    w = mode->width / scale;
    h = mode->height / scale - PHOSH_PANEL_HEIGHT - PHOSH_HOME_HEIGHT;
  }

  if (x)
    *x = 0;
  if (y)
    *y = PHOSH_PANEL_HEIGHT;
  if (width)
    *width = w;
  if (height)
    *height = h;
}


PhoshShell *
phosh_shell_get_default (void)
{
  static PhoshShell *instance;

  if (instance == NULL) {
    instance = g_object_new (PHOSH_TYPE_SHELL, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
  }
  return instance;
}
