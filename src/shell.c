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
#include "fader.h"
#include "feedback-manager.h"
#include "home.h"
#include "idle-manager.h"
#include "lockscreen-manager.h"
#include "monitor-manager.h"
#include "monitor/monitor.h"
#include "notifications/notify-manager.h"
#include "notifications/notification-banner.h"
#include "osk-manager.h"
#include "panel.h"
#include "phosh-wayland.h"
#include "polkit-auth-agent.h"
#include "proximity.h"
#include "sensor-proxy-manager.h"
#include "screen-saver-manager.h"
#include "session.h"
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

typedef struct
{
  PhoshLayerSurface *panel;
  PhoshLayerSurface *home;
  GPtrArray *faders;              /* for final fade out */

  GtkWidget *notification_banner;

  PhoshBackgroundManager *background_manager;
  PhoshMonitor *primary_monitor;
  PhoshMonitor *builtin_monitor;
  PhoshMonitorManager *monitor_manager;
  PhoshLockscreenManager *lockscreen_manager;
  PhoshIdleManager *idle_manager;
  PhoshOskManager  *osk_manager;
  PhoshToplevelManager *toplevel_manager;
  PhoshWifiManager *wifi_manager;
  PhoshPolkitAuthAgent *polkit_auth_agent;
  PhoshScreenSaverManager *screen_saver_manager;
  PhoshNotifyManager *notify_manager;
  PhoshFeedbackManager *feedback_manager;

  /* sensors */
  PhoshSensorProxyManager *sensor_proxy_manager;
  PhoshProximity *proximity;

  gboolean startup_finished;
} PhoshShellPrivate;


typedef struct _PhoshShell
{
  GObject parent;
} PhoshShell;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshShell, phosh_shell, G_TYPE_OBJECT)


static void
settings_activated_cb (PhoshShell *self,
                       PhoshPanel *window)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  g_return_if_fail (PHOSH_IS_PANEL (priv->panel));
  phosh_panel_toggle_fold (PHOSH_PANEL(priv->panel));
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

/**
 * phosh_shell_set_locked:
 *
 * Lock the shell. We proxy to lockscreen-manager to avoid
 * that other parts of the shell need to care about this
 * abstraction.
 */
void
phosh_shell_set_locked (PhoshShell *self, gboolean state)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);
  gboolean current;

  current = phosh_lockscreen_manager_get_locked (priv->lockscreen_manager);

  if (current == state)
    return;

  phosh_lockscreen_manager_set_locked (priv->lockscreen_manager, state);
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_SHELL_PROP_LOCKED]);
}

static void
on_home_state_changed (PhoshShell *self, GParamSpec *pspec, PhoshHome *home)
{
  PhoshShellPrivate *priv;
  PhoshHomeState state;

  g_return_if_fail (PHOSH_IS_SHELL (self));
  g_return_if_fail (PHOSH_IS_HOME (home));

  priv = phosh_shell_get_instance_private (self);

  g_object_get (priv->home, "state", &state, NULL);
  if (state == PHOSH_HOME_STATE_UNFOLDED) {
    phosh_panel_fold (PHOSH_PANEL (priv->panel));
    phosh_osk_manager_set_visible (priv->osk_manager, FALSE);
  }
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
    "notify::state",
    G_CALLBACK(on_home_state_changed),
    self);
}


static void
panels_dispose (PhoshShell *self)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  g_clear_pointer (&priv->panel, phosh_cp_widget_destroy);
  g_clear_pointer (&priv->home, phosh_cp_widget_destroy);
  g_clear_pointer (&priv->faders, g_ptr_array_unref);
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

  if (priv->sensor_proxy_manager) {
    phosh_dbus_sensor_proxy_call_release_accelerometer_sync (
      PHOSH_DBUS_SENSOR_PROXY(priv->sensor_proxy_manager),
      NULL, NULL);
      g_clear_object (&priv->sensor_proxy_manager);
  }

  panels_dispose (self);
  g_clear_object (&priv->notification_banner);
  g_clear_object (&priv->notify_manager);
  g_clear_object (&priv->screen_saver_manager);
  g_clear_object (&priv->lockscreen_manager);
  g_clear_object (&priv->monitor_manager);
  g_clear_object (&priv->toplevel_manager);
  g_clear_object (&priv->wifi_manager);
  g_clear_object (&priv->osk_manager);
  g_clear_object (&priv->polkit_auth_agent);
  g_clear_object (&priv->background_manager);
  g_clear_object (&priv->proximity);
  g_clear_object (&priv->sensor_proxy_manager);
  g_clear_object (&priv->feedback_manager);
  phosh_system_prompter_unregister ();
  phosh_session_unregister ();

  G_OBJECT_CLASS (phosh_shell_parent_class)->dispose (object);
}


