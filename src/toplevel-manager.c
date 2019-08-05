/*
 * Copyright (C) 2019 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Sebastian Krzyszkowiak <sebastian.krzyszkowiak@puri.sm>
 */

#define G_LOG_DOMAIN "phosh-toplevel-manager"

#include "toplevel-manager.h"
#include "toplevel.h"
#include "phosh-wayland.h"
#include "shell.h"
#include "util.h"
#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"

#include <gdk/gdkwayland.h>

/**
 * SECTION:phosh-toplevel-manager
 * @short_description: Tracks and interacts with toplevel surfaces
 * for window management purposes.
 * @Title: PhoshToplevelManager
 */

enum {
  SIGNAL_TOPLEVEL_ADDED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct _PhoshToplevelManager {
  GObject parent;
  GPtrArray *toplevels;
};

G_DEFINE_TYPE (PhoshToplevelManager, phosh_toplevel_manager, G_TYPE_OBJECT);


static void
on_toplevel_closed (PhoshToplevelManager *self, PhoshToplevel *toplevel)
{
  g_return_if_fail (PHOSH_IS_TOPLEVEL_MANAGER (self));
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));
  g_return_if_fail (self->toplevels);

  g_assert_true(g_ptr_array_remove (self->toplevels, toplevel));
}


static void
on_toplevel_configured (PhoshToplevelManager *self, GParamSpec *pspec, PhoshToplevel *toplevel)
{
  gboolean configured;
  g_return_if_fail (PHOSH_IS_TOPLEVEL_MANAGER (self));
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));
  g_return_if_fail (self->toplevels);

  configured = phosh_toplevel_is_configured (toplevel);

  if (configured && !g_ptr_array_find (self->toplevels, toplevel, NULL)) {
    g_ptr_array_add (self->toplevels, toplevel);
    g_signal_emit (self, signals[SIGNAL_TOPLEVEL_ADDED], 0, toplevel);
  }
}


static void
handle_zwlr_foreign_toplevel_manager_toplevel(
  void *data,
  struct zwlr_foreign_toplevel_manager_v1 *zwlr_foreign_toplevel_manager_v1,
  struct zwlr_foreign_toplevel_handle_v1 *handle)
{
  PhoshToplevelManager *self = data;
  PhoshToplevel *toplevel;
  g_return_if_fail (PHOSH_IS_TOPLEVEL_MANAGER (self));
  toplevel = phosh_toplevel_new_from_handle (handle);

  g_signal_connect_swapped (toplevel, "closed", G_CALLBACK (on_toplevel_closed), self);
  g_signal_connect_swapped (toplevel, "notify::configured", G_CALLBACK (on_toplevel_configured), self);

  g_debug ("Got toplevel %p", toplevel);
}


static void
handle_zwlr_foreign_toplevel_manager_finished(
  void *data,
  struct zwlr_foreign_toplevel_manager_v1 *zwlr_foreign_toplevel_manager_v1)
{
  g_debug ("wlr_foreign_toplevel_manager_finished");
}


static const struct zwlr_foreign_toplevel_manager_v1_listener zwlr_foreign_toplevel_manager_listener = {
  handle_zwlr_foreign_toplevel_manager_toplevel,
  handle_zwlr_foreign_toplevel_manager_finished,
};


static void
phosh_toplevel_manager_dispose (GObject *object)
{
  PhoshToplevelManager *self = PHOSH_TOPLEVEL_MANAGER (object);
  if (self->toplevels) {
    g_ptr_array_free(self->toplevels, TRUE);
    self->toplevels = NULL;
  }
  G_OBJECT_CLASS (phosh_toplevel_manager_parent_class)->dispose (object);
}


static void
phosh_toplevel_manager_class_init (PhoshToplevelManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = phosh_toplevel_manager_dispose;
  /**
   * PhoshToplevelManager::toplevel-added:
   * @manager: The #PhoshToplevelManager emitting the signal.
   * @toplevel: The #PhoshToplevel being added to the list.
   *
   * Emitted whenever a toplevel has been added to the list.
   */
  signals[SIGNAL_TOPLEVEL_ADDED] = g_signal_new (
    "toplevel-added",
    G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
    NULL, G_TYPE_NONE, 1, PHOSH_TYPE_TOPLEVEL);
}


static void
phosh_toplevel_manager_init (PhoshToplevelManager *self)
{
  struct zwlr_foreign_toplevel_manager_v1 *toplevel_manager;
  toplevel_manager = phosh_wayland_get_zwlr_foreign_toplevel_manager_v1 (
     phosh_wayland_get_default ());

  self->toplevels = g_ptr_array_new_with_free_func ((GDestroyNotify) (g_object_unref));

  if (!toplevel_manager) {
    g_warning ("Skipping app list due to missing wlr-foreign-toplevel-management protocol extension");
    return;
  }

  zwlr_foreign_toplevel_manager_v1_add_listener (toplevel_manager, &zwlr_foreign_toplevel_manager_listener, self);
}


PhoshToplevel *
phosh_toplevel_manager_get_toplevel (PhoshToplevelManager *self, guint num)
{
  g_return_val_if_fail (PHOSH_IS_TOPLEVEL_MANAGER (self), NULL);
  g_return_val_if_fail (self->toplevels, NULL);

  g_return_val_if_fail (num < self->toplevels->len, NULL);

  return g_ptr_array_index (self->toplevels, num);
}


guint
phosh_toplevel_manager_get_num_toplevels (PhoshToplevelManager *self)
{
  g_return_val_if_fail (PHOSH_IS_TOPLEVEL_MANAGER (self), 0);
  g_return_val_if_fail (self->toplevels, 0);

  return self->toplevels->len;
}


PhoshToplevelManager *
phosh_toplevel_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_TOPLEVEL_MANAGER, NULL);
}
