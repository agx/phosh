/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Florian Loers <florianloers@mailbox.org>
 */

#define G_LOG_DOMAIN "phosh-run-command-dialog"

#include "run-command-dialog.h"
#include "config.h"

/**
 * SECTION:run-command-dialog
 * @short_description: A modal dialog to run commands from
 * @Title: PhoshRunCommandDialog
 *
 * The #PhoshRunCommandDialog is used to run commands (e.g. via Alt+F2).
 */

enum {
  DONE,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = {0};

typedef struct _PhoshRunCommandDialog {
  PhoshSystemModalDialog parent;
  GtkWidget *entry_command;
} PhoshRunCommandDialog;

G_DEFINE_TYPE (PhoshRunCommandDialog, phosh_run_command_dialog, PHOSH_TYPE_SYSTEM_MODAL_DIALOG)

static gboolean
run_command_dialog_key_press_event_cb (PhoshRunCommandDialog *self, GdkEventKey *event)
{
  if (event->keyval == GDK_KEY_Return) {
    GtkEntry *entry = GTK_ENTRY (self->entry_command);
    const char *command = gtk_entry_get_text (entry);
    g_signal_emit (self, signals[DONE], 0, FALSE, command);
    return TRUE;
  }
  return FALSE;
}

static void
run_command_dialog_canceled_event_cb (PhoshRunCommandDialog *self)
{
  g_return_if_fail (PHOSH_IS_RUN_COMMAND_DIALOG (self));
  g_signal_emit (self, signals[DONE], 0, TRUE, NULL);
}

static void
phosh_run_command_dialog_finalize (GObject *obj)
{
  PhoshRunCommandDialog *self = PHOSH_RUN_COMMAND_DIALOG (obj);

  g_free (self->entry_command);

  G_OBJECT_CLASS (phosh_run_command_dialog_parent_class)->finalize (obj);
}

static void
phosh_run_command_dialog_class_init (PhoshRunCommandDialogClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_run_command_dialog_finalize;

  signals[DONE] = g_signal_new ("done",
                                G_TYPE_FROM_CLASS (klass),
                                G_SIGNAL_RUN_LAST,
                                0, NULL, NULL, NULL,
                                G_TYPE_NONE, 2, G_TYPE_BOOLEAN, G_TYPE_STRING);

  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/phosh/ui/run-command-dialog.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshRunCommandDialog, entry_command);
  gtk_widget_class_bind_template_callback (widget_class, run_command_dialog_key_press_event_cb);
  gtk_widget_class_bind_template_callback (widget_class, run_command_dialog_canceled_event_cb);
}

static void
phosh_run_command_dialog_init (PhoshRunCommandDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
phosh_run_command_dialog_new (void)
{
  return g_object_new (PHOSH_TYPE_RUN_COMMAND_DIALOG, NULL);
}