static void
on_num_toplevels_changed (PhoshShell *self, GParamSpec *pspec, PhoshToplevelManager *toplevel_manager)
{
  PhoshShellPrivate *priv;

  g_return_if_fail (PHOSH_IS_SHELL (self));
  g_return_if_fail (PHOSH_IS_TOPLEVEL_MANAGER (toplevel_manager));

  priv = phosh_shell_get_instance_private (self);
  /* all toplevels gone, show the overview */
  /* TODO: once we have unfoldable app-drawer unfold that too */
  if (!phosh_toplevel_manager_get_num_toplevels (toplevel_manager))
    phosh_home_set_state (PHOSH_HOME (priv->home), PHOSH_HOME_STATE_UNFOLDED);
}


static void
on_toplevel_added (PhoshShell *self, GParamSpec *pspec, PhoshToplevelManager *toplevel_manager)
{
  PhoshShellPrivate *priv;

  g_return_if_fail (PHOSH_IS_SHELL (self));
  g_return_if_fail (PHOSH_IS_TOPLEVEL_MANAGER (toplevel_manager));

  priv = phosh_shell_get_instance_private (self);
  phosh_home_set_state (PHOSH_HOME (priv->home), PHOSH_HOME_STATE_FOLDED);
}


static void
on_new_notification (PhoshShell         *self,
                     PhoshNotification  *notification,
                     PhoshNotifyManager *manager)
{
  PhoshShellPrivate *priv;

  g_return_if_fail (PHOSH_IS_SHELL (self));
  g_return_if_fail (PHOSH_IS_NOTIFICATION (notification));
  g_return_if_fail (PHOSH_IS_NOTIFY_MANAGER (manager));

  priv = phosh_shell_get_instance_private (self);

  /* Clear existing banner */
  if (priv->notification_banner && GTK_IS_WIDGET (priv->notification_banner)) {
    gtk_widget_destroy (priv->notification_banner);
  }

  if (phosh_notify_manager_get_show_banners (manager) &&
      !phosh_lockscreen_manager_get_locked (priv->lockscreen_manager)) {
    g_set_weak_pointer (&priv->notification_banner,
                        phosh_notification_banner_new (notification));

    gtk_widget_show (GTK_WIDGET (priv->notification_banner));
  }
}


static gboolean
on_fade_out_timeout (PhoshShell *self)
{
  PhoshShellPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_SHELL (self), G_SOURCE_REMOVE);

  priv = phosh_shell_get_instance_private (self);

  /* kill all faders if we time out */
  g_clear_pointer (&priv->faders, g_ptr_array_unref);
  return G_SOURCE_REMOVE;
}


