/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-plugin-prefs-config.h"

#include "ticket-box-prefs.h"

#define TICKET_BOX_SCHEMA_ID "sm.puri.phosh.plugins.ticket-box"
#define TICKET_BOX_FOLDER_KEY "folder"

#include <glib/gi18n-lib.h>

/**
 * PhoshTicketBoxPrefs:
 *
 * Preferences for ticket-box plugin
 */
struct _PhoshTicketBoxPrefs {
  AdwPreferencesWindow  parent;

  GtkWidget            *folder_entry;
  GtkWidget            *folder_button;

  char                 *ticket_box_path;
  GSettings            *settings;
};

G_DEFINE_TYPE (PhoshTicketBoxPrefs, phosh_ticket_box_prefs, ADW_TYPE_PREFERENCES_WINDOW);

static gboolean
folder_get_mapping (GValue *value, GVariant *variant, gpointer user_data)
{
  g_autofree char *path = NULL;
  const char *folder;

  folder = g_variant_get_string (variant, NULL);

  if (folder[0] != '/')
    path = g_build_filename (g_get_home_dir (), folder, NULL);
  else
    path = g_strdup (folder);

  g_value_set_string (value, path);

  return TRUE;
}


static void
on_file_chooser_response (GtkNativeDialog* dialog, gint response_id, gpointer user_data)
{
    PhoshTicketBoxPrefs *self = PHOSH_TICKET_BOX_PREFS (user_data);
    GtkFileChooser *filechooser = GTK_FILE_CHOOSER (dialog);
    g_autofree gchar *filename = NULL;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    if (response_id == GTK_RESPONSE_ACCEPT) {
        g_autoptr (GFile) file = gtk_file_chooser_get_file (filechooser);
        filename = g_file_get_path (file);
    }
G_GNUC_END_IGNORE_DEPRECATIONS

    g_object_unref (dialog);

    if (filename == NULL)
      return;

    gtk_editable_set_text (GTK_EDITABLE (self->folder_entry), filename);
}


static void
on_folder_button_clicked (PhoshTicketBoxPrefs *self)
{
  GtkFileChooserNative *filechooser;
  const char *current;
  g_autoptr (GFile) current_file = NULL;

  g_assert (PHOSH_IS_TICKET_BOX_PREFS (self));
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  filechooser = gtk_file_chooser_native_new(_("Choose Folder"),
                                            GTK_WINDOW (self),
                                            GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                            _("_Open"),
                                            _("_Cancel"));
G_GNUC_END_IGNORE_DEPRECATIONS

  current = gtk_editable_get_text (GTK_EDITABLE (self->folder_entry));
  current_file = g_file_new_for_path (current);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (current_file)
    gtk_file_chooser_set_file (GTK_FILE_CHOOSER (filechooser), current_file, NULL);
G_GNUC_END_IGNORE_DEPRECATIONS

  g_signal_connect (filechooser, "response",
                    G_CALLBACK (on_file_chooser_response), self);
  gtk_native_dialog_show (GTK_NATIVE_DIALOG (filechooser));
}

static void
phosh_ticket_box_prefs_finalize (GObject *object)
{
  PhoshTicketBoxPrefs *self = PHOSH_TICKET_BOX_PREFS (object);

  g_clear_object (&self->settings);
  g_clear_pointer (&self->ticket_box_path, g_free);

  G_OBJECT_CLASS (phosh_ticket_box_prefs_parent_class)->finalize (object);
}

static void
phosh_ticket_box_prefs_class_init (PhoshTicketBoxPrefsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_ticket_box_prefs_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/plugins/ticket-box-prefs/ticket-box-prefs.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshTicketBoxPrefs, folder_entry);
  gtk_widget_class_bind_template_callback (widget_class, on_folder_button_clicked);
}


static void
phosh_ticket_box_prefs_init (PhoshTicketBoxPrefs *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->settings = g_settings_new (TICKET_BOX_SCHEMA_ID);
  g_settings_bind_with_mapping (self->settings, TICKET_BOX_FOLDER_KEY,
                                self->folder_entry, "text",
                                G_SETTINGS_BIND_DEFAULT,
                                folder_get_mapping,
                                NULL /* set_mapping */,
                                NULL /* userdata */,
                                NULL /* destroyfunc */);
}
