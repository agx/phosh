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
#include "phosh.h"

#include "phosh-wayland.h"
#include "monitor/monitor.h"
#include "background.h"
#include "lockscreen.h"
#include "lockshield.h"
#include "monitor-manager.h"
#include "panel.h"
#include "favorites.h"
#include "settings.h"

/* FIXME: use org.gnome.desktop.session.idle-delay */
#define LOCKSCREEN_TIMEOUT 300 * 1000

enum {
  PHOSH_SHELL_PROP_0,
  PHOSH_SHELL_PROP_ROTATION,
  PHOSH_SHELL_PROP_LOCKED,
  PHOSH_SHELL_PROP_LAST_PROP
};
static GParamSpec *props[PHOSH_SHELL_PROP_LAST_PROP];

struct elem {
  GtkWidget *window;
  struct wl_surface *wl_surface;
  struct zwlr_layer_surface_v1 *layer_surface;
};

struct popup {
  GtkWidget *window;
  struct wl_surface *wl_surface;
  struct xdg_popup *popup;
};

typedef struct
{
  gint rotation;

  /* Top panel */
  struct elem *panel;

  /* Background */
  GtkWidget *background;

  /* Lockscreen */
  struct elem *lockscreen;   /* phone display lock screen */
  GPtrArray *shields;        /* other outputs */
  gulong unlock_handler_id;
  struct org_kde_kwin_idle_timeout *lock_timer;
  struct zwlr_input_inhibitor_v1 *input_inhibitor;
  gboolean locked;

  /* Favorites menu */
  struct popup *favorites;

  /* Settings menu */
  struct popup *settings;

  /* MonitorManager */
  PhoshMonitorManager *monitor_manager;
} PhoshShellPrivate;


typedef struct _PhoshShell
{
  GObject parent;
} PhoshShell;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshShell, phosh_shell, G_TYPE_OBJECT)

/* Shell singleton */
static PhoshShell *_phosh;


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


static void layer_surface_configure(void *data,
    struct zwlr_layer_surface_v1 *surface,
    uint32_t serial, uint32_t w, uint32_t h) {
  struct elem *e = data;

  gtk_window_resize (GTK_WINDOW (e->window), w, h);
  zwlr_layer_surface_v1_ack_configure(surface, serial);
  gtk_widget_show_all (e->window);
}


static void layer_surface_closed(void *data,
    struct zwlr_layer_surface_v1 *surface) {
  struct elem *e = data;
  zwlr_layer_surface_v1_destroy(surface);
  gtk_widget_destroy (e->window);
}


struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_configure,
	.closed = layer_surface_closed,
};


static void
lockscreen_unlock_cb (PhoshShell *self, PhoshLockscreen *window)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  g_return_if_fail (PHOSH_IS_LOCKSCREEN (window));
  g_return_if_fail (window == PHOSH_LOCKSCREEN (priv->lockscreen->window));

  g_signal_handler_disconnect (window, priv->unlock_handler_id);
  priv->unlock_handler_id = 0;
  gtk_widget_destroy (GTK_WIDGET (priv->lockscreen->window));

  /* Unlock all other outputs */
  g_ptr_array_free (priv->shields, TRUE);
  priv->shields = NULL;

  priv->lockscreen->window = NULL;
  zwlr_layer_surface_v1_destroy(priv->lockscreen->layer_surface);
  g_free (priv->lockscreen);
  priv->lockscreen = NULL;

  zwlr_input_inhibitor_v1_destroy(priv->input_inhibitor);
  priv->input_inhibitor = NULL;

  priv->locked = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_SHELL_PROP_LOCKED]);
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
app_launched_cb (PhoshShell *self,
                 PhoshFavorites *favorites)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  g_return_if_fail (priv->favorites);
  close_menu (&priv->favorites);
}


