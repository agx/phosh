/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Florian Loers <florianloers@mailbox.org>
 */

#define G_LOG_DOMAIN "phosh-run-command-dialog"

#include "run-command-dialog.h"
#include "phosh-config.h"

/**
 * PhoshRunCommandDialog:
 *
 * A modal dialog to run commands from
 *
 * The #PhoshRunCommandDialog is used to run commands (e.g. via Alt+F2).
 */

enum {
  SUBMITTED,
  CANCELLED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = {0};

typedef struct _PhoshRunCommandDialog {
  PhoshSystemModalDialog parent;
  GtkWidget *entry_command;
  GtkLabel  *lbl_description;
} PhoshRunCommandDialog;

G_DEFINE_TYPE (PhoshRunCommandDialog, phosh_run_command_dialog, PHOSH_TYPE_SYSTEM_MODAL_DIALOG)

static void
on_activated (PhoshRunCommandDialog *self, GtkEntry *entry)
{
  const char *command = gtk_entry_get_text (entry);

  g_signal_emit (self, signals[SUBMITTED], 0, command);
}

static void
on_run_command_dialog_canceled (PhoshRunCommandDialog *self)
{
  g_return_if_fail (PHOSH_IS_RUN_COMMAND_DIALOG (self));
  g_signal_emit (self, signals[CANCELLED], 0);
}

static void
phosh_run_command_dialog_finalize (GObject *obj)
{
  PhoshRunCommandDialog *self = PHOSH_RUN_COMMAND_DIALOG (obj);

  g_free (self->entry_command);

  G_OBJECT_CLASS (phosh_run_command_dialog_parent_class)->finalize (obj);
}


static void
on_text_changed (PhoshRunCommandDialog *self)
{
  g_return_if_fail (PHOSH_IS_RUN_COMMAND_DIALOG (self));

  /* Make sure we reset any error message */
  phosh_run_command_dialog_set_message (self, NULL);
}


static void
phosh_run_command_dialog_class_init (PhoshRunCommandDialogClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_run_command_dialog_finalize;

  /**
   * RunCommandDialog:submitted:
   *
   * The user submitted a command to run
   */
  signals[SUBMITTED] = g_signal_new ("submitted",
                                     G_TYPE_FROM_CLASS (klass),
                                     G_SIGNAL_RUN_LAST,
                                     0, NULL, NULL, NULL,
                                     G_TYPE_NONE, 1, G_TYPE_STRING);
  /**
   * RunCommandDialog:cancelled:
   *
   * The user cancelled the dialog
   */
  signals[CANCELLED] = g_signal_new ("cancelled",
                                     G_TYPE_FROM_CLASS (klass),
                                     G_SIGNAL_RUN_LAST,
                                     0, NULL, NULL, NULL,
                                     G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/phosh/ui/run-command-dialog.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshRunCommandDialog, entry_command);
  gtk_widget_class_bind_template_child (widget_class, PhoshRunCommandDialog, lbl_description);
  gtk_widget_class_bind_template_callback (widget_class, on_activated);
  gtk_widget_class_bind_template_callback (widget_class, on_run_command_dialog_canceled);
}

static void
phosh_run_command_dialog_init (PhoshRunCommandDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  phosh_run_command_dialog_set_message (self, NULL);
  g_signal_connect_swapped (self->entry_command, "notify::text", G_CALLBACK (on_text_changed), self);
}

GtkWidget *
phosh_run_command_dialog_new (void)
{
  return g_object_new (PHOSH_TYPE_RUN_COMMAND_DIALOG, NULL);
}

void
phosh_run_command_dialog_set_message (PhoshRunCommandDialog *self,
                                      const gchar           *message)
{
  if (message == NULL)
    message = _("Press ESC to close");

  gtk_label_set_label (self->lbl_description, message);
}
