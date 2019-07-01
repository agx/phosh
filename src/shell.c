/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Once based on maynard's panel which is
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

#include "batteryinfo.h"
#include "background-manager.h"
#include "favorites.h"
#include "home.h"
#include "idle-manager.h"
#include "lockscreen-manager.h"
#include "monitor-manager.h"
#include "monitor/monitor.h"
#include "osk-manager.h"
#include "panel.h"
#include "phosh-wayland.h"
#include "polkit-auth-agent.h"
#include "session.h"
#include "settings.h"
#include "system-prompter.h"
#include "util.h"
#include "wifiinfo.h"
#include "wwaninfo.h"


enum {
  PHOSH_SHELL_PROP_0,
  PHOSH_SHELL_PROP_ROTATION,
  PHOSH_SHELL_PROP_LOCKED,
  PHOSH_SHELL_PROP_PRIMARY_MONITOR,
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
  PhoshLayerSurface *panel;
  PhoshLayerSurface *home;
  struct popup *favorites;
  struct popup *settings;

  PhoshBackgroundManager *background_manager;
  PhoshMonitor *primary_monitor;
  PhoshMonitorManager *monitor_manager;
  PhoshLockscreenManager *lockscreen_manager;
  PhoshIdleManager *idle_manager;
  PhoshOskManager  *osk_manager;
  PhoshWifiManager *wifi_manager;
  PhoshPolkitAuthAgent *polkit_auth_agent;
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
home_activated_cb (PhoshShell *self,
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

  phosh_osk_manager_set_visible (priv->osk_manager, FALSE);

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

  phosh_osk_manager_set_visible (priv->osk_manager, FALSE);

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
  gtk_widget_show (GTK_WIDGET (priv->panel));

  priv->home = PHOSH_LAYER_SURFACE(phosh_home_new (phosh_wayland_get_zwlr_layer_shell_v1(wl),
                                                    monitor->wl_output));
  gtk_widget_show (GTK_WIDGET (priv->home));

  g_signal_connect_swapped (
    priv->panel,
    "settings-activated",
    G_CALLBACK(settings_activated_cb),
    self);

  g_signal_connect_swapped (
    priv->home,
    "home-activated",
    G_CALLBACK(home_activated_cb),
    self);
}


static void
panels_dispose (PhoshShell *self)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  g_clear_pointer (&priv->panel, phosh_cp_widget_destroy);
  g_clear_pointer (&priv->home, phosh_cp_widget_destroy);
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
  case PHOSH_SHELL_PROP_PRIMARY_MONITOR:
    phosh_shell_set_primary_monitor (self, g_value_get_object (value));
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
    g_value_set_uint (value, phosh_monitor_get_rotation(priv->primary_monitor));
    break;
  case PHOSH_SHELL_PROP_LOCKED:
    g_value_set_boolean (value,
                         phosh_lockscreen_manager_get_locked (priv->lockscreen_manager));
    break;
  case PHOSH_SHELL_PROP_PRIMARY_MONITOR:
    g_value_set_object (value, phosh_shell_get_primary_monitor (self));
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

  panels_dispose (self);
  g_clear_object (&priv->lockscreen_manager);
  g_clear_object (&priv->monitor_manager);
  g_clear_object (&priv->wifi_manager);
  g_clear_object (&priv->osk_manager);
  g_clear_object (&priv->polkit_auth_agent);
  g_clear_object (&priv->background_manager);
  phosh_system_prompter_unregister ();
  phosh_session_unregister ();

  G_OBJECT_CLASS (phosh_shell_parent_class)->dispose (object);
}


static gboolean
setup_idle_cb (PhoshShell *self)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  panels_create (self);
  /* Create background after panel since it needs the panel's size */
  priv->background_manager = phosh_background_manager_new ();

  return FALSE;
}


/* Load all types that might be used in UI files */
static void
type_setup (void)
{
  phosh_battery_info_get_type();
  phosh_wifi_info_get_type();
  phosh_wwan_info_get_type();
}


static void
phosh_shell_constructed (GObject *object)
{
  PhoshShell *self = PHOSH_SHELL (object);
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  G_OBJECT_CLASS (phosh_shell_parent_class)->constructed (object);

  priv->monitor_manager = phosh_monitor_manager_new ();
  if (phosh_monitor_manager_get_num_monitors(priv->monitor_manager)) {
    priv->primary_monitor = phosh_monitor_manager_get_monitor (
      priv->monitor_manager, 0);
  }
  gtk_icon_theme_add_resource_path (gtk_icon_theme_get_default (),
                                    "/sm/puri/phosh/icons");
  env_setup ();
  css_setup (self);
  type_setup ();

  priv->lockscreen_manager = phosh_lockscreen_manager_new ();
  priv->idle_manager = phosh_idle_manager_get_default();

  phosh_session_register ("sm.puri.Phosh");
  phosh_system_prompter_register ();
  priv->polkit_auth_agent = phosh_polkit_auth_agent_new ();

  g_idle_add ((GSourceFunc) setup_idle_cb, self);
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
    g_param_spec_uint ("rotation",
                       "Rotation",
                       "Clockwise display rotation in degree",
                       0,
                       360,
                       0,
                       G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  props[PHOSH_SHELL_PROP_LOCKED] =
    g_param_spec_boolean ("locked",
                          "Locked",
                          "Whether the screen is locked",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PHOSH_SHELL_PROP_PRIMARY_MONITOR] =
    g_param_spec_object ("primary-monitor",
                         "Primary monitor",
                         "The primary monitor",
                         PHOSH_TYPE_MONITOR,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PHOSH_SHELL_PROP_LAST_PROP, props);
}


static void
phosh_shell_init (PhoshShell *self)
{
  GtkSettings *gtk_settings;

  gtk_settings = gtk_settings_get_default ();
  g_object_set (G_OBJECT (gtk_settings), "gtk-application-prefer-dark-theme", TRUE, NULL);
}


gint
phosh_shell_get_rotation (PhoshShell *self)
{
  PhoshShellPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_SHELL (self), 0);
  priv = phosh_shell_get_instance_private (self);
  g_return_val_if_fail (priv->primary_monitor, 0);
  return phosh_monitor_get_rotation (priv->primary_monitor);
}