static void
favorites_selection_aborted (PhoshShell *self,
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

  /* TODO: how to get meaningful serial from gdk? */
  xdg_popup_grab(favorites->popup, phosh_wayland_get_wl_seat (wl), 1);
  zwlr_layer_surface_v1_get_popup(priv->panel->layer_surface, favorites->popup);
  xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
  xdg_popup_add_listener(favorites->popup, &xdg_popup_listener, self);

  wl_surface_commit(favorites->wl_surface);
  xdg_positioner_destroy(xdg_positioner);

  g_signal_connect_swapped (priv->favorites->window,
                            "app-launched",
                            G_CALLBACK(app_launched_cb),
                            self);
  g_signal_connect_swapped (priv->favorites->window,
                            "selection-aborted",
                            G_CALLBACK(favorites_selection_aborted),
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

  /* TODO: how to get meaningful serial from GDK? */
  xdg_popup_grab(settings->popup, phosh_wayland_get_wl_seat (wl), 1);
  zwlr_layer_surface_v1_get_popup(priv->panel->layer_surface, settings->popup);
  xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
  xdg_popup_add_listener(settings->popup, &xdg_popup_listener, self);

  wl_surface_commit(settings->wl_surface);
  xdg_positioner_destroy(xdg_positioner);

  g_signal_connect_swapped (priv->settings->window,
                            "setting-done",
                            G_CALLBACK(setting_done_cb),
                            self);
}


static void
lockscreen_create (PhoshShell *self)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  GdkWindow *gdk_window;
  struct elem *lockscreen;
  PhoshMonitor *monitor;
  PhoshWayland *wl = phosh_wayland_get_default ();

  monitor = phosh_shell_get_primary_monitor ();
  g_return_if_fail (monitor);

  lockscreen = g_malloc0 (sizeof *lockscreen);
  lockscreen->window = phosh_lockscreen_new ();

  priv->input_inhibitor =
    zwlr_input_inhibit_manager_v1_get_inhibitor(
      phosh_wayland_get_zwlr_input_inhibit_manager_v1 (wl));

  gdk_window = gtk_widget_get_window (lockscreen->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);

  lockscreen->wl_surface = gdk_wayland_window_get_wl_surface (gdk_window);
  lockscreen->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
    phosh_wayland_get_zwlr_layer_shell_v1(wl),
    lockscreen->wl_surface,
    monitor->wl_output,
    ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
    "lockscreen");
  zwlr_layer_surface_v1_set_exclusive_zone(lockscreen->layer_surface, -1);
  zwlr_layer_surface_v1_set_size(lockscreen->layer_surface, 0, 0);
  zwlr_layer_surface_v1_set_anchor(lockscreen->layer_surface,
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
  zwlr_layer_surface_v1_set_keyboard_interactivity(lockscreen->layer_surface, TRUE);
  zwlr_layer_surface_v1_add_listener(lockscreen->layer_surface, &layer_surface_listener, lockscreen);
  wl_surface_commit(lockscreen->wl_surface);
  priv->lockscreen = lockscreen;

  /* Lock all other outputs */
  priv->shields = g_ptr_array_new_with_free_func ((GDestroyNotify) (gtk_widget_destroy));
  for (int i = 1; i < phosh_monitor_manager_get_num_monitors (priv->monitor_manager); i++) {
    monitor = phosh_monitor_manager_get_monitor(priv->monitor_manager, i);
    if (monitor == NULL)
      continue;
    g_ptr_array_add (priv->shields, phosh_lockshield_new (
                       phosh_wayland_get_zwlr_layer_shell_v1 (wl),
                       monitor->wl_output));
  }

  priv->unlock_handler_id = g_signal_connect_swapped (
    lockscreen->window,
    "lockscreen-unlock",
    G_CALLBACK(lockscreen_unlock_cb),
    self);

  priv->locked = TRUE;
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_SHELL_PROP_LOCKED]);
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

  if (state == priv->locked)
    return;

  if (state)
    lockscreen_create (self);
  else
    lockscreen_unlock_cb (self, PHOSH_LOCKSCREEN (priv->lockscreen->window));
}


static void lock_idle_cb(void* data, struct org_kde_kwin_idle_timeout *timer)
{
  PhoshShell *self = data;
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  g_return_if_fail (PHOSH_IS_SHELL (data));
  if (!priv->lockscreen)
    lockscreen_create(self);
}


static void lock_resume_cb(void* data, struct org_kde_kwin_idle_timeout *timer)
{
}


static const struct org_kde_kwin_idle_timeout_listener idle_timer_listener = {
  .idle = lock_idle_cb,
  .resumed = lock_resume_cb,
};


static void
lockscreen_prepare (PhoshShell *self)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  PhoshWayland *wl = phosh_wayland_get_default ();

  struct org_kde_kwin_idle *idle_manager = phosh_wayland_get_org_kde_kwin_idle (wl);


  g_return_if_fail(idle_manager);
  priv->lock_timer = org_kde_kwin_idle_get_idle_timeout(idle_manager,
                                                        phosh_wayland_get_wl_seat (wl),
                                                        LOCKSCREEN_TIMEOUT);
  g_return_if_fail (priv->lock_timer);
  org_kde_kwin_idle_timeout_add_listener(priv->lock_timer,
                                         &idle_timer_listener,
                                         self);
}


PhoshMonitor *
get_primary_monitor (PhoshShell *self)
{
  PhoshShellPrivate *priv;
  PhoshMonitor *monitor;

  g_return_val_if_fail (PHOSH_IS_SHELL (self), NULL);
  priv = phosh_shell_get_instance_private (self);

  monitor = phosh_monitor_manager_get_monitor (priv->monitor_manager, 0);
  g_return_val_if_fail (monitor, NULL);

  return monitor;
}


