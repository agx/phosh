/* Stubs so we don't need to run the shell */

#include <glib.h>
#include "phosh-wayland.h"
#include "phosh.h"

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
