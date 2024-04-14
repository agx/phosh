/*
 * Copyright (C) 2018-2022 Purism SPC
 *               2023-2024 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-background-manager"

#include "background-manager.h"
#include "background.h"
#include "background-cache.h"
#include "manager.h"
#include "monitor/monitor.h"
#include "phosh-wayland.h"
#include "shell.h"
#include "util.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-bg.h>
#include <libgnome-desktop/gnome-bg-slide-show.h>

#include <gdk/gdkwayland.h>
#include <gio/gio.h>

#include <math.h>
#include <string.h>

#define BG_KEY_PRIMARY_COLOR      "primary-color"
#define BG_KEY_PICTURE_OPTIONS    "picture-options"
#define BG_KEY_PICTURE_URI        "picture-uri"
#define BG_KEY_PICTURE_URI_DARK   "picture-uri-dark"

#define IF_KEY_COLOR_SCHEME       "color-scheme"

/**
 * PhoshBackgroundManager:
 *
 * `PhoshBackgroundManager` keeps tracks of [type@PhoshMonitor]s to
 * create [type@PhoshBackground]s that are responsible for rendering
 * the background (or wallpaper). Whenever either the monitors'
 * configuration or the configured wallpaper properties change the
 * backgrounds are notified to update their contents.
 */

enum {
  CONFIG_CHANGED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


struct _PhoshBackgroundManager {
  PhoshManager             parent;

  PhoshMonitor            *primary_monitor;
  GHashTable              *backgrounds; /* key: PhoshMonitor, value: PhoshBackground */

  GDesktopBackgroundStyle  style;
  GnomeBGSlideShow        *slideshow;
  GFile                   *file;        /* Background XML or image */
  GdkRGBA                  color;
  GSettings               *settings;
  GSettings               *interface_settings;

  GCancellable            *cancel_load;
};

G_DEFINE_TYPE (PhoshBackgroundManager, phosh_background_manager, PHOSH_TYPE_MANAGER);


static void
update_background (gpointer key, gpointer value, gpointer user_data)
{
  PhoshBackground *background = PHOSH_BACKGROUND (value);

  phosh_background_needs_update (background);
}


static void
update_all_backgrounds (PhoshBackgroundManager *self)
{
  g_hash_table_foreach (self->backgrounds, update_background, self);

  /* We only notify config changed and don't append any background data as the specific data
     depends on the surface it applies to (e.g. it's size) */
  g_signal_emit (self, signals[CONFIG_CHANGED], 0);
}


static void
on_slideshow_loaded (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GnomeBGSlideShow) slideshow = GNOME_BG_SLIDE_SHOW (source_object);
  PhoshBackgroundManager *self = PHOSH_BACKGROUND_MANAGER (user_data);
  g_autoptr (GError) err = NULL;

  g_return_if_fail (GNOME_BG_IS_SLIDE_SHOW (slideshow));
  g_return_if_fail (PHOSH_IS_BACKGROUND_MANAGER (self));

  if (!g_task_propagate_boolean (G_TASK (res), &err)) {
    phosh_async_error_warn (err, "Failed to load %s", g_file_peek_path (self->file));
    return;
  }

  self->slideshow = g_steal_pointer (&slideshow);
  update_all_backgrounds (self);
}


static void
load_slideshow (PhoshBackgroundManager *self)
{
  g_autoptr (GError) err = NULL;
  g_autofree char *path = NULL;
  GnomeBGSlideShow *slideshow;

  g_return_if_fail (G_IS_FILE (self->file));

  path = g_file_get_path (self->file);
  if (!path) {
    g_warning ("Couldn't get filename for %s: %s", path, err->message);
    return;
  }

  g_debug ("Loading slideshow '%s'", path);
  slideshow = gnome_bg_slide_show_new (path);

  g_cancellable_cancel (self->cancel_load);
  self->cancel_load = g_cancellable_new ();
  gnome_bg_slide_show_load_async (slideshow, self->cancel_load, on_slideshow_loaded, self);
}


static gboolean
is_slideshow (PhoshBackgroundManager *self)
{
  if (!self->file)
    return FALSE;

  return g_str_has_suffix (g_file_peek_path (self->file), ".xml");
}


static void
color_from_string (GdkRGBA *color, const char *string)
{
  if (!gdk_rgba_parse (color, string))
    gdk_rgba_parse (color, "black");
}