static void
panel_create (PhoshShell *self)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  struct elem *panel;
  GdkWindow *gdk_window;
  gint width;
  PhoshMonitor *monitor;
  PhoshWayland *wl = phosh_wayland_get_default ();

  monitor = get_primary_monitor (self);
  g_return_if_fail (monitor);

  panel = calloc (1, sizeof *panel);
  panel->window = phosh_panel_new ();

  phosh_shell_get_usable_area (self, NULL, NULL, &width, NULL);
  /* set it up as the panel */
  gdk_window = gtk_widget_get_window (panel->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);
  panel->wl_surface = gdk_wayland_window_get_wl_surface (gdk_window);
  panel->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
    phosh_wayland_get_zwlr_layer_shell_v1 (wl),
    panel->wl_surface,
    monitor->wl_output,
    ZWLR_LAYER_SHELL_V1_LAYER_TOP,
    "phosh");
  zwlr_layer_surface_v1_set_anchor(panel->layer_surface,
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
  zwlr_layer_surface_v1_set_size(panel->layer_surface, 0, PHOSH_PANEL_HEIGHT);
  zwlr_layer_surface_v1_set_exclusive_zone(panel->layer_surface, PHOSH_PANEL_HEIGHT);
  zwlr_layer_surface_v1_add_listener(panel->layer_surface, &layer_surface_listener, panel);
  wl_surface_commit(panel->wl_surface);
  priv->panel = panel;

  g_signal_connect_swapped (
    panel->window,
    "favorites-activated",
    G_CALLBACK(favorites_activated_cb),
    self);

  g_signal_connect_swapped (
    panel->window,
    "settings-activated",
    G_CALLBACK(settings_activated_cb),
    self);
}



static void
background_create (PhoshShell *self)
{

#ifdef WITH_PHOSH_BACKGROUND
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  PhoshMonitor *monitor;
  gint width, height;

  monitor = phosh_shell_get_primary_monitor ();
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
env_setup ()
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
    g_value_set_boolean (value, priv->locked);
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
    gtk_widget_destroy (priv->background);
    priv->background = NULL;
  }

  if (priv->shields) {
    g_ptr_array_free (priv->shields, TRUE);
    priv->shields = NULL;
  }

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
  panel_create (self);
  /* Create background after panel since it needs the panel's size */
  background_create (self);
  lockscreen_prepare (self);
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
                                priv->panel->wl_surface,
                                degree);
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_SHELL_PROP_ROTATION]);
}


PhoshMonitor *
phosh_shell_get_primary_monitor ()
{
  g_return_val_if_fail (PHOSH_IS_SHELL (_phosh), NULL);
  return get_primary_monitor (_phosh);
}


/**
 * Returns the usable area in pixels usable by a client on the phone
 * display
 */
void
phosh_shell_get_usable_area (PhoshShell *self, gint *x, gint *y, gint *width, gint *height)
{
  PhoshMonitor *monitor = NULL;
  gint panel_height = 0;
  gint w, h;

  monitor = get_primary_monitor (self);
  g_return_if_fail(monitor);

  w = monitor->width;
  h = monitor->height - panel_height;

  if (x)
    *x = 0;
  if (y)
    *y = panel_height;
  if (width)
    *width = w;
  if (height)
    *height = h;
}


PhoshShell *
phosh ()
{
  return _phosh;
}


gboolean
sigterm_cb (gpointer unused)
{
  g_debug ("Cleaning up");
  gtk_main_quit ();
  return FALSE;
}


int main(int argc, char *argv[])
{
  g_autoptr(GSource) sigterm;
  GMainContext *context;
  g_autoptr(GOptionContext) opt_context;
  GError *err = NULL;
  gboolean unlocked = FALSE;
  g_autoptr(PhoshWayland) wl;

  const GOptionEntry options [] = {
    {"unlocked", 'U', 0, G_OPTION_ARG_NONE, &unlocked,
     "Don't start with screen locked", NULL},
    { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
  };

  opt_context = g_option_context_new ("- A phone graphical shell");
  g_option_context_add_main_entries (opt_context, options, NULL);
  g_option_context_add_group (opt_context, gtk_get_option_group (TRUE));
  g_option_context_parse (opt_context, &argc, &argv, &err);

  textdomain (GETTEXT_PACKAGE);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  gtk_init (&argc, &argv);

  sigterm = g_unix_signal_source_new (SIGTERM);
  context = g_main_context_default ();
  g_source_set_callback (sigterm, sigterm_cb, NULL, NULL);
  g_source_attach (sigterm, context);

  wl = phosh_wayland_get_default ();
  _phosh = g_object_new (PHOSH_TYPE_SHELL, NULL);
  if (!unlocked)
    phosh_shell_lock (_phosh);

  gtk_main ();
  g_object_unref (_phosh);
  _phosh = NULL;

  return EXIT_SUCCESS;
}
