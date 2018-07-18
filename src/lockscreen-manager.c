/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-lockscreen-manager"

#include "lockscreen-manager.h"
#include "lockscreen.h"
#include "lockshield.h"
#include "phosh.h"
#include "monitor-manager.h"
#include "monitor/monitor.h"
#include "phosh-wayland.h"

#include <gdk/gdkwayland.h>

/* FIXME: use org.gnome.desktop.session.idle-delay */
#define LOCKSCREEN_TIMEOUT 300 * 1000

enum {
  PHOSH_LOCKSCREEN_MANAGER_PROP_0,
  PHOSH_LOCKSCREEN_MANAGER_PROP_LOCKED,
  PHOSH_LOCKSCREEN_MANAGER_PROP_LAST_PROP
};
static GParamSpec *props[PHOSH_LOCKSCREEN_MANAGER_PROP_LAST_PROP];


struct elem {
  GtkWidget *window;
  struct wl_surface *wl_surface;
  struct zwlr_layer_surface_v1 *layer_surface;
};


typedef struct {
  GObject parent;

  struct elem *lockscreen;   /* phone display lock screen */
  GPtrArray *shields;        /* other outputs */
  gulong unlock_handler_id;
  struct org_kde_kwin_idle_timeout *lock_timer;
  struct zwlr_input_inhibitor_v1 *input_inhibitor;
  gboolean locked;
} PhoshLockscreenManagerPrivate;


typedef struct _PhoshLockscreenManager {
  GObject parent;
} PhoshLockscreenManager;


G_DEFINE_TYPE_WITH_PRIVATE (PhoshLockscreenManager, phosh_lockscreen_manager, G_TYPE_OBJECT)

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


static struct zwlr_layer_surface_v1_listener layer_surface_listener = {
  .configure = layer_surface_configure,
  .closed = layer_surface_closed,
};


static void
lockscreen_unlock_cb (PhoshLockscreenManager *self, PhoshLockscreen *window)
{
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);

  g_return_if_fail (PHOSH_IS_LOCKSCREEN (window));
  g_return_if_fail (window == PHOSH_LOCKSCREEN (priv->lockscreen->window));

  if (priv->unlock_handler_id) {
    g_signal_handler_disconnect (window, priv->unlock_handler_id);
    priv->unlock_handler_id = 0;
  }
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
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_LOCKSCREEN_MANAGER_PROP_LOCKED]);
}


static void
lockscreen_lock (PhoshLockscreenManager *self)
{
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);
  GdkWindow *gdk_window;
  struct elem *lockscreen;
  PhoshMonitor *monitor;
  PhoshWayland *wl = phosh_wayland_get_default ();
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshMonitorManager *monitor_manager = phosh_shell_get_monitor_manager (shell);

  monitor = phosh_shell_get_primary_monitor (shell);
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
  for (int i = 1; i < phosh_monitor_manager_get_num_monitors (monitor_manager); i++) {
    monitor = phosh_monitor_manager_get_monitor (monitor_manager, i);
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
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_LOCKSCREEN_MANAGER_PROP_LOCKED]);
}


static void lock_idle_cb(void* data, struct org_kde_kwin_idle_timeout *timer)
{
  PhoshLockscreenManager *self = data;

  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (data));
  phosh_lockscreen_manager_set_locked (self, TRUE);
}


static void lock_resume_cb(void* data, struct org_kde_kwin_idle_timeout *timer)
{
}


static const struct org_kde_kwin_idle_timeout_listener idle_timer_listener = {
  .idle = lock_idle_cb,
  .resumed = lock_resume_cb,
};


static void
phosh_lockscreen_manager_set_property (GObject *object,
                          guint property_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
  PhoshLockscreenManager *self = PHOSH_LOCKSCREEN_MANAGER (object);

  switch (property_id) {
  case PHOSH_LOCKSCREEN_MANAGER_PROP_LOCKED:
    phosh_lockscreen_manager_set_locked (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_lockscreen_manager_get_property (GObject *object,
                          guint property_id,
                          GValue *value,
                          GParamSpec *pspec)
{
  PhoshLockscreenManager *self = PHOSH_LOCKSCREEN_MANAGER (object);
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private(self);

  switch (property_id) {
  case PHOSH_LOCKSCREEN_MANAGER_PROP_LOCKED:
    g_value_set_boolean (value, priv->locked);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_lockscreen_manager_dispose (GObject *object)
{
  PhoshLockscreenManager *self = PHOSH_LOCKSCREEN_MANAGER (object);
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);

  if (priv->shields) {
    g_ptr_array_unref (priv->shields);
    priv->shields = NULL;
  }

  if (priv->lockscreen) {
    if (priv->unlock_handler_id) {
      g_signal_handler_disconnect (priv->lockscreen->window, priv->unlock_handler_id);
      priv->unlock_handler_id = 0;
    }
    gtk_widget_destroy (priv->lockscreen->window);
    priv->lockscreen = NULL;
  }

  G_OBJECT_CLASS (phosh_lockscreen_manager_parent_class)->dispose (object);
}


static void
phosh_lockscreen_manager_constructed (GObject *object)
{
  PhoshLockscreenManager *self = PHOSH_LOCKSCREEN_MANAGER (object);
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);
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


static void
phosh_lockscreen_manager_class_init (PhoshLockscreenManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_lockscreen_manager_constructed;
  object_class->dispose = phosh_lockscreen_manager_dispose;

  object_class->set_property = phosh_lockscreen_manager_set_property;
  object_class->get_property = phosh_lockscreen_manager_get_property;

  props[PHOSH_LOCKSCREEN_MANAGER_PROP_LOCKED] =
    g_param_spec_boolean ("locked",
                          "Locked",
                          "Whether the screen is locked",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_properties (object_class, PHOSH_LOCKSCREEN_MANAGER_PROP_LAST_PROP, props);
}


static void
phosh_lockscreen_manager_init (PhoshLockscreenManager *self)
{
}


PhoshLockscreenManager *
phosh_lockscreen_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_LOCKSCREEN_MANAGER, NULL);
}


void
phosh_lockscreen_manager_set_locked (PhoshLockscreenManager *self, gboolean state)
{
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);

  g_return_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self));
  if (state == priv->locked)
    return;

  if (state)
    lockscreen_lock (self);
  else
    lockscreen_unlock_cb (self, PHOSH_LOCKSCREEN (priv->lockscreen->window));
}


gboolean
phosh_lockscreen_manager_get_locked (PhoshLockscreenManager *self)
{
  PhoshLockscreenManagerPrivate *priv = phosh_lockscreen_manager_get_instance_private (self);

  g_return_val_if_fail (PHOSH_IS_LOCKSCREEN_MANAGER (self), FALSE);
  return priv->locked;
}
