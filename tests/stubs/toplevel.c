/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Sebastian Krzyszkowiak <sebastian.krzyszkowiak@puri.sm>
 */

#include "toplevel.h"

enum {
  SIGNAL_CLOSED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct _PhoshToplevel {
  GObject parent;
};

G_DEFINE_TYPE (PhoshToplevel, phosh_toplevel, G_TYPE_OBJECT);

static void
phosh_toplevel_class_init (PhoshToplevelClass *klass)
{
  signals[SIGNAL_CLOSED] = g_signal_new (
    "closed",
    G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
    NULL, G_TYPE_NONE, 0);
}


static void
phosh_toplevel_init (PhoshToplevel *self)
{
}


const char *
phosh_toplevel_get_title (PhoshToplevel *self) {
  g_return_val_if_fail (PHOSH_IS_TOPLEVEL (self), NULL);
  return "Mock Title";
}


const char *
phosh_toplevel_get_app_id (PhoshToplevel *self) {
  g_return_val_if_fail (PHOSH_IS_TOPLEVEL (self), NULL);
  return "mock.app.id";
}


gboolean
phosh_toplevel_is_configured (PhoshToplevel *self) {
  g_return_val_if_fail (PHOSH_IS_TOPLEVEL (self), FALSE);
  return FALSE;
}


gboolean
phosh_toplevel_is_activated (PhoshToplevel *self) {
  g_return_val_if_fail (PHOSH_IS_TOPLEVEL (self), FALSE);
  return FALSE;
}


gboolean
phosh_toplevel_is_maximized (PhoshToplevel *self)
{
  g_return_val_if_fail (PHOSH_IS_TOPLEVEL (self), FALSE);
  return FALSE;
}


gboolean
phosh_toplevel_is_fullscreen (PhoshToplevel *self)
{
  g_return_val_if_fail (PHOSH_IS_TOPLEVEL (self), FALSE);
  return FALSE;
}

void
phosh_toplevel_activate (PhoshToplevel *self, struct wl_seat *seat) {
  g_return_if_fail (PHOSH_IS_TOPLEVEL (self));
}


void
phosh_toplevel_close (PhoshToplevel *self) {
  g_return_if_fail (PHOSH_IS_TOPLEVEL (self));
}


PhoshToplevel *
phosh_toplevel_new_from_handle (struct zwlr_foreign_toplevel_handle_v1 *handle)
{
  return g_object_new (PHOSH_TYPE_TOPLEVEL, "handle", handle, NULL);
}
