/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Chris Talbot <chris@talbothome.com>
 */

#include "emergency-info.h"
#include "emergency-info-row.h"
#include "emergency-info-common.h"

/**
 * PhoshEmergencyInfo:
 *
 * Show emergency information and contacts
 * Calling contacts is not supported
 * For now, you need to manually create a GKeyfile at
 * `~/.config/phosh/EmergencyInfo.keyfile` with contents looking like:
 * ```
 * [Info]
 * # Emergency Info On the Owner
 * # If you do not add one of the items (or leave the item blank),
 * # the section will not display in the lockscreen.
 *
 * OwnerName=Chris
 * # You can make a newline with \n
 * HomeAddress=Lorem ipsum\nLorem ipsum
 * DateOfBirth=1 Jan, 1970
 * PreferredLanguage=C, MATLAB
 * Age=10,000
 * BloodType=Lorem ipsum
 * Height=163 cm
 * Weight=74 kg
 * # You can add multiple and seperate by a ";"
 * Allergies=Lorem ipsum dolor 1;Lorem ipsum dolor 2
 * # You can add multiple and seperate by a ";"
 * MedicationsAndConditions=Lorem ipsum 1;Lorem ipsum 2
 * OtherInfo=Lorem ipsum dolor sit amet
 *
 * [Contacts]
 * # You can add as many contacts as you want here
 * Contact 1=(123) 555-12;Brother
 * Contact 2=(204) 555-00
 * ```
 */
struct _PhoshEmergencyInfo {
  GtkBox               parent;

  char                *owner_name;
  char                *dob;
  char                *language;
  char                *home_address;
  char                *age;
  char                *blood_type;
  char                *height;
  char                *weight;
  char                *allergies;
  char                *medications_conditions;
  char                *other_info;
  GStrv                contacts;

  GtkLabel            *label_owner_name;
  GtkLabel            *label_dob;
  GtkLabel            *label_language;
  GtkLabel            *label_age;
  GtkLabel            *label_blood_type;
  GtkLabel            *label_height;
  GtkLabel            *label_weight;

  HdyActionRow        *row_owner_name;
  HdyActionRow        *row_dob;
  HdyActionRow        *row_language;
  HdyActionRow        *row_home_address;
  HdyActionRow        *row_age;
  HdyActionRow        *row_blood_type;
  HdyActionRow        *row_height;
  HdyActionRow        *row_weight;
  HdyActionRow        *row_allergies;
  HdyActionRow        *row_medications;
  HdyActionRow        *row_other_info;

  HdyPreferencesGroup *pers_info;
  HdyPreferencesGroup *emer_info;
  HdyPreferencesGroup *emer_contacts;
};

G_DEFINE_TYPE (PhoshEmergencyInfo, phosh_emergency_info, GTK_TYPE_BOX);

static gboolean
set_subtitle_or_hide_widget (const char   *label,
                             HdyActionRow *widget_action_row)
{
  gboolean visible;

  visible = !!(label && *label);
  hdy_action_row_set_subtitle (widget_action_row, label);
  gtk_widget_set_visible (GTK_WIDGET (widget_action_row), visible);
  return visible;
}

static gboolean
set_label_or_hide_widget (const char *label,
                          GtkLabel   *widget_label,
                          GtkWidget  *widget)
{
  gboolean visible;

  visible = !!(label && *label);
  gtk_label_set_text (widget_label, label);
  gtk_widget_set_visible (widget, visible);
  return visible;
}

