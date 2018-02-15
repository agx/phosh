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

#include <gtk/gtk.h>
#include <gdk/gdkwayland.h>

#include "phosh-mobile-shell-client-protocol.h"

#include "phosh.h"
#include "lockscreen.h"
#include "panel.h"
#include "favorites.h"
#include "settings.h"


struct elem {
  GtkWidget *window;
  GdkPixbuf *pixbuf;
  struct wl_surface *surface;
};

struct desktop {
  struct wl_display *display;
  struct wl_registry *registry;
  struct phosh_mobile_shell *mshell;
  struct wl_output *output;

  struct wl_seat *seat;
  struct wl_pointer *pointer;

  GdkDisplay *gdk_display;

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
};


struct desktop *phosh;


static void
lockscreen_unlock_cb (struct desktop *desktop, PhoshLockscreen *window)
{
  phosh_mobile_shell_unlock (desktop->mshell);

  g_signal_handler_disconnect (window, desktop->unlock_handler_id);
  gtk_widget_destroy (GTK_WIDGET (window));
  g_free (desktop->lockscreen);
  desktop->lockscreen = NULL;
  phosh_mobile_shell_unlock(desktop->mshell);
}


static void
favorites_activated_cb (struct desktop *desktop, PhoshPanel *window)
{
  phosh_menu_toggle (PHOSH_MENU (desktop->favorites));
}


static void
app_launched_cb (struct desktop *desktop, PhoshFavorites *favorites)
{
  phosh_menu_hide (PHOSH_MENU (desktop->favorites));
}


static void
settings_activated_cb (struct desktop *desktop, PhoshPanel *window)
{
  phosh_menu_toggle (PHOSH_MENU (desktop->settings));
}


static void
lockscreen_create (struct desktop *desktop)
{
  struct elem *lockscreen;
  GdkWindow *gdk_window;

  lockscreen = g_malloc0 (sizeof *lockscreen);
  lockscreen->window = phosh_lockscreen_new ();

  gdk_window = gtk_widget_get_window (lockscreen->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);

  lockscreen->surface = gdk_wayland_window_get_wl_surface (gdk_window);

  phosh_mobile_shell_set_lock_surface(desktop->mshell,
				      lockscreen->surface);
  gtk_widget_show_all (lockscreen->window);
  desktop->lockscreen = lockscreen;

  desktop->unlock_handler_id = g_signal_connect_swapped (
    lockscreen->window,
    "lockscreen-unlock",
    G_CALLBACK(lockscreen_unlock_cb),
    desktop);
}


static void
favorites_create (struct desktop *desktop)
{
  desktop->favorites = phosh_favorites_new (PHOSH_MOBILE_SHELL_MENU_POSITION_LEFT,
					    (gpointer) desktop->mshell);

  gtk_widget_show_all (desktop->favorites);

  g_signal_connect_swapped (desktop->favorites,
			    "app-launched",
			    G_CALLBACK(app_launched_cb),
			    desktop);
}


static void
settings_create (struct desktop *desktop)
{
  desktop->settings = phosh_settings_new (PHOSH_MOBILE_SHELL_MENU_POSITION_RIGHT,
					  (gpointer) desktop->mshell);
  gtk_widget_show_all (desktop->settings);
}


static void
panel_create (struct desktop *desktop)
{
  struct elem *panel;
  GdkWindow *gdk_window;

  panel = calloc (1, sizeof *panel);
  panel->window = phosh_panel_new ();

  /* set it up as the panel */
  gdk_window = gtk_widget_get_window (panel->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);
  panel->surface = gdk_wayland_window_get_wl_surface (gdk_window);

  phosh_mobile_shell_set_user_data (desktop->mshell, desktop);
  phosh_mobile_shell_set_panel (desktop->mshell, desktop->output,
      panel->surface);
  phosh_mobile_shell_set_panel_position (desktop->mshell,
     PHOSH_MOBILE_SHELL_PANEL_POSITION_TOP);

  gtk_widget_show_all (panel->window);
  desktop->panel = panel;

  g_signal_connect_swapped (
    panel->window,
    "favorites-activated",
    G_CALLBACK(favorites_activated_cb),
    desktop);

  g_signal_connect_swapped (
    panel->window,
    "settings-activated",
    G_CALLBACK(settings_activated_cb),
    desktop);
}


static GdkPixbuf *
scale_background (GdkPixbuf *original_pixbuf)
{
  GdkDisplay *display = gdk_display_get_default ();
  /* There's no primary monitor on nested wayland so just use the
     first one for now */
  GdkMonitor *monitor = gdk_display_get_monitor (display, 0);
  GdkRectangle geom;
  gint original_width, original_height;
  gint final_width, final_height;
  gdouble ratio_horizontal, ratio_vertical;

  g_return_val_if_fail(monitor, NULL);

  gdk_monitor_get_geometry (monitor, &geom);

  original_width = gdk_pixbuf_get_width (original_pixbuf);
  original_height = gdk_pixbuf_get_height (original_pixbuf);

  ratio_horizontal = (double) geom.width / original_width;
  ratio_vertical = (double) geom.height / original_height;

  final_width = ceil (ratio_horizontal * original_width);
  final_height = ceil (ratio_vertical * original_height);

  return gdk_pixbuf_scale_simple (original_pixbuf,
      final_width, final_height, GDK_INTERP_BILINEAR);
}


