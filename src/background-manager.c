/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
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
 * SECTION:phosh-background-manager
 * @short_description: Tracks screen related events and updates
 * backgrounds accordingly.
 * @Title: PhoshBackgroundManager
 */

struct _PhoshBackgroundManager {
  GObject parent;
  GHashTable *backgrounds;
};

G_DEFINE_TYPE (PhoshBackgroundManager, phosh_background_manager, G_TYPE_OBJECT);


static PhoshBackground *
create_background_for_monitor (PhoshBackgroundManager *self, PhoshMonitor *monitor)
{
 PhoshShell *shell = phosh_shell_get_default ();
  PhoshWayland *wl = phosh_wayland_get_default();
  PhoshMonitor *primary_monitor;
  PhoshBackground *background;

  primary_monitor = phosh_shell_get_primary_monitor (shell);
  background = g_object_ref_sink(PHOSH_BACKGROUND (phosh_background_new (
                                                     phosh_wayland_get_zwlr_layer_shell_v1(wl),
                                                     monitor->wl_output,
                                                     monitor->width / monitor->scale,
                                                     monitor->height / monitor->scale,
                                                     monitor == primary_monitor)));
  g_hash_table_insert (self->backgrounds,
                       g_object_ref (monitor),
                       background);
  gtk_widget_show (GTK_WIDGET (background));
  return background;
}


static void
create_all_backgrounds (PhoshBackgroundManager *self)
{
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshMonitorManager *monitor_manager = phosh_shell_get_monitor_manager (shell);

  g_hash_table_remove_all (self->backgrounds);

  for (int i = 0; i < phosh_monitor_manager_get_num_monitors (monitor_manager); i++) {
    PhoshMonitor *monitor = phosh_monitor_manager_get_monitor (monitor_manager, i);

    create_background_for_monitor (self, monitor);
  }
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
                       PhoshMonitor *monitor)
{
  create_background_for_monitor (self, monitor);

  g_signal_handlers_disconnect_by_func (monitor, on_monitor_configured, self);
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
}


static void
on_primary_monitor_changed (PhoshBackgroundManager *self,
                            GParamSpec *pspec,
                            PhoshShell *shell)
{
  g_return_if_fail (PHOSH_IS_BACKGROUND_MANAGER (self));
  g_return_if_fail (PHOSH_IS_SHELL (shell));

  create_all_backgrounds (self);
}


static void
phosh_background_manager_dispose (GObject *object)
{
  PhoshBackgroundManager *self = PHOSH_BACKGROUND_MANAGER (object);

  g_hash_table_destroy (self->backgrounds);
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

  create_all_backgrounds (self);
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