void
phosh_shell_rotate_display (PhoshShell *self,
                            guint degree)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  PhoshWayland *wl = phosh_wayland_get_default();
  guint current;

  g_return_if_fail (phosh_wayland_get_phosh_private (wl));
  g_return_if_fail (priv->primary_monitor);
  current = phosh_monitor_get_rotation (priv->primary_monitor);
  if (current == degree)
    return;

  phosh_private_rotate_display (phosh_wayland_get_phosh_private (wl),
                                phosh_layer_surface_get_wl_surface (priv->panel),
                                degree);
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_SHELL_PROP_ROTATION]);
}


void
phosh_shell_set_primary_monitor (PhoshShell *self, PhoshMonitor *monitor)
{
  PhoshShellPrivate *priv;
  PhoshMonitor *m = NULL;

  g_return_if_fail (monitor);
  g_return_if_fail (PHOSH_IS_SHELL (self));
  priv = phosh_shell_get_instance_private (self);

  for (int i = 0; i < phosh_monitor_manager_get_num_monitors (priv->monitor_manager); i++) {
    m = phosh_monitor_manager_get_monitor (priv->monitor_manager, i);
    if (monitor == m)
      break;
  }
  g_return_if_fail (monitor == m);

  priv->primary_monitor = monitor;
  /* Move panels to the new monitor be recreating the layer shell surfaces */
  panels_dispose (self);
  panels_create (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_SHELL_PROP_PRIMARY_MONITOR]);
}


PhoshMonitor *
phosh_shell_get_primary_monitor (PhoshShell *self)
{
  PhoshShellPrivate *priv;
  PhoshMonitor *monitor;

  g_return_val_if_fail (PHOSH_IS_SHELL (self), NULL);
  priv = phosh_shell_get_instance_private (self);

  if (priv->primary_monitor)
    return priv->primary_monitor;

  /* When the shell started up we might not have had all monitors */
  monitor = phosh_monitor_manager_get_monitor (priv->monitor_manager, 0);
  g_return_val_if_fail (monitor, NULL);

  return monitor;
}


PhoshLockscreenManager *
phosh_shell_get_lockscreen_manager (PhoshShell *self)
{
  PhoshShellPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_SHELL (self), NULL);
  priv = phosh_shell_get_instance_private (self);

  g_return_val_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (priv->lockscreen_manager), NULL);
  return priv->lockscreen_manager;
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


PhoshWifiManager *
phosh_shell_get_wifi_manager (PhoshShell *self)
{
  PhoshShellPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_SHELL (self), NULL);
  priv = phosh_shell_get_instance_private (self);

  if (!priv->wifi_manager)
      priv->wifi_manager = phosh_wifi_manager_new ();

  g_return_val_if_fail (PHOSH_IS_WIFI_MANAGER (priv->wifi_manager), NULL);
  return priv->wifi_manager;
}


PhoshOskManager *
phosh_shell_get_osk_manager (PhoshShell *self)
{
  PhoshShellPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_SHELL (self), NULL);
  priv = phosh_shell_get_instance_private (self);

  if (!priv->osk_manager)
      priv->osk_manager = phosh_osk_manager_new ();

  g_return_val_if_fail (PHOSH_IS_OSK_MANAGER (priv->osk_manager), NULL);
  return priv->osk_manager;
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
  gint w, h;
  gint scale;

  g_return_if_fail (PHOSH_IS_SHELL (self));

  monitor = phosh_shell_get_primary_monitor (self);
  g_return_if_fail(monitor);
  mode = phosh_monitor_get_current_mode (monitor);
  g_return_if_fail (mode != NULL);

  scale = monitor->scale ? monitor->scale : 1;

  g_debug ("Primary monitor %p scale is %d, transform is %d",
           monitor,
           monitor->scale,
           monitor->transform);

  switch (phosh_monitor_get_rotation(monitor)) {
  case 0:
  case 180:
    w = mode->width / scale;
    h = mode->height / scale - PHOSH_PANEL_HEIGHT - PHOSH_HOME_HEIGHT;
    break;
  default:
    w = mode->height / scale;
    h = mode->width / scale - PHOSH_PANEL_HEIGHT - PHOSH_HOME_HEIGHT;
    break;
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
    g_debug("Creating shell");
    instance = g_object_new (PHOSH_TYPE_SHELL, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
  }
  return instance;
}
