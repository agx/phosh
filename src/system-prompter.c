/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-system-prompter"

#include "config.h"

#include "system-prompt.h"
#include "system-prompter.h"
#include "shell.h"
#include "phosh-wayland.h"

/**
 * SECTION:system-prompter
 * @short_description: Manages system prompter registration
 * @Title: PhoshSystemPrompter
 *
 * The PhoshSystemPrompter is responsible for displaying system
 * wide modal #PhoshSystemPrompt dialogs
 */
static GcrSystemPrompter *_prompter;
static ulong owner_id;
static gboolean registered_prompter;
static gboolean acquired_prompter;


static GcrPrompt *
new_prompt_cb (GcrSystemPrompter *prompter,
               gpointer user_data)
{
  PhoshWayland *wl = phosh_wayland_get_default ();
  PhoshShell *shell = phosh_shell_get_default ();
  GtkWidget *prompt;

  g_debug ("Building new system prompt");
  g_return_val_if_fail (GCR_IS_SYSTEM_PROMPTER (prompter), NULL);

  prompt = phosh_system_prompt_new (phosh_wayland_get_zwlr_layer_shell_v1 (wl),
                                    phosh_shell_get_primary_monitor (shell)->wl_output);
  gtk_widget_show (prompt);
  return GCR_PROMPT (prompt);
}


static void
on_bus_acquired (GDBusConnection *connection,
                 const char *name,
                 gpointer user_data)
{
  g_debug ("bus acquired for %s", name);

  if (!registered_prompter) {
    gcr_system_prompter_register (_prompter, connection);
    g_debug ("registered prompter");
  }
  registered_prompter = TRUE;
}


static void
on_name_lost (GDBusConnection *connection,
              const char *name,
              gpointer user_data)
{
  g_debug ("lost name: %s", name);

  /* Called like so when no connection can be made */
  if (connection == NULL) {
    g_warning ("couldn't connect to session bus");
    phosh_system_prompter_unregister ();
  }
}


static void
on_name_acquired (GDBusConnection *connection,
                  const char *name,
                  gpointer user_data)
{
  g_debug ("acquired name: %s", name);
  acquired_prompter = TRUE;
}


GcrSystemPrompter *
phosh_system_prompter_register (void)
{
  _prompter = gcr_system_prompter_new (GCR_SYSTEM_PROMPTER_SINGLE, 0);

  g_signal_connect (_prompter, "new-prompt",
                    G_CALLBACK (new_prompt_cb), NULL);

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             "org.gnome.keyring.SystemPrompter",
                             G_BUS_NAME_OWNER_FLAGS_REPLACE,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             NULL,
                             NULL);
  return _prompter;
}


void
phosh_system_prompter_unregister(void)
{
  if (_prompter) {
    gcr_system_prompter_unregister (_prompter, TRUE);
    _prompter = NULL;
  }

  if (acquired_prompter) {
    g_bus_unown_name (owner_id);
    owner_id = 0;
    acquired_prompter = FALSE;
  }
}
