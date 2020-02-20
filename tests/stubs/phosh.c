/* Stubs so we don't need to run the shell */

#include <glib.h>
#include "phosh-wayland.h"
#include "shell.h"

PhoshToplevelManager *toplevel_manager = NULL;

PhoshShell*
phosh_shell_get_default (void)
{
  return NULL;
}

void
phosh_shell_get_usable_area (PhoshShell *self,  gint *x, gint *y, gint *width, gint *height)
{
  if (x)
    *x = 16;
  if (y)
    *y = 32;
  if (width)
    *width = 64;
  if (height)
    *height = 128;
  return;
}


PhoshWayland *
phosh_wayland_get_default (void)
{
  return NULL;
}

struct phosh_private*
phosh_wayland_get_phosh_private (PhoshWayland *self)
{
  return NULL;
}

struct zwlr_foreign_toplevel_manager_v1*
phosh_wayland_get_zwlr_foreign_toplevel_manager_v1 (PhoshWayland *self)
{
  return NULL;
}

struct zwlr_layer_shell_v1 *
phosh_wayland_get_zwlr_layer_shell_v1 (PhoshWayland *self)
{
  return NULL;
}

PhoshMonitor *
phosh_shell_get_primary_monitor (PhoshShell *self)
{
  return NULL;
}

PhoshToplevelManager*
phosh_shell_get_toplevel_manager (PhoshShell *self)
{
  return g_object_new (PHOSH_TYPE_TOPLEVEL_MANAGER, NULL);
}

struct wl_seat*
phosh_wayland_get_wl_seat (PhoshWayland *self)
{
  return NULL;
}
