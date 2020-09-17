/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-background-manager"

#include "background-manager.h"
#include "background.h"
#include "monitor/monitor.h"
#include "phosh-wayland.h"
#include "shell.h"
#include "util.h"

#include <gdk/gdkwayland.h>

/**
 * SECTION:background-manager
 * @short_description: Tracks screen related events and updates
 * backgrounds accordingly.
 * @Title: PhoshBackgroundManager
 */

struct _PhoshBackgroundManager {
  GObject parent;

  PhoshMonitor *primary_monitor;
  GHashTable   *backgrounds;
};

G_DEFINE_TYPE (PhoshBackgroundManager, phosh_background_manager, G_TYPE_OBJECT);


static PhoshBackground *
create_background_for_monitor (PhoshBackgroundManager *self, PhoshMonitor *monitor)
{
  PhoshWayland *wl = phosh_wayland_get_default();
  PhoshBackground *background;

  background = g_object_ref_sink(PHOSH_BACKGROUND (phosh_background_new (
                                                     phosh_wayland_get_zwlr_layer_shell_v1(wl),
                                                     monitor->wl_output,
                                                     MAX(1, monitor->scale),
                                                     monitor == self->primary_monitor)));
  g_hash_table_insert (self->backgrounds,
                       g_object_ref (monitor),
                       background);
  return background;
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
on_monitor_configured (PhoshBackgroundManager *self,
                       PhoshMonitor           *monitor)
{
  PhoshBackground *background;

  g_return_if_fail (PHOSH_IS_MONITOR (monitor));
  g_debug ("Monitor %p (%s) configured", monitor, monitor->name);

  background = g_hash_table_lookup (self->backgrounds, monitor);
  g_return_if_fail (background);

  phosh_background_set_scale (background, monitor->scale);
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

  create_background_for_monitor (self, monitor);

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

  if (monitor) {
    g_clear_object (&self->primary_monitor);
    self->primary_monitor = g_object_ref (monitor);
    background = g_hash_table_lookup (self->backgrounds, monitor);
    if (background)
      phosh_background_set_primary (background, TRUE);
  }
}


static void
phosh_background_manager_dispose (GObject *object)
{
  PhoshBackgroundManager *self = PHOSH_BACKGROUND_MANAGER (object);

  g_hash_table_destroy (self->backgrounds);
  g_clear_object (&self->primary_monitor);
  G_OBJECT_CLASS (phosh_background_manager_parent_class)->dispose (object);
}


static void
phosh_background_manager_constructed (GObject *object)
{
  PhoshBackgroundManager *self = PHOSH_BACKGROUND_MANAGER (object);
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshMonitorManager *monitor_manager = phosh_shell_get_monitor_manager (shell);

  G_OBJECT_CLASS (phosh_background_manager_parent_class)->constructed (object);

  /* Listen for monitor changes */
  g_signal_connect_object (monitor_manager, "monitor-added",
                           G_CALLBACK (on_monitor_added),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (monitor_manager, "monitor-removed",
                           G_CALLBACK (on_monitor_removed),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (shell, "notify::primary-monitor",
                           G_CALLBACK (on_primary_monitor_changed),
                           self,
                           G_CONNECT_SWAPPED);
  self->primary_monitor = g_object_ref (phosh_shell_get_primary_monitor (shell));

  /* catch up with monitors already present */
  for (int i = 0; i < phosh_monitor_manager_get_num_monitors (monitor_manager); i++) {
    PhoshMonitor *monitor = phosh_monitor_manager_get_monitor (monitor_manager, i);

    on_monitor_added (self, monitor, NULL);
  }
}


static void
phosh_background_manager_class_init (PhoshBackgroundManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_background_manager_constructed;
  object_class->dispose = phosh_background_manager_dispose;
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
