/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Chris Talbot <chris@talbothome.com>
 */

#include "emergency-info-row.h"

#include <glib/gi18n.h>
#include <handy.h>

enum {
  PROP_0,
  PROP_NUMBER,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshEmergencyInfoRow {
  HdyActionRow parent;

  GtkLabel    *label_contact;
};

G_DEFINE_TYPE (PhoshEmergencyInfoRow, phosh_emergency_info_row, HDY_TYPE_ACTION_ROW)

/* TODO: Clicking on the emergency contact does nothing. */

static void
phosh_emergency_info_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  PhoshEmergencyInfoRow *self = PHOSH_EMERGENCY_INFO_ROW (object);

  switch (property_id) {
  case PROP_NUMBER:
    gtk_label_set_text (self->label_contact, g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
phosh_emergency_info_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  PhoshEmergencyInfoRow *self = PHOSH_EMERGENCY_INFO_ROW (object);

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
phosh_emergency_info_row_class_init (PhoshEmergencyInfoRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_emergency_info_get_property;
  object_class->set_property = phosh_emergency_info_set_property;

  /**
   * PhoshEmergencyInfoRow::number
   *
   * The phone number
   */
  props[PROP_NUMBER] =
    g_param_spec_string ("number", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/plugins/emergency-info/emergency-info-row.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyInfoRow, label_contact);
}

static void
phosh_emergency_info_row_init (PhoshEmergencyInfoRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
phosh_emergency_info_row_new (const char *contact,
                              const char *number,
                              const char *relationship)
{
  PhoshEmergencyInfoRow *self;

  self = g_object_new (PHOSH_TYPE_EMERGENCY_INFO_ROW,
                       "title", contact,
                       "subtitle", relationship,
                       "number", number,
                       NULL);

  return GTK_WIDGET (self);
}
