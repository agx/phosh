/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Based on maynard's panel which is
 * Copyright (C) 2014 Collabora Ltd. *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkwayland.h>

#include "config.h"

#include "idle-client-protocol.h"
#include "phosh-private-client-protocol.h"
#include "wlr-input-inhibitor-unstable-v1-client-protocol.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"

#include "phosh.h"
#include "background.h"
#include "lockscreen.h"
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
  struct wl_display *display;
  struct wl_registry *registry;
  struct phosh_private *mshell;
  struct zwlr_layer_shell_v1 *layer_shell;
  struct org_kde_kwin_idle *idle_manager;
  struct zwlr_input_inhibit_manager_v1 *input_inhibit_manager;
  struct zwlr_input_inhibitor_v1 *input_inhibitor;
  struct wl_output *output;
  struct xdg_wm_base *xdg_wm_base;

  GdkDisplay *gdk_display;
  gint rotation;

  /* Top panel */
  struct elem *panel;

  /* Background */
  struct elem *background;

  /* Lockscreen */
  struct elem *lockscreen;
  gulong unlock_handler_id;
  struct org_kde_kwin_idle_timeout *lock_timer;
  gboolean locked;

  /* Favorites menu */
  struct popup *favorites;

  /* Settings menu */
  struct popup *settings;
} PhoshShellPrivate;


typedef struct _PhoshShell
{
  GObject parent;
} PhoshShell;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshShell, phosh_shell, G_TYPE_OBJECT)

/* Shell singleton */
static PhoshShell *_phosh;


static struct wl_seat*
get_seat (PhoshShell *self)
{
  PhoshShellPrivate *priv;
  GdkSeat *gdk_seat;

  g_return_val_if_fail (PHOSH_IS_SHELL (self), NULL);
  priv = phosh_shell_get_instance_private (self);
  gdk_seat = gdk_display_get_default_seat(priv->gdk_display);
  return gdk_wayland_seat_get_wl_seat (gdk_seat);
}


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
app_launched_cb (PhoshShell *self,
                 PhoshFavorites *favorites)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  g_return_if_fail (priv->favorites);
  gtk_window_close (GTK_WINDOW (priv->favorites->window));
  gtk_widget_destroy (GTK_WIDGET (priv->favorites->window));
  free (priv->favorites);
  priv->favorites = NULL;
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

  if (priv->favorites)
    return;

  favorites = calloc (1, sizeof *favorites);
  favorites->window = phosh_favorites_new ();

  gdk_window = gtk_widget_get_window (favorites->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);
  favorites->wl_surface = gdk_wayland_window_get_wl_surface (gdk_window);

  xdg_surface = xdg_wm_base_get_xdg_surface(priv->xdg_wm_base, favorites->wl_surface);
  g_return_if_fail (xdg_surface);
  xdg_positioner = xdg_wm_base_create_positioner(priv->xdg_wm_base);
  gtk_window_get_size (GTK_WINDOW (favorites->window), &width, &height);
  xdg_positioner_set_size(xdg_positioner, width, height);
  xdg_positioner_set_offset(xdg_positioner, 0, PHOSH_PANEL_HEIGHT-1);
  xdg_positioner_set_anchor_rect(xdg_positioner, 100, 0, 1, 1);
  xdg_positioner_set_anchor(xdg_positioner, XDG_POSITIONER_ANCHOR_BOTTOM);
  xdg_positioner_set_gravity(xdg_positioner, XDG_POSITIONER_GRAVITY_BOTTOM);

  favorites->popup = xdg_surface_get_popup(xdg_surface, NULL, xdg_positioner);
  g_return_if_fail (favorites->popup);
  priv->favorites = favorites;

  /* TODO: how to get meaningful serial from gdk? */
  xdg_popup_grab(favorites->popup, get_seat(self), 1);
  zwlr_layer_surface_v1_get_popup(priv->panel->layer_surface, favorites->popup);
  xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
  xdg_popup_add_listener(favorites->popup, &xdg_popup_listener, self);

  wl_surface_commit(favorites->wl_surface);
  xdg_positioner_destroy(xdg_positioner);

  g_signal_connect_swapped (priv->favorites->window,
                            "app-launched",
                            G_CALLBACK(app_launched_cb),
                            self);
}


