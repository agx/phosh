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
#include <gtk/gtk.h>
#include <gdk/gdkwayland.h>

#include "phosh-mobile-shell-client-protocol.h"

#include "phosh.h"
#include "background.h"
#include "lockscreen.h"
#include "panel.h"
#include "favorites.h"
#include "settings.h"

enum {
  PHOSH_SHELL_PROP_0,
  PHOSH_SHELL_PROP_ROTATION,
  PHOSH_SHELL_PROP_LAST_PROP
};
static GParamSpec *props[PHOSH_SHELL_PROP_LAST_PROP];

struct elem {
  GtkWidget *window;
  struct wl_surface *surface;
};

typedef struct
{
  struct wl_display *display;
  struct wl_registry *registry;
  struct phosh_mobile_shell *mshell;
  struct wl_output *output;

  struct wl_seat *seat;
  struct wl_pointer *pointer;

  GdkDisplay *gdk_display;
  gint rotation;

  /* Top panel */
  struct elem *panel;

  /* Background */
  struct elem *background;

  /* Lockscreen */
  struct elem *lockscreen;
  gulong unlock_handler_id;

  /* Favorites menu */
  GtkWidget *favorites;

  /* Settings menu */
  GtkWidget *settings;
} PhoshShellPrivate;


typedef struct _PhoshShell
{
  GObject parent;
} PhoshShell;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshShell, phosh_shell, G_TYPE_OBJECT)

/* Shell singleton */
PhoshShell *_phosh;


static void
lockscreen_unlock_cb (PhoshShell *self, PhoshLockscreen *window)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  g_signal_handler_disconnect (window, priv->unlock_handler_id);
  gtk_widget_destroy (GTK_WIDGET (window));
  g_free (priv->lockscreen);
  priv->lockscreen = NULL;
  phosh_mobile_shell_unlock(priv->mshell);
}


static void
favorites_activated_cb (PhoshShell *self,
                        PhoshPanel *window)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  phosh_menu_toggle (PHOSH_MENU (priv->favorites));
}


static void
app_launched_cb (PhoshShell *self,
                 PhoshFavorites *favorites)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  phosh_menu_hide (PHOSH_MENU (priv->favorites));
}


static void
settings_activated_cb (PhoshShell *self,
                       PhoshPanel *window)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  phosh_menu_toggle (PHOSH_MENU (priv->settings));
}


static void
setting_done_cb (PhoshShell *self,
                 PhoshFavorites *favorites)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  phosh_menu_hide (PHOSH_MENU (priv->settings));
}


static void
lockscreen_create (PhoshShell *self)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  struct elem *lockscreen;
  GdkWindow *gdk_window;

  lockscreen = g_malloc0 (sizeof *lockscreen);
  lockscreen->window = phosh_lockscreen_new ();

  gdk_window = gtk_widget_get_window (lockscreen->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);

  lockscreen->surface = gdk_wayland_window_get_wl_surface (gdk_window);

  phosh_mobile_shell_set_lock_surface(priv->mshell,
                                      lockscreen->surface);
  gtk_widget_show_all (lockscreen->window);
  priv->lockscreen = lockscreen;

  priv->unlock_handler_id = g_signal_connect_swapped (
    lockscreen->window,
    "lockscreen-unlock",
    G_CALLBACK(lockscreen_unlock_cb),
    self);
}


static void
favorites_create (PhoshShell *self)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  priv->favorites = phosh_favorites_new (PHOSH_MOBILE_SHELL_MENU_POSITION_LEFT,
                                         (gpointer) priv->mshell);

  gtk_widget_show_all (priv->favorites);

  g_signal_connect_swapped (priv->favorites,
                            "app-launched",
                            G_CALLBACK(app_launched_cb),
                            self);
}


static void
settings_create (PhoshShell *self)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  priv->settings = phosh_settings_new (PHOSH_MOBILE_SHELL_MENU_POSITION_RIGHT,
                                       (gpointer) priv->mshell);
  gtk_widget_show_all (priv->settings);

  g_signal_connect_swapped (priv->settings,
                            "setting-done",
                            G_CALLBACK(setting_done_cb),
                            self);
}