static gboolean
setup_idle_cb (PhoshShell *self)
{
  PhoshShellPrivate *priv = phosh_shell_get_instance_private (self);

  panels_create (self);
  /* Create background after panel since it needs the panel's size */
  priv->background_manager = phosh_background_manager_new ();

  g_signal_connect_object (priv->toplevel_manager,
                           "notify::num-toplevels",
                           G_CALLBACK(on_num_toplevels_changed),
                           self,
                           G_CONNECT_SWAPPED);
  on_num_toplevels_changed (self, NULL, priv->toplevel_manager);

  g_signal_connect_object (priv->toplevel_manager,
                           "toplevel-added",
                           G_CALLBACK(on_toplevel_added),
                           self,
                           G_CONNECT_SWAPPED);

  /* Screen saver manager needs lock screen manager */
  priv->screen_saver_manager = phosh_screen_saver_manager_get_default (
    priv->lockscreen_manager);

  priv->notify_manager = phosh_notify_manager_get_default ();
  g_signal_connect_object (priv->notify_manager,
                           "new-notification",
                           G_CALLBACK (on_new_notification),
                           self,
                           G_CONNECT_SWAPPED);

  priv->sensor_proxy_manager = phosh_sensor_proxy_manager_get_default_failable ();
  if (priv->sensor_proxy_manager) {
    priv->proximity = phosh_proximity_new (priv->sensor_proxy_manager,
                                           priv->lockscreen_manager);
    /* TODO: accelerometer */
  }

  phosh_session_register (PHOSH_APP_ID);

  priv->startup_finished = TRUE;

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
on_builtin_monitor_power_mode_changed (PhoshShell *self, GParamSpec *pspec, PhoshMonitor *monitor)
{
  PhoshMonitorPowerSaveMode mode;

  g_return_if_fail (PHOSH_IS_SHELL (self));
  g_return_if_fail (PHOSH_IS_MONITOR (monitor));

  g_object_get (monitor, "power-mode", &mode, NULL);
  if (mode == PHOSH_MONITOR_POWER_SAVE_MODE_OFF)
    phosh_shell_lock (self);
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

  if (phosh_monitor_is_builtin(priv->primary_monitor))
    priv->builtin_monitor = priv->primary_monitor;
  else
    priv->builtin_monitor = phosh_shell_get_builtin_monitor(self);

  gtk_icon_theme_add_resource_path (gtk_icon_theme_get_default (),
                                    "/sm/puri/phosh/icons");
  env_setup ();
  css_setup (self);
  type_setup ();

  priv->lockscreen_manager = phosh_lockscreen_manager_new ();
  priv->idle_manager = phosh_idle_manager_get_default();

  priv->toplevel_manager = phosh_toplevel_manager_new ();
  priv->faders = g_ptr_array_new_with_free_func ((GDestroyNotify) (gtk_widget_destroy));

  phosh_system_prompter_register ();
  priv->polkit_auth_agent = phosh_polkit_auth_agent_new ();

  priv->feedback_manager = phosh_feedback_manager_new ();

  if (priv->builtin_monitor) {
    g_signal_connect_swapped (
      priv->builtin_monitor,
      "notify::power-mode",
      G_CALLBACK(on_builtin_monitor_power_mode_changed),
      self);
  }

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
phosh_shell_get_builtin_monitor (PhoshShell *self)
{
  PhoshShellPrivate *priv;
  PhoshMonitor *monitor = NULL;

  g_return_val_if_fail (PHOSH_IS_SHELL (self), NULL);
  priv = phosh_shell_get_instance_private (self);

  if (priv->builtin_monitor)
    return priv->builtin_monitor;

  for (int i = 0; i < phosh_monitor_manager_get_num_monitors (priv->monitor_manager); i++) {
    monitor = phosh_monitor_manager_get_monitor (priv->monitor_manager, i);
    if (phosh_monitor_is_builtin (monitor))
      break;
  }

  if (!monitor)
    monitor = phosh_monitor_manager_get_monitor (priv->monitor_manager, 0);
  g_return_val_if_fail (monitor, NULL);

  return monitor;
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

PhoshToplevelManager *
phosh_shell_get_toplevel_manager (PhoshShell *self)
{
  PhoshShellPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_SHELL (self), NULL);
  priv = phosh_shell_get_instance_private (self);

  g_return_val_if_fail (PHOSH_IS_TOPLEVEL_MANAGER (priv->toplevel_manager), NULL);
  return priv->toplevel_manager;
}

PhoshFeedbackManager *
phosh_shell_get_feedback_manager (PhoshShell *self)
{
  PhoshShellPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_SHELL (self), NULL);
  priv = phosh_shell_get_instance_private (self);
  g_return_val_if_fail (PHOSH_IS_FEEDBACK_MANAGER (priv->feedback_manager), NULL);

  return priv->feedback_manager;
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
    h = mode->height / scale - PHOSH_PANEL_HEIGHT - PHOSH_HOME_BUTTON_HEIGHT;
    break;
  default:
    w = mode->height / scale;
    h = mode->width / scale - PHOSH_PANEL_HEIGHT - PHOSH_HOME_BUTTON_HEIGHT;
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

void
phosh_shell_fade_out (PhoshShell *self, guint timeout)
{
  PhoshShellPrivate *priv;
  PhoshWayland *wl = phosh_wayland_get_default ();
  PhoshMonitorManager *monitor_manager;

  g_debug ("Fading out...");
  g_return_if_fail (PHOSH_IS_SHELL (self));
  monitor_manager = phosh_shell_get_monitor_manager (self);
  g_return_if_fail (PHOSH_IS_MONITOR_MANAGER (monitor_manager));
  priv = phosh_shell_get_instance_private (self);

  for (int i = 0; i < phosh_monitor_manager_get_num_monitors (monitor_manager); i++) {
    PhoshFader *fader;
    PhoshMonitor *monitor = phosh_monitor_manager_get_monitor (monitor_manager, i);

    fader = phosh_fader_new (phosh_wayland_get_zwlr_layer_shell_v1 (wl),
                             monitor->wl_output);
    g_ptr_array_add (priv->faders, fader);
    gtk_widget_show (GTK_WIDGET (fader));
    if (timeout > 0)
      g_timeout_add_seconds (timeout, (GSourceFunc) on_fade_out_timeout, self);
  }
}

/**
 * phosh_shell_set_power_save:
 *
 * Enter power saving mode. This currently blanks all monitors.
 */
void
phosh_shell_enable_power_save (PhoshShell *self, gboolean enable)
{
  g_debug ("Entering power save mode");
  g_return_if_fail (PHOSH_IS_SHELL (self));

  /*
   * Locking the outputs instructs g-s-d to tell us to put
   * outputs into power save mode via org.gnome.Mutter.DisplayConfig
   */
  phosh_shell_set_locked(self, enable);

  /* TODO: other means of power saving */
}

/**
 * phosh_shell_started_by_display_manager:
 *
 * returns %TRUE if we were started from a
 * display manager. %FALSE otherwise.
 */
gboolean
phosh_shell_started_by_display_manager(PhoshShell *self)
{
  g_return_val_if_fail (PHOSH_IS_SHELL (self), FALSE);

  if (!g_strcmp0 (g_getenv ("GDMSESSION"), "phosh"))
    return TRUE;

  return FALSE;
}

/**
 * phosh_shell_is_startup_finished:
 *
 * returns %TRUE if the shell finished startup. %FALSE otherwise.
 */
gboolean
phosh_shell_is_startup_finished(PhoshShell *self)
{
  PhoshShellPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_SHELL (self), FALSE);
  priv = phosh_shell_get_instance_private (self);

  return priv->startup_finished;
}
