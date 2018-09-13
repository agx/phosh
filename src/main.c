/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-main"

#include "config.h"

#include "phosh.h"
#include "phosh-wayland.h"

#include <glib/gi18n.h>
#include <glib-unix.h>


static gboolean
sigterm_cb (gpointer unused)
{
  g_debug ("Cleaning up");
  gtk_main_quit ();
  return FALSE;
}


int main(int argc, char *argv[])
{
  g_autoptr(GSource) sigterm = NULL;
  GMainContext *context = NULL;
  g_autoptr(GOptionContext) opt_context = NULL;
  GError *err = NULL;
  gboolean unlocked = FALSE;
  g_autoptr(PhoshWayland) wl = NULL;
  g_autoptr(PhoshShell) shell = NULL;

  const GOptionEntry options [] = {
    {"unlocked", 'U', 0, G_OPTION_ARG_NONE, &unlocked,
     "Don't start with screen locked", NULL},
    { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
  };

  opt_context = g_option_context_new ("- A phone graphical shell");
  g_option_context_add_main_entries (opt_context, options, NULL);
  g_option_context_add_group (opt_context, gtk_get_option_group (TRUE));
  g_option_context_parse (opt_context, &argc, &argv, &err);

  textdomain (GETTEXT_PACKAGE);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  gtk_init (&argc, &argv);

  sigterm = g_unix_signal_source_new (SIGTERM);
  context = g_main_context_default ();
  g_source_set_callback (sigterm, sigterm_cb, NULL, NULL);
  g_source_attach (sigterm, context);

  wl = phosh_wayland_get_default ();
  shell = phosh_shell_get_default ();
  if (!unlocked)
    phosh_shell_lock (shell);

  gtk_main ();

  return EXIT_SUCCESS;
}