static void
panel_create (PhoshShell *self)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  struct elem *panel;
  GdkWindow *gdk_window;

  panel = calloc (1, sizeof *panel);
  panel->window = phosh_panel_new ();

  /* set it up as the panel */
  gdk_window = gtk_widget_get_window (panel->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);
  panel->surface = gdk_wayland_window_get_wl_surface (gdk_window);

  phosh_mobile_shell_set_user_data (priv->mshell, self);
  phosh_mobile_shell_set_panel (priv->mshell, priv->output,
      panel->surface);
  phosh_mobile_shell_set_panel_position (priv->mshell,
     PHOSH_MOBILE_SHELL_PANEL_POSITION_TOP);

  gtk_widget_show_all (panel->window);
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
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  GdkWindow *gdk_window;
  struct elem *background;

  background = calloc (1, sizeof *background);
  background->window = phosh_background_new ();

  gdk_window = gtk_widget_get_window (background->window);
  g_return_if_fail (gdk_window);

  gdk_wayland_window_set_use_custom_surface (gdk_window);
  background->surface = gdk_wayland_window_get_wl_surface (gdk_window);

  phosh_mobile_shell_set_user_data (priv->mshell, self);
  phosh_mobile_shell_set_background (priv->mshell, priv->output,
                                     background->surface);

  priv->background = background;
  gtk_widget_show_all (background->window);
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
shell_configure (PhoshShell *self,
                 uint32_t edges,
                 struct wl_surface *surface,
                 int32_t width, int32_t height)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  gtk_widget_set_size_request (priv->background->window,
      width, height);

  gtk_window_resize (GTK_WINDOW (priv->panel->window),
      width, PHOSH_PANEL_HEIGHT);

  phosh_mobile_shell_desktop_ready (priv->mshell);

  /* Create menus once we now the panel's position */
  if (!priv->favorites)
    favorites_create (self);
  if (!priv->settings)
    settings_create (self);
}


static void
phosh_mobile_shell_configure (void *data,
                              struct phosh_mobile_shell *phosh_mobile_shell,
                              uint32_t edges,
                              struct wl_surface *surface,
                              int32_t width, int32_t height)
{
  shell_configure(data, edges, surface, width, height);
}


static void
phosh_mobile_shell_prepare_lock_surface (void *data,
    struct phosh_mobile_shell *phosh_mobile_shell)
{
  PhoshShell *self = data;
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  if (!priv->lockscreen)
    lockscreen_create(self);
}


static void
phosh_mobile_shell_grab_cursor (void *data,
                                struct phosh_mobile_shell *phosh_mobile_shell,
    uint32_t cursor)
{
  g_warning("%s not implmented", __func__);
}


static const struct phosh_mobile_shell_listener mshell_listener = {
  phosh_mobile_shell_configure,
  phosh_mobile_shell_prepare_lock_surface,
  phosh_mobile_shell_grab_cursor
};


static void
registry_handle_global (void *data,
                        struct wl_registry *registry,
                        uint32_t name,
                        const char *interface,
                        uint32_t version)
{
  PhoshShell *self = data;
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  if (!strcmp (interface, "phosh_mobile_shell")) {
      priv->mshell = wl_registry_bind (registry, name,
          &phosh_mobile_shell_interface, MIN(version, 1));
      phosh_mobile_shell_add_listener (priv->mshell, &mshell_listener, self);
      phosh_mobile_shell_set_user_data (priv->mshell, self);
    }
  else if (!strcmp (interface, "wl_output")) {
      /* TODO: create multiple outputs */
      priv->output = wl_registry_bind (registry, name,
          &wl_output_interface, 1);
    }
}


static void
registry_handle_global_remove (void *data,
    struct wl_registry *registry,
    uint32_t name)
{
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
  PhoshShellPrivate *priv = phosh_shell_get_instance_private(self);

  switch (property_id) {
  case PHOSH_SHELL_PROP_ROTATION:
    priv->rotation = g_value_get_uint (value);
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
  wl_registry_add_listener (priv->registry,
      &registry_listener, self);

  /* Wait until we have been notified about the compositor,
   * shell, and shell helper objects */
  if (!priv->output || !priv->mshell)
    wl_display_roundtrip (priv->display);
  if (!priv->output || !priv->mshell) {
      g_error ("Could not find output, shell or helper modules\n"
               "output: %p, mshell: %p\n",
                 priv->output, priv->mshell);
  }

  env_setup ();
  css_setup (self);
  panel_create (self);
  /* Create background after panel since it needs the panel's size */
  background_create (self);
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

  priv->rotation = degree;
  phosh_mobile_shell_rotate_display (priv->mshell,
                                     priv->panel->surface,
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
  gint panel_height;

  g_return_if_fail(monitor);
  g_return_if_fail(priv->panel);
  g_return_if_fail(priv->panel->window);

  gdk_monitor_get_geometry (monitor, &geom);
  panel_height = phosh_panel_get_height (PHOSH_PANEL (priv->panel->window));

  /* GDK fails to take rotation into account
   * https://bugzilla.gnome.org/show_bug.cgi?id=793618 */
  if (priv->rotation != 90 && priv->rotation != 270) {
    *width = geom.width;
    *height = geom.height - panel_height;
  } else {
    *width = geom.height;
    *height = geom.width - panel_height;
  }

  *x = 0;
  *y = panel_height;
}


PhoshShell *
phosh ()
{
  return _phosh;
}


int main(int argc, char *argv[])
{
  gtk_init (&argc, &argv);

  g_object_new (PHOSH_TYPE_SHELL, NULL);
  gtk_main ();
  g_object_unref (_phosh);
  _phosh = NULL;

  return EXIT_SUCCESS;
}
