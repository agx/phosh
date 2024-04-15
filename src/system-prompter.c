/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-system-prompter"

#include "phosh-config.h"

#include "system-prompt.h"
#include "system-prompter.h"
#include "shell.h"
#include "phosh-wayland.h"

/**
 * PhoshSystemPrompter:
 *
 * Manages system prompter registration
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
  GtkWidget *prompt;

  g_debug ("Building new system prompt");
  g_return_val_if_fail (GCR_IS_SYSTEM_PROMPTER (prompter), NULL);

  prompt = phosh_system_prompt_new ();

  /* Show widget when not locked and keep that in sync */
  g_object_bind_property (phosh_shell_get_default (), "locked",
                          prompt, "visible",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

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
    registered_prompter = FALSE;
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

/**
 * phosh_system_prompter_register:
 *
 * Register the system prompter
 *
 * Returns:(transfer none): The system prompter
 */
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
    if (registered_prompter) {
      gcr_system_prompter_unregister (_prompter, TRUE);
      registered_prompter = FALSE;
    }
    g_clear_object (&_prompter);
  }

  if (owner_id) {
    g_bus_unown_name (owner_id);
    owner_id = 0;
  }

  acquired_prompter = FALSE;
}
