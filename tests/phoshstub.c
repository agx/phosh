/* Stubs so we don't need to run the shell */

#include <glib.h>

gpointer
phosh_shell_get_default ()
{
  return NULL;
}

void
phosh_shell_get_usable_area (gpointer *self, gint *x, gint *y, gint *width, gint *height)
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