static void
load_info (PhoshEmergencyInfo *self)
{
  g_autofree char *path = NULL;
  g_autoptr(GError) err = NULL;
  g_auto (GStrv) temp_med_cond = NULL;
  g_auto (GStrv) temp_allergies = NULL;
  g_autoptr (GKeyFile) key_file = NULL;
  gboolean display_med_info = FALSE;
  gboolean display_pers_info = FALSE;
  gsize i;

  path = g_build_filename (g_get_user_config_dir (),
                           EMERGENCY_INFO_GKEYFILE_LOCATION,
                           EMERGENCY_INFO_GKEYFILE_NAME,
                           NULL);

  key_file = g_key_file_new ();

  if (!g_key_file_load_from_file (key_file, path, G_KEY_FILE_NONE, &err)) {
    g_warning ("Failed to load keyfile at '%s': %s", path, err->message);
    return;
  }

  self->owner_name = g_key_file_get_string (key_file,
                                            INFO_GROUP,
                                            "OwnerName",
                                            NULL);

  self->dob = g_key_file_get_string (key_file,
                                     INFO_GROUP,
                                     "DateOfBirth",
                                     NULL);

  self->language = g_key_file_get_string (key_file,
                                          INFO_GROUP,
                                          "PreferredLanguage",
                                          NULL);

  self->home_address = g_key_file_get_string (key_file,
                                              INFO_GROUP,
                                              "HomeAddress",
                                              NULL);

  self->age = g_key_file_get_string (key_file,
                                     INFO_GROUP,
                                     "Age",
                                     NULL);

  self->blood_type = g_key_file_get_string (key_file,
                                            INFO_GROUP,
                                            "BloodType",
                                            NULL);

  self->height = g_key_file_get_string (key_file,
                                        INFO_GROUP,
                                        "Height",
                                        NULL);

  self->weight = g_key_file_get_string (key_file,
                                        INFO_GROUP,
                                        "Weight",
                                        NULL);

  temp_allergies = g_key_file_get_string_list (key_file,
                                               INFO_GROUP,
                                               "Allergies",
                                               NULL,
                                               NULL);

  if (temp_allergies)
    self->allergies = g_strjoinv ("\n", temp_allergies);

  temp_med_cond = g_key_file_get_string_list (key_file,
                                              INFO_GROUP,
                                              "MedicationsAndConditions",
                                              NULL,
                                              NULL);

  if (temp_med_cond)
    self->medications_conditions = g_strjoinv ("\n", temp_med_cond);

  self->other_info = g_key_file_get_string (key_file,
                                            INFO_GROUP,
                                            "OtherInfo",
                                            NULL);

  self->contacts = g_key_file_get_keys (key_file,
                                        CONTACTS_GROUP,
                                        NULL,
                                        NULL);

  for (i = 0; self->contacts && self->contacts[i]; i++) {
    g_autofree char *number = NULL;
    GtkWidget *new_row;

    number = g_key_file_get_string (key_file,
                                    CONTACTS_GROUP,
                                    self->contacts[i], NULL);
    if (number && *number) {
      g_auto (GStrv) number_split = NULL;

      number_split = g_strsplit (number, ";", 2);
      new_row = phosh_emergency_info_row_new (self->contacts[i],
                                              number_split[0],
                                              number_split[1]);
      gtk_container_add (GTK_CONTAINER (self->emer_contacts),
                         GTK_WIDGET (new_row));
    }
  }

  if (!self->contacts || !self->contacts[0])
    gtk_widget_hide (GTK_WIDGET (self->emer_contacts));

  set_label_or_hide_widget (self->owner_name, self->label_owner_name,
                            GTK_WIDGET (self->row_owner_name));
  display_pers_info |= set_label_or_hide_widget (self->dob, self->label_dob,
                                                 GTK_WIDGET (self->row_dob));
  display_pers_info |= set_label_or_hide_widget (self->language, self->label_language,
                                                 GTK_WIDGET (self->row_language));
  display_pers_info |= set_subtitle_or_hide_widget (self->home_address,
                                                    self->row_home_address);
  display_med_info |= set_label_or_hide_widget (self->age, self->label_age,
                                                GTK_WIDGET (self->row_age));
  display_med_info |= set_label_or_hide_widget (self->blood_type, self->label_blood_type,
                                                GTK_WIDGET (self->row_blood_type));
  display_med_info |= set_label_or_hide_widget (self->height, self->label_height,
                                                GTK_WIDGET (self->row_height));
  display_med_info |= set_label_or_hide_widget (self->weight, self->label_weight,
                                                GTK_WIDGET (self->row_weight));
  display_med_info |= set_subtitle_or_hide_widget (self->allergies,
                                                   self->row_allergies);
  display_med_info |= set_subtitle_or_hide_widget (self->medications_conditions,
                                                   self->row_medications);
  display_med_info |= set_subtitle_or_hide_widget (self->other_info,
                                                   self->row_other_info);
  gtk_widget_set_visible (GTK_WIDGET (self->emer_info), display_med_info);
  gtk_widget_set_visible (GTK_WIDGET (self->pers_info), display_pers_info);
}

static void
phosh_emergency_info_finalize (GObject *object)
{
  PhoshEmergencyInfo *self = PHOSH_EMERGENCY_INFO (object);

  g_free (self->owner_name);
  g_free (self->dob);
  g_free (self->language);
  g_free (self->home_address);
  g_free (self->age);
  g_free (self->blood_type);
  g_free (self->height);
  g_free (self->weight);
  g_free (self->allergies);
  g_free (self->other_info);
  g_free (self->medications_conditions);
  g_strfreev (self->contacts);

  G_OBJECT_CLASS (phosh_emergency_info_parent_class)->finalize (object);
}

static void
phosh_emergency_info_class_init (PhoshEmergencyInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_emergency_info_finalize;

  g_type_ensure (PHOSH_TYPE_EMERGENCY_INFO_ROW);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/plugins/emergency-info/emergency-info.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, label_owner_name);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, label_dob);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, label_language);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, label_age);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, label_blood_type);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, label_height);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, label_weight);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, row_owner_name);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, row_home_address);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, row_dob);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, row_language);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, row_allergies);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, row_medications);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, row_other_info);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, row_age);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, row_blood_type);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, row_height);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, row_weight);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, pers_info);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, emer_info);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfo, emer_contacts);

  gtk_widget_class_set_css_name (widget_class, "phosh-emergency-info");
}

static void
phosh_emergency_info_init (PhoshEmergencyInfo *self)
{
  g_autoptr (GtkCssProvider) css_provider = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));
  css_provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (css_provider,
                                       "/sm/puri/phosh/plugins/emergency-info/stylesheet/common.css");

  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (self)),
                                  GTK_STYLE_PROVIDER (css_provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  load_info (self);
}