static void
setting_done_cb (PhoshShell *self,
                 PhoshSettings *settings)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  g_return_if_fail (priv->settings);

  gtk_window_close (GTK_WINDOW (priv->settings->window));
  gtk_widget_destroy (GTK_WIDGET (priv->settings->window));
  free (priv->settings);
  priv->settings = NULL;
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
  gint width, height;

  if (priv->settings)
    return;

  settings = calloc (1, sizeof *settings);
  settings->window = phosh_settings_new ();

  gdk_window = gtk_widget_get_window (settings->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);
  settings->wl_surface = gdk_wayland_window_get_wl_surface (gdk_window);

  xdg_surface = xdg_wm_base_get_xdg_surface(priv->xdg_wm_base, settings->wl_surface);
  g_return_if_fail (xdg_surface);
  xdg_positioner = xdg_wm_base_create_positioner(priv->xdg_wm_base);
  gtk_window_get_size (GTK_WINDOW (settings->window), &width, &height);
  xdg_positioner_set_size(xdg_positioner, width, height);
  phosh_shell_get_usable_area (self, NULL, NULL, &width, NULL);
  xdg_positioner_set_offset(xdg_positioner, 0, PHOSH_PANEL_HEIGHT-1);
  xdg_positioner_set_anchor_rect(xdg_positioner, width, 0, 1, 1);
  xdg_positioner_set_anchor(xdg_positioner, XDG_POSITIONER_ANCHOR_BOTTOM);
  xdg_positioner_set_gravity(xdg_positioner, XDG_POSITIONER_GRAVITY_BOTTOM);

  settings->popup = xdg_surface_get_popup(xdg_surface, NULL, xdg_positioner);
  g_return_if_fail (settings->popup);
  priv->settings = settings;

  /* TODO: how to get meaningful serial from GDK? */
  xdg_popup_grab(settings->popup, get_seat(self), 1);
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
  struct elem *lockscreen;
  GdkWindow *gdk_window;

  lockscreen = g_malloc0 (sizeof *lockscreen);
  lockscreen->window = phosh_lockscreen_new ();

  priv->input_inhibitor =
    zwlr_input_inhibit_manager_v1_get_inhibitor(priv->input_inhibit_manager);

  gdk_window = gtk_widget_get_window (lockscreen->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);

  lockscreen->wl_surface = gdk_wayland_window_get_wl_surface (gdk_window);
  lockscreen->layer_surface = zwlr_layer_shell_v1_get_layer_surface(priv->layer_shell,
                                                                    lockscreen->wl_surface,
                                                                    priv->output,
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

  g_return_if_fail(priv->idle_manager);
  g_return_if_fail(priv->gdk_display);

  priv->lock_timer = org_kde_kwin_idle_get_idle_timeout(
    priv->idle_manager,
    get_seat(self),
    LOCKSCREEN_TIMEOUT);

  g_return_if_fail (priv->lock_timer);

  org_kde_kwin_idle_timeout_add_listener(priv->lock_timer,
                                         &idle_timer_listener,
                                         self);
}


static void
panel_create (PhoshShell *self)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  struct elem *panel;
  GdkWindow *gdk_window;
  gint width;

  panel = calloc (1, sizeof *panel);
  panel->window = phosh_panel_new ();

  phosh_shell_get_usable_area (self, NULL, NULL, &width, NULL);
  /* set it up as the panel */
  gdk_window = gtk_widget_get_window (panel->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);
  panel->wl_surface = gdk_wayland_window_get_wl_surface (gdk_window);
  panel->layer_surface = zwlr_layer_shell_v1_get_layer_surface(priv->layer_shell,
                                                               panel->wl_surface,
                                                               priv->output,
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


#if 0  /* https://github.com/swaywm/wlroots/issues/897 */
static void
background_create (PhoshShell *self)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  GdkWindow *gdk_window;
  struct elem *background;
  gint width, height;

  background = calloc (1, sizeof *background);
  background->window = phosh_background_new ();

  phosh_shell_get_usable_area (self, NULL, NULL, &width, &height);
  /* set it up as the background */
  gdk_window = gtk_widget_get_window (background->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);
  background->wl_surface = gdk_wayland_window_get_wl_surface (gdk_window);
  background->layer_surface =
    zwlr_layer_shell_v1_get_layer_surface(priv->layer_shell,
                                          background->wl_surface,
                                          priv->output,
                                          ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND,
                                          "phosh");
  zwlr_layer_surface_v1_set_anchor(background->layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM);
  zwlr_layer_surface_v1_set_size(background->layer_surface, width, height);
  zwlr_layer_surface_v1_add_listener(background->layer_surface, &layer_surface_listener, background);
  wl_surface_commit(background->wl_surface);
  priv->background = background;
}
#endif

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
registry_handle_global (void *data,
                        struct wl_registry *registry,
                        uint32_t name,
                        const char *interface,
                        uint32_t version)
{
  PhoshShell *self = data;
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  if (!strcmp (interface, "phosh_private")) {
      priv->mshell = wl_registry_bind (registry, name,
          &phosh_private_interface, 1);
  } else  if (!strcmp (interface, zwlr_layer_shell_v1_interface.name)) {
      priv->layer_shell = wl_registry_bind (registry, name,
          &zwlr_layer_shell_v1_interface, 1);
  } else if (!strcmp (interface, "wl_output")) {
      /* TODO: create multiple outputs */
      priv->output = wl_registry_bind (registry, name,
          &wl_output_interface, 1);
  } else if (!strcmp (interface, "org_kde_kwin_idle")) {
    priv->idle_manager = wl_registry_bind (registry,
                                           name,
                                           &org_kde_kwin_idle_interface, 1);
  } else if (!strcmp(interface, "wl_seat")) {
#if 0 /* FIXME: this breaks GTK+ input since GTK+ binds it as well */
    priv->seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
#endif
  } else if (!strcmp(interface, zwlr_input_inhibit_manager_v1_interface.name)) {
    priv->input_inhibit_manager = wl_registry_bind(
      registry,
      name,
      &zwlr_input_inhibit_manager_v1_interface,
      1);
  } else if (!strcmp(interface, xdg_wm_base_interface.name)) {
    priv->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
  }
}


