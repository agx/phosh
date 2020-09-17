/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-main"

#include "config.h"

#include "shell.h"
#include "phosh-wayland.h"

#include <handy.h>

#include <glib/gi18n.h>
#include <glib-unix.h>


static gboolean
quit (gpointer unused)
{
  g_debug ("Cleaning up");
  gtk_main_quit ();

  return G_SOURCE_REMOVE;
}


static gboolean
on_shutdown_signal (gpointer unused)
{
  phosh_shell_fade_out (phosh_shell_get_default (), 0);
  g_timeout_add_seconds (2, (GSourceFunc)quit, NULL);

  return FALSE;
}


static void
print_version (void)
{
  printf ("Phosh %s - A Wayland shell for mobile devices\n", PHOSH_VERSION);
  exit (0);
}


int main(int argc, char *argv[])
{
  g_autoptr(GSource) sigterm = NULL;
  g_autoptr(GOptionContext) opt_context = NULL;
  GError *err = NULL;
  gboolean unlocked = FALSE, locked = FALSE, version = FALSE;
  g_autoptr(PhoshWayland) wl = NULL;
  g_autoptr(PhoshShell) shell = NULL;

  const GOptionEntry options [] = {
    {"unlocked", 'U', 0, G_OPTION_ARG_NONE, &unlocked,
     "Don't start with screen locked", NULL},
    {"locked", 'L', 0, G_OPTION_ARG_NONE, &locked,
     "Start with screen locked, no matter what", NULL},
    {"version", 0, 0, G_OPTION_ARG_NONE, &version,
     "Show version information", NULL},
    { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
  };

  opt_context = g_option_context_new ("- A phone graphical shell");
  g_option_context_add_main_entries (opt_context, options, NULL);
  g_option_context_add_group (opt_context, gtk_get_option_group (TRUE));
  if (!g_option_context_parse (opt_context, &argc, &argv, &err)) {
    g_warning ("%s", err->message);
    g_clear_error (&err);
    return 1;
  }

  if (version) {
    print_version ();
  }

  textdomain (GETTEXT_PACKAGE);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  gtk_init (&argc, &argv);
  hdy_init ();

  g_unix_signal_add (SIGTERM, on_shutdown_signal, NULL);
  g_unix_signal_add (SIGINT, on_shutdown_signal, NULL);

  wl = phosh_wayland_get_default ();
  shell = phosh_shell_get_default ();
  if (!(unlocked || phosh_shell_started_by_display_manager(shell)) || locked)
    phosh_shell_lock (shell);

  gtk_main ();

  return EXIT_SUCCESS;
}
