/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Thomas Booker <tw.booker@outlook.com>
 */

#define G_LOG_DOMAIN "phosh-emergency-contact-row"

#include "phosh-config.h"

#include "emergency-contact-row.h"
#include "emergency-contact.h"
#include "emergency-calls-manager.h"
#include "util.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>


/**
 * PhoshEmergencyContactRow:
 *
 * A widget that displays a the data in the attached #PhoshEmergencyContact.
 *
 * contact: The contact object contains data about the bound emergency contact.
 *
 * #PhoshEmergencyContactRow is a widget derived from `GtkListBoxRow`. It displays the data from
 * the bound #PhoshEmergencyContact.
 *
 * #PhoshEmergencyContactRow is designed to be used with `gtk_list_box_bind_model`.
 *
 * NOTE: Don't get confused between the widgets on this object and the
 * properties on the contact.  The widgets are bound to the data on
 * the contact object.
 */
struct _PhoshEmergencyContactRow {
  HdyActionRow           parent;

  GtkLabel              *name_label;
  GtkLabel              *id_label;

  PhoshEmergencyContact *contact;
};

G_DEFINE_TYPE (PhoshEmergencyContactRow, phosh_emergency_contact_row, HDY_TYPE_ACTION_ROW)


enum {
  PROP_0,
  PROP_CONTACT,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];


static void
bind_contact (PhoshEmergencyContactRow *self)
{
  g_object_bind_property (self->contact, "name",
                          self, "title",
                          G_BINDING_SYNC_CREATE);

  g_object_bind_property (self->contact, "id",
                          self, "subtitle",
                          G_BINDING_SYNC_CREATE);
}


static void
emergency_contact_row_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  PhoshEmergencyContactRow *self = PHOSH_EMERGENCY_CONTACT_ROW (object);

  switch (property_id) {
  case PROP_CONTACT:
    g_value_set_object (value, self->contact);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}



static void
emergency_contact_row_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PhoshEmergencyContactRow *self = PHOSH_EMERGENCY_CONTACT_ROW (object);

  switch (property_id) {
  case PROP_CONTACT:
    g_set_object (&self->contact, g_value_get_object (value));
    bind_contact (self);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
emergency_contact_row_dispose (GObject *object)
{
  PhoshEmergencyContactRow *self = PHOSH_EMERGENCY_CONTACT_ROW (object);

  g_clear_object (&self->contact);

  G_OBJECT_CLASS (phosh_emergency_contact_row_parent_class)->dispose (object);
}


static void
phosh_emergency_contact_row_class_init (PhoshEmergencyContactRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = emergency_contact_row_get_property;
  object_class->set_property = emergency_contact_row_set_property;
  object_class->dispose = emergency_contact_row_dispose;

  /**
   * PhoshEmergencyContactRow:contact:
   *
   * The contact object that contains data about the emergency contact.
   */
  props[PROP_CONTACT] =
    g_param_spec_object ("contact", "", "",
                         PHOSH_TYPE_EMERGENCY_CONTACT,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/emergency-contact-row.ui");
}


static void
phosh_emergency_contact_row_init (PhoshEmergencyContactRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


/**
 * phosh_emergency_contact_row_new:
 * @contact: (transfer none): The emergency contact that the widget will be bound to.
 *
 * Creates a new emergency contact row bound to [type@EmergencyContact].

 * Returns: (transfer full): a new #PhoshEmergencyContactRow
 */
PhoshEmergencyContactRow *
phosh_emergency_contact_row_new (PhoshEmergencyContact *contact)
{
  return g_object_new (PHOSH_TYPE_EMERGENCY_CONTACT_ROW, "contact", contact, NULL);
}


/**
 * phosh_emergency_contact_row_call:
 * @self: (transfer none): The EmergencyContactRow that will be called
 * @manager: (transfer none): The contact manager that will interface with the API.
 *
 * Starts an emergency call to the bound emergency contact.
 */
void
phosh_emergency_contact_row_call (PhoshEmergencyContactRow   *self,
                                  PhoshEmergencyCallsManager *manager)
{
  g_return_if_fail (PHOSH_IS_EMERGENCY_CONTACT_ROW (self));

  phosh_emergency_contact_call (self->contact, manager);
}