static void
background_destroy_cb (GObject *object,
    gpointer data)
{
  gtk_main_quit ();
}


static gboolean
background_draw_cb (GtkWidget *widget,
    cairo_t *cr,
    gpointer data)
{
  struct desktop *desktop = data;

  gdk_cairo_set_source_pixbuf (cr, desktop->background->pixbuf, 0, 0);
  cairo_paint (cr);
  return TRUE;
}


static void
background_create (struct desktop *desktop)
{
  GdkWindow *gdk_window;
  struct elem *background;
  GdkPixbuf *unscaled_background;
  const gchar *xpm_data[] = {"1 1 1 1", "_ c WebGrey", "_"};

  background = calloc (1, sizeof *background);

  unscaled_background = gdk_pixbuf_new_from_xpm_data (xpm_data);
  background->pixbuf = scale_background (unscaled_background);
  g_object_unref (unscaled_background);

  background->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  g_signal_connect (background->window, "destroy",
      G_CALLBACK (background_destroy_cb), NULL);

  g_signal_connect (background->window, "draw",
      G_CALLBACK (background_draw_cb), desktop);

  gtk_window_set_title (GTK_WINDOW (background->window), "phosh background");
  gtk_window_set_decorated (GTK_WINDOW (background->window), FALSE);
  gtk_widget_realize (background->window);

  gdk_window = gtk_widget_get_window (background->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);

  background->surface = gdk_wayland_window_get_wl_surface (gdk_window);

  phosh_mobile_shell_set_user_data (desktop->mshell, desktop);
  phosh_mobile_shell_set_background (desktop->mshell, desktop->output,
      background->surface);

  desktop->background = background;
  gtk_widget_show_all (background->window);
}


static void
css_setup (struct desktop *desktop)
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
shell_configure (struct desktop *desktop,
    uint32_t edges,
    struct wl_surface *surface,
    int32_t width, int32_t height)
{
  gtk_widget_set_size_request (desktop->background->window,
      width, height);

  gtk_window_resize (GTK_WINDOW (desktop->panel->window),
      width, PHOSH_PANEL_HEIGHT);

  phosh_mobile_shell_desktop_ready (desktop->mshell);

  /* Create menus once we now the panel's position */
  if (!desktop->favorites)
    favorites_create (desktop);
  if (!desktop->settings)
    settings_create (desktop);
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
  struct desktop *desktop = data;

  if (!desktop->lockscreen)
    lockscreen_create(desktop);
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
  struct desktop *d = data;

  if (!strcmp (interface, "phosh_mobile_shell")) {
      d->mshell = wl_registry_bind (registry, name,
          &phosh_mobile_shell_interface, MIN(version, 1));
      phosh_mobile_shell_add_listener (d->mshell, &mshell_listener, d);
      phosh_mobile_shell_set_user_data (d->mshell, d);
    }
  else if (!strcmp (interface, "wl_output")) {
      /* TODO: create multiple outputs */
      d->output = wl_registry_bind (registry, name,
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


int main(int argc, char *argv[])
{
  struct desktop *desktop;

  gdk_set_allowed_backends ("wayland");

  gtk_init (&argc, &argv);

  desktop = g_malloc0 (sizeof *desktop);

  desktop->gdk_display = gdk_display_get_default ();
  desktop->display =
    gdk_wayland_display_get_wl_display (desktop->gdk_display);

  if (desktop->display == NULL) {
      g_error ("Failed to get display: %m\n");
      return -1;
  }

  desktop->registry = wl_display_get_registry (desktop->display);
  wl_registry_add_listener (desktop->registry,
      &registry_listener, desktop);

  /* Wait until we have been notified about the compositor,
   * shell, and shell helper objects */
  if (!desktop->output || !desktop->mshell)
    wl_display_roundtrip (desktop->display);
  if (!desktop->output || !desktop->mshell) {
      g_error ("Could not find output, shell or helper modules\n"
               "output: %p, mshell: %p\n",
		 desktop->output, desktop->mshell);
      return -1;
  }

  /* FIXME: use the phosh global everywhere in this file */
  phosh = desktop;
  css_setup (desktop);
  background_create (desktop);
  panel_create (desktop);

  gtk_main ();

  return EXIT_SUCCESS;
}


void
phosh_rotate_display (guint degree)
{
  phosh_mobile_shell_rotate_display (phosh->mshell,
				     phosh->panel->surface,
				     degree);
}
