/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Chris Talbot <chris@talbothome.com>
 */

#include "emergency-info-common.h"
#include "emergency-info-prefs-row.h"

enum {
  PROP_0,
  PROP_NUMBER,
  PROP_LAST_PROP,
};

static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshEmergencyInfoPrefsRow {
  AdwActionRow parent;

  GtkLabel    *label_contact;
};

G_DEFINE_TYPE (PhoshEmergencyInfoPrefsRow, phosh_emergency_info_prefs_row, ADW_TYPE_ACTION_ROW)

static void
on_delete_button_clicked (PhoshEmergencyInfoPrefsRow *self)
{
  GtkWidget *parent;
  g_autofree char *path = NULL;
  g_autoptr (GKeyFile) key_file = NULL;

  /* To dispose: You need to get the parent and have the parent remove it. */
  parent = gtk_widget_get_parent (GTK_WIDGET (self));
  gtk_list_box_remove (GTK_LIST_BOX (parent), (GTK_WIDGET (self)));

  path = g_build_filename (g_get_user_config_dir (),
                           EMERGENCY_INFO_GKEYFILE_LOCATION,
                           EMERGENCY_INFO_GKEYFILE_NAME,
                           NULL);

  key_file = g_key_file_new ();

  if (!g_key_file_load_from_file (key_file, path, G_KEY_FILE_KEEP_COMMENTS, NULL)) {
    g_warning ("No Keyfile found at %s", path);
    return;
  }

  g_key_file_remove_key (key_file,
                         CONTACTS_GROUP,
                         adw_preferences_row_get_title (ADW_PREFERENCES_ROW (self)),
                         NULL);

  if (!g_key_file_save_to_file (key_file, path, NULL))
    g_warning ("Error Saving Keyfile at %s", path);
}

static void
phosh_emergency_info_pref_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  PhoshEmergencyInfoPrefsRow *self = PHOSH_EMERGENCY_INFO_PREFS_ROW (object);

  switch (property_id) {
  case PROP_NUMBER:
    gtk_label_set_text (self->label_contact, g_value_get_string(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
phosh_emergency_info_pref_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  PhoshEmergencyInfoPrefsRow *self = PHOSH_EMERGENCY_INFO_PREFS_ROW (object);

  switch (property_id) {
  case PROP_NUMBER:
    g_value_set_string (value, gtk_label_get_text (self->label_contact));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
phosh_emergency_info_prefs_row_class_init (PhoshEmergencyInfoPrefsRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_emergency_info_pref_get_property;
  object_class->set_property = phosh_emergency_info_pref_set_property;

  props[PROP_NUMBER] =
    g_param_spec_string("number", "", "", " ",
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/plugins/emergency-info-prefs/emergency-info-prefs-row.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfoPrefsRow, label_contact);

  gtk_widget_class_bind_template_callback (widget_class, on_delete_button_clicked);
}

static void
phosh_emergency_info_prefs_row_init (PhoshEmergencyInfoPrefsRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
phosh_emergency_info_prefs_row_new (const char *contact,
                                    const char *number,
                                    const char *relationship)
{
  PhoshEmergencyInfoPrefsRow *self;

  self = g_object_new (PHOSH_TYPE_EMERGENCY_INFO_PREFS_ROW,
                       "title", contact,
                       "subtitle", relationship ?: "",
                       "number", number,
                       NULL);

  return GTK_WIDGET (self);
}