static void
on_settings_changed (PhoshBackgroundManager *self)
{
  GdkRGBA color;
  GDesktopBackgroundStyle style;
  g_autofree char *color_name = NULL;
  g_autofree char *val = NULL;
  g_autoptr (GFile) file = NULL;
  PhoshBackgroundCache *cache = phosh_background_cache_get_default ();
  GDesktopColorScheme scheme;
  const char *key;

  style = g_settings_get_enum (self->settings, BG_KEY_PICTURE_OPTIONS);
  color_name = g_settings_get_string (self->settings, BG_KEY_PRIMARY_COLOR);
  color_from_string (&color, color_name);

  scheme = g_settings_get_enum (self->interface_settings, IF_KEY_COLOR_SCHEME);
  key = scheme == G_DESKTOP_COLOR_SCHEME_PREFER_DARK ? BG_KEY_PICTURE_URI_DARK : BG_KEY_PICTURE_URI;
  val = g_settings_get_string (self->settings, key);

  if (g_str_has_prefix (val, "/") || g_str_has_prefix (val, "file:///"))
    file = g_file_new_for_uri (val);
  else if (g_strcmp0 (val, ""))
    g_warning ("Invalid background path %s", val);

  if (self->style == style && gdk_rgba_equal (&self->color, &color) &&
      phosh_util_file_equal (self->file, file)) {
    return;
  }

  /* Clear cache if uri changed */
  if (!phosh_util_file_equal (self->file, file))
    phosh_background_cache_clear_all (cache);

  self->style = style;
  self->color = color;
  g_clear_object (&self->slideshow);
  g_clear_object (&self->file);
  self->file = g_steal_pointer (&file);

  if (is_slideshow (self)) {
    load_slideshow (self);
  } else {
    /* Single file backed image or no image at all */
    update_all_backgrounds (self);
  }
}


static PhoshBackground *
create_background_for_monitor (PhoshBackgroundManager *self, PhoshMonitor *monitor)
{
  PhoshWayland *wl = phosh_wayland_get_default();
  GtkWidget *background;

  background = phosh_background_new (phosh_wayland_get_zwlr_layer_shell_v1 (wl),
                                     monitor,
                                     monitor == self->primary_monitor);
  return PHOSH_BACKGROUND (background);
}


static void
on_monitor_removed (PhoshBackgroundManager *self,
                    PhoshMonitor           *monitor,
                    PhoshMonitorManager    *monitormanager)
{
  g_return_if_fail (PHOSH_IS_BACKGROUND_MANAGER (self));
  g_return_if_fail (PHOSH_IS_MONITOR (monitor));

  g_debug ("Monitor %p removed", monitor);
  g_return_if_fail (g_hash_table_remove (self->backgrounds, monitor));
}


static void
on_monitor_configured (PhoshBackgroundManager *self, PhoshMonitor *monitor)
{
  PhoshBackground *background;
  float scale;

  g_return_if_fail (PHOSH_IS_MONITOR (monitor));

  scale = phosh_monitor_get_fractional_scale (monitor);
  g_debug ("Monitor %p (%s) configured, scale %f", monitor, monitor->name, scale);

  background = g_hash_table_lookup (self->backgrounds, monitor);
  if (background == NULL) {
    background = create_background_for_monitor (self, monitor);
    g_hash_table_insert (self->backgrounds, g_object_ref (monitor), background);
  } else {
    phosh_background_needs_update (background);
  }

  gtk_widget_show (GTK_WIDGET (background));
}


static void
on_monitor_added (PhoshBackgroundManager *self,
                  PhoshMonitor           *monitor,
                  PhoshMonitorManager    *unused)
{
  g_return_if_fail (PHOSH_IS_BACKGROUND_MANAGER (self));
  g_return_if_fail (PHOSH_IS_MONITOR (monitor));

  g_debug ("Monitor %p added", monitor);

  g_signal_connect_object (monitor, "configured",
                           G_CALLBACK (on_monitor_configured),
                           self,
                           G_CONNECT_SWAPPED);
  if (phosh_monitor_is_configured (monitor))
    on_monitor_configured (self, monitor);
}


static void
on_primary_monitor_changed (PhoshBackgroundManager *self,
                            GParamSpec *pspec,
                            PhoshShell *shell)
{
  PhoshBackground *background;
  PhoshMonitor *monitor;

  g_return_if_fail (PHOSH_IS_BACKGROUND_MANAGER (self));
  g_return_if_fail (PHOSH_IS_SHELL (shell));

  monitor = phosh_shell_get_primary_monitor (shell);
  if (monitor == self->primary_monitor)
    return;

  if (self->primary_monitor) {
    background = g_hash_table_lookup (self->backgrounds, self->primary_monitor);
    if (background)
      phosh_background_set_primary (background, FALSE);
  }

  g_set_object (&self->primary_monitor, monitor);

  if (monitor) {
    background = g_hash_table_lookup (self->backgrounds, monitor);
    if (background)
      phosh_background_set_primary (background, TRUE);
  }
}


