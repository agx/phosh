/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Florian Loers <florianloers@mailbox.org>
 */

#define G_LOG_DOMAIN "phosh-run-command-manager"

#include "run-command-manager.h"
#include "run-command-dialog.h"
#include "shell.h"
#include "util.h"

#define KEYBINDINGS_SCHEMA_ID_DESKTOP_WM "org.gnome.desktop.wm.keybindings"
#define KEYBINDING_KEY_RUN_DIALOG "panel-run-dialog"

/**
 * PhoshRunCommandManager:
 *
 * Handles the run-command-dialog
 *
 * The interface is responsible to handle the non-ui parts of a
 * #PhoshRunCommandDialog.
 */

typedef struct _PhoshRunCommandManager {
  GObject parent;
  PhoshRunCommandDialog *dialog;
  GStrv action_names;
  GSettings *settings;
} PhoshRunCommandManager;

G_DEFINE_TYPE (PhoshRunCommandManager, phosh_run_command_manager, G_TYPE_OBJECT)

static void
cleanup_child_process (GPid pid, gint status, void *user_data)
{
  g_autoptr (GError) error = NULL;

  g_spawn_close_pid (pid);

  if (!g_spawn_check_wait_status (status, &error))
    g_warning ("Could not end child process: %s\n", error->message);
}

static gboolean
run_command (char *command)
{
  GPid child_pid;
  g_auto (GStrv) argv = NULL;
  g_autoptr (GError) error = NULL;

  if (!g_shell_parse_argv (command, NULL, &argv, &error)) {
    g_warning ("Could not parse command: %s\n", error->message);
    return FALSE;
  }
  if (g_spawn_async (NULL,
                      argv,
                      NULL,
                      G_SPAWN_DO_NOT_REAP_CHILD |
                      G_SPAWN_SEARCH_PATH |
                      G_SPAWN_STDOUT_TO_DEV_NULL |
                      G_SPAWN_STDERR_TO_DEV_NULL,
                      NULL,
                      NULL,
                      &child_pid,
                      &error)) {
    g_child_watch_add (child_pid, cleanup_child_process, NULL);
    return TRUE;
  }

  g_warning ("Could not run command: %s\n", error->message);
  return FALSE;
}

static void
on_run_command_dialog_submitted (PhoshRunCommandManager *self, char *command)
{
  g_autofree char *msg = NULL;

  g_return_if_fail (PHOSH_IS_RUN_COMMAND_DIALOG (self->dialog));
  g_return_if_fail (command);

  if (run_command (command)) {
    g_clear_pointer ((PhoshSystemModalDialog**)&self->dialog, phosh_system_modal_dialog_close);
  } else {
    msg = g_strdup_printf (_("Running '%s' failed"), command);
    phosh_run_command_dialog_set_message (self->dialog, msg);
  }
}

static void
on_run_command_dialog_cancelled (PhoshRunCommandManager *self)
{
  g_return_if_fail (PHOSH_IS_RUN_COMMAND_DIALOG (self->dialog));

  g_clear_pointer ((PhoshSystemModalDialog**)&self->dialog, phosh_system_modal_dialog_close);
}

static void
show_run_command_dialog (GSimpleAction *action, GVariant *param, gpointer data)
{
  GtkWidget *dialog;
  PhoshRunCommandManager *self = PHOSH_RUN_COMMAND_MANAGER (data);

  if (self->dialog)
    return;
  dialog = phosh_run_command_dialog_new ();
  self->dialog = PHOSH_RUN_COMMAND_DIALOG (dialog);
  gtk_widget_show (GTK_WIDGET (self->dialog));
  g_object_connect (self->dialog,
                    "swapped-object-signal::submitted", G_CALLBACK (on_run_command_dialog_submitted), self,
                    "swapped-object-signal::cancelled", G_CALLBACK (on_run_command_dialog_cancelled), self,
                    NULL);
}

static void
add_keybindings (PhoshRunCommandManager *self)
{
  g_auto (GStrv) bindings = NULL;
  g_autoptr (GArray) actions = g_array_new (FALSE, TRUE, sizeof (GActionEntry));

  bindings = g_settings_get_strv (self->settings, KEYBINDING_KEY_RUN_DIALOG);
  for (int i = 0; i < g_strv_length (bindings); i++) {
    GActionEntry entry = { .name = bindings[i], .activate = show_run_command_dialog };
    g_array_append_val (actions, entry);
  }
  phosh_shell_add_global_keyboard_action_entries (phosh_shell_get_default (),
                                                  (GActionEntry *)actions->data,
                                                  actions->len,
                                                  self);
  self->action_names = g_steal_pointer (&bindings);
}

static void
on_keybindings_changed (PhoshRunCommandManager *self)
{
  g_debug ("Updating keybindings in run-command-manager");
  phosh_shell_remove_global_keyboard_action_entries (phosh_shell_get_default (),
                                                     self->action_names);
  g_clear_pointer (&self->action_names, g_strfreev);
  add_keybindings (self);
}

static void
phosh_run_command_manager_constructed (GObject *object)
{
  PhoshRunCommandManager *self = PHOSH_RUN_COMMAND_MANAGER (object);

  G_OBJECT_CLASS (phosh_run_command_manager_parent_class)->constructed (object);
  g_signal_connect_swapped (self->settings,
                            "changed::" KEYBINDING_KEY_RUN_DIALOG,
                            G_CALLBACK (on_keybindings_changed),
                            self);
  add_keybindings (self);
}

static void
phosh_run_command_manager_dispose (GObject *object)
{
  PhoshRunCommandManager *self = PHOSH_RUN_COMMAND_MANAGER (object);

  g_clear_pointer (&self->dialog, phosh_cp_widget_destroy);
  g_clear_pointer (&self->action_names, g_strfreev);
  g_clear_object (&self->settings);
  G_OBJECT_CLASS (phosh_run_command_manager_parent_class)->dispose (object);
}

static void
phosh_run_command_manager_class_init (PhoshRunCommandManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_run_command_manager_constructed;
  object_class->dispose = phosh_run_command_manager_dispose;
}

static void
phosh_run_command_manager_init (PhoshRunCommandManager *self)
{
  self->settings = g_settings_new (KEYBINDINGS_SCHEMA_ID_DESKTOP_WM);
}

PhoshRunCommandManager *
phosh_run_command_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_RUN_COMMAND_MANAGER, NULL);
}