static void
registry_handle_global_remove (void *data,
    struct wl_registry *registry,
    uint32_t name)
{
  // TODO
}


static const struct wl_registry_listener registry_listener = {
  registry_handle_global,
  registry_handle_global_remove
};


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
  case PHOSH_SHELL_PROP_LOCKED:
    g_value_set_boolean (value, priv->locked);
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_shell_constructed (GObject *object)
{
  PhoshShell *self = PHOSH_SHELL (object);
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  _phosh = self; /* set singleton */

  G_OBJECT_CLASS (phosh_shell_parent_class)->constructed (object);

  gdk_set_allowed_backends ("wayland");
  priv->gdk_display = gdk_display_get_default ();
  priv->display =
    gdk_wayland_display_get_wl_display (priv->gdk_display);

  if (priv->display == NULL) {
      g_error ("Failed to get display: %m\n");
  }

  priv->registry = wl_display_get_registry (priv->display);
  wl_registry_add_listener (priv->registry, &registry_listener, self);

  /* Wait until we have been notified about the compositor,
   * shell, and shell helper objects */
  if (!priv->output || !priv->layer_shell || !priv->idle_manager ||
      !priv->input_inhibit_manager || !priv->mshell || !priv->xdg_wm_base)
    wl_display_roundtrip (priv->display);
  if (!priv->output || !priv->layer_shell || !priv->idle_manager ||
      !priv->input_inhibit_manager || !priv->xdg_wm_base) {
    g_error ("Could not find needed globals\n"
             "output: %p, layer_shell: %p, seat: %p, "
             "inhibit: %p, xdg_wm: %p\n",
             priv->output, priv->layer_shell, priv->idle_manager,
             priv->input_inhibit_manager, priv->xdg_wm_base);
  }
  if (!priv->mshell) {
    g_warning ("Could not find phosh global, disabling some features\n");
  }


  env_setup ();
  css_setup (self);
  panel_create (self);
  /* Create background after panel since it needs the panel's size */
#if 0
  /* https://github.com/swaywm/wlroots/issues/897 */
  background_create (self);
#endif
  lockscreen_prepare (self);
}


static void
phosh_shell_class_init (PhoshShellClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_shell_constructed;

  object_class->set_property = phosh_shell_set_property;
  object_class->get_property = phosh_shell_get_property;

  props[PHOSH_SHELL_PROP_ROTATION] =
    g_param_spec_string ("rotation",
                         "Rotation",
                         "Clockwise display rotation in degree",
                         "",
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  props[PHOSH_SHELL_PROP_LOCKED] =
    g_param_spec_string ("locked",
                         "Locked",
                         "Whether the screen is locked",
                         "",
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

  g_return_if_fail (priv->mshell);

  priv->rotation = degree;
  phosh_private_rotate_display (priv->mshell,
                                priv->panel->wl_surface,
                                degree);
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_SHELL_PROP_ROTATION]);
}


/**
 * Returns the usable area in pixels usable by a client on the phone
 * display
 */
void
phosh_shell_get_usable_area (PhoshShell *self, gint *x, gint *y, gint *width, gint *height)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  GdkDisplay *display = gdk_display_get_default ();
  /* There's no primary monitor on nested wayland so just use the
     first one for now */
  GdkMonitor *monitor = gdk_display_get_monitor (display, 0);
  GdkRectangle geom;
  gint panel_height = 0;
  gint w, h;

  g_return_if_fail(monitor);
  gdk_monitor_get_geometry (monitor, &geom);

  if (priv->panel && priv->panel->window)
    panel_height = phosh_panel_get_height (PHOSH_PANEL (priv->panel->window));

  /* GDK fails to take rotation into account
   * https://bugzilla.gnome.org/show_bug.cgi?id=793618 */
  if (priv->rotation != 90 && priv->rotation != 270) {
    w = geom.width;
    h = geom.height - panel_height;
  } else {
    w = geom.height;
    h = geom.width - panel_height;
  }

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


int main(int argc, char *argv[])
{
  textdomain (GETTEXT_PACKAGE);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  gtk_init (&argc, &argv);

  g_object_new (PHOSH_TYPE_SHELL, NULL);
  gtk_main ();
  g_object_unref (_phosh);
  _phosh = NULL;

  return EXIT_SUCCESS;
}