static void
phosh_background_manager_idle_init (PhoshManager *manager)
{
  PhoshBackgroundManager *self = PHOSH_BACKGROUND_MANAGER (manager);
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshMonitorManager *monitor_manager = phosh_shell_get_monitor_manager (shell);

  self->settings = g_settings_new ("org.gnome.desktop.background");
  g_object_connect (self->settings,
                    "swapped_signal::changed::" BG_KEY_PICTURE_URI, on_settings_changed, self,
                    "swapped_signal::changed::" BG_KEY_PICTURE_OPTIONS, on_settings_changed, self,
                    "swapped_signal::changed::" BG_KEY_PRIMARY_COLOR, on_settings_changed, self,
                    NULL);
  self->interface_settings = g_settings_new ("org.gnome.desktop.interface");
  g_signal_connect_swapped (self->interface_settings,
                            "changed::" IF_KEY_COLOR_SCHEME,
                            G_CALLBACK (on_settings_changed),
                            self);
  /* Fill in initial values */
  on_settings_changed (self);

  /* Listen for monitor changes */
  g_object_connect (monitor_manager,
                    "swapped-object-signal::monitor-added", on_monitor_added, self,
                    "swapped-object-signal::monitor-removed", on_monitor_removed, self,
                    NULL);
  g_signal_connect_swapped (shell, "notify::primary-monitor",
                            G_CALLBACK (on_primary_monitor_changed),
                            self);
  self->primary_monitor = g_object_ref (phosh_shell_get_primary_monitor (shell));

 /* catch up with monitors already present */
  for (int i = 0; i < phosh_monitor_manager_get_num_monitors (monitor_manager); i++) {
    PhoshMonitor *monitor = phosh_monitor_manager_get_monitor (monitor_manager, i);

    on_monitor_added (self, monitor, NULL);
  }
}


static void
phosh_background_manager_finalize (GObject *object)
{
  PhoshBackgroundManager *self = PHOSH_BACKGROUND_MANAGER (object);

  g_cancellable_cancel (self->cancel_load);
  g_clear_object (&self->cancel_load);

  g_hash_table_destroy (self->backgrounds);
  g_clear_object (&self->primary_monitor);
  g_clear_object (&self->settings);
  g_clear_object (&self->interface_settings);
  g_clear_object (&self->slideshow);
  g_clear_object (&self->file);

  G_OBJECT_CLASS (phosh_background_manager_parent_class)->finalize (object);
}


static void
phosh_background_manager_class_init (PhoshBackgroundManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhoshManagerClass *manager_class = PHOSH_MANAGER_CLASS (klass);

  object_class->finalize = phosh_background_manager_finalize;

  manager_class->idle_init = phosh_background_manager_idle_init;

  /**
   * PhoshBackgroundManager::config-changed:
   * @self: The backgroundd manager
   *
   */
  signals[CONFIG_CHANGED] =
    g_signal_new ("config-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE,
                  0);
}


static void
phosh_background_manager_init (PhoshBackgroundManager *self)
{
  self->backgrounds = g_hash_table_new_full (g_direct_hash,
                                             g_direct_equal,
                                             g_object_unref,
                                             (GDestroyNotify)gtk_widget_destroy);
}

PhoshBackgroundManager *
phosh_background_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_BACKGROUND_MANAGER, NULL);
}

/**
 * phosh_background_manager_get_backgrounds:
 * @self: The #PhoshBackgroundManager
 *
 * Returns:(transfer container) (element-type PhoshBackground): The current backgrounds
 */
GList *
phosh_background_manager_get_backgrounds (PhoshBackgroundManager *self)
{
  g_return_val_if_fail (PHOSH_IS_BACKGROUND_MANAGER (self), NULL);

  return g_hash_table_get_values (self->backgrounds);
}

/**
 * phosh_background_manager_get_data: (skip)
 * @self: The background manager
 * @background: The background to fetch information fore
 *
 * Get the data that allows a [type@Background] to build it's
 * image. This is the single place that determines this so other parts
 * don't need to care whether we handle a slide or a single image.
 *
 * Returns:(transfer full)(nullable): The data allowing to build a background image
 */
PhoshBackgroundData *
phosh_background_manager_get_data (PhoshBackgroundManager *self, PhoshBackground *background)
{
  g_autoptr (PhoshBackgroundData) bg_data = g_new0 (PhoshBackgroundData, 1);

  g_return_val_if_fail (PHOSH_IS_BACKGROUND_MANAGER (self), NULL);
  g_return_val_if_fail (PHOSH_IS_BACKGROUND (background), NULL);

  *bg_data = (PhoshBackgroundData) {
    .color = self->color,
    .style = self->style,
  };

  /* Slideshow did not load successfully */
  if (is_slideshow (self) && !self->slideshow)
    return g_steal_pointer (&bg_data);

  if (self->slideshow) {
    gint width, height;
    gboolean fixed;
    const gchar *file1;

    width = phosh_layer_surface_get_configured_width (PHOSH_LAYER_SURFACE (background));
    height = phosh_layer_surface_get_configured_height (PHOSH_LAYER_SURFACE (background));

    g_assert (GNOME_BG_IS_SLIDE_SHOW (self->slideshow));

    /* TODO: handle actual slideshows (fixed == false) */
    gnome_bg_slide_show_get_slide (self->slideshow, 0, width, height,
                                   NULL, NULL, &fixed, &file1, NULL);
    g_debug ("Background file: %s, fixed: %d", file1, fixed);
    if (!fixed)
      g_warning ("Only fixed slideshows supported properly atm");

    bg_data->uri = g_file_new_for_path (file1);
  } else if (self->file) {
    bg_data->uri = g_object_ref (self->file);
  }

  return g_steal_pointer (&bg_data);
}
