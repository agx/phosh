/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Thomas Booker <tw.booker@outlook.com>
 */

#define G_LOG_DOMAIN "phosh-emergency-contact"

#include "phosh-config.h"

#include "emergency-contact.h"

enum {
  PROP_0,
  PROP_ID,
  PROP_NAME,
  PROP_SOURCE,
  PROP_ADDITIONAL_PROPERTIES,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];

/**
 * PhoshEmergencyContact:
 * @short_description: A object to hold data about an emergency contact.
 * @Title: PhoshEmergencyContact
 *
 * id: The id that is given to this emergency contact by the calls DBus API. Eg `+123 123 123`
 * name: The name of person in this emergency contact. Eg Bob
 * source: An integer identifying the source of the emergency contact. Eg SIM card or user entered.
 * additional_properties: Any other information, given by the calls DBus API. Format is `a{sv}`
 *
 * #PhoshEmergencyContact holds data about an emergency contact and is used to build #PhoshEmergencyContactRow.
 *
 * A GListStore of #PhoshEmergencyContact is provided by #PhoshEmergencyContactManager.
 */
struct _PhoshEmergencyContact {
  GObject   parent;

  gchar    *id;
  gchar    *name;
  gint32    source;
  GVariant *additional_properties;
};

G_DEFINE_TYPE (PhoshEmergencyContact, phosh_emergency_contact, G_TYPE_OBJECT);

static void
emergency_contact_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PhoshEmergencyContact *self = PHOSH_EMERGENCY_CONTACT (object);

  switch (property_id) {
  case PROP_ID:
    g_value_set_string (value, self->id);
    break;
  case PROP_NAME:
    g_value_set_string (value, self->name);
    break;
  case PROP_SOURCE:
    g_value_set_int (value, self->source);
    break;
  case PROP_ADDITIONAL_PROPERTIES:
    g_value_set_variant (value, self->additional_properties);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
emergency_contact_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  PhoshEmergencyContact *self = PHOSH_EMERGENCY_CONTACT (object);

  switch (property_id) {
  case PROP_ID:
    g_free (self->id);
    self->id = g_value_dup_string (value);
    break;
  case PROP_NAME:
    g_free (self->name);
    self->name = g_value_dup_string (value);
    break;
  case PROP_SOURCE:
    self->source = g_value_get_int (value);
    break;
  case PROP_ADDITIONAL_PROPERTIES:
    g_clear_pointer (&self->additional_properties, g_variant_unref);
    self->additional_properties = g_value_dup_variant (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
emergency_contact_finalize (GObject *object)
{
  PhoshEmergencyContact *self = PHOSH_EMERGENCY_CONTACT (object);

  g_free (self->id);
  g_free (self->name);
  g_variant_unref (self->additional_properties);

  G_OBJECT_CLASS (phosh_emergency_contact_parent_class)->finalize (object);
}


static void
phosh_emergency_contact_class_init (PhoshEmergencyContactClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = emergency_contact_finalize;
  object_class->get_property = emergency_contact_get_property;
  object_class->set_property = emergency_contact_set_property;

  /**
   * PhoshEmergencyContact:id:
   *
   * The id that is given to this emergency contact by the calls DBus
   * API.
   */
  props[PROP_ID] =
    g_param_spec_string ("id", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  /**
   * PhoshEmergencyContact:name:
   *
   * The name of person in this emergency contact. Eg Bob.
   */
  props[PROP_NAME] =
    g_param_spec_string ("name", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
 /**
  * PhoshEmergencyContact:source:
  *
  * An integer identifying the source of the emergency contact. Eg SIM card or user entered.
  */
  props[PROP_SOURCE] =
    g_param_spec_int ("source", "", "",
                      G_MININT, G_MAXINT,
                      1,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
 /**
  * PhoshEmergencyContact:additional-properties:
  *
  * Any other information, given by the calls DBus API.
  */
  props[PROP_ADDITIONAL_PROPERTIES] =
    g_param_spec_variant ("additional-properties", "", "",
                          G_VARIANT_TYPE ("a{sv}"),
                          NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}


static void
phosh_emergency_contact_init (PhoshEmergencyContact *self)
{
}

/**
 * phosh_emergency_contact_new:
 * @id: The id that is given to this emergency contact by the calls DBus API. Eg `+123 123 123`
 * @name: The name of person in this emergency contact. Eg Bob
 * @source: An integer identifying the source of the emergency contact. Eg SIM card or user entered.
 * @additional_properties: Any other information, given by the calls DBus API. Format is `a{sv}`
 *
 * #phosh_emergency_contact_new creates a new #PhoshEmergencyContact with the provided information.
 * The input information is designed parsed from the DBus API and then provided to this function.
 * See #PhoshEmergencyContactManager `on_update_finish` function for reference.
 */
PhoshEmergencyContact *
phosh_emergency_contact_new (const char *id,
                             const char *name,
                             gint32      source,
                             GVariant   *additional_properties)
{
  return g_object_new (PHOSH_TYPE_EMERGENCY_CONTACT,
                       "id", id,
                       "name", name,
                       "source", source,
                       "additional-properties", additional_properties,
                       NULL);
}

/**
 * phosh_emergency_contact_call:
 * @self: The Contact to call.
 * @emergency_calls_manager: The contact manager that will interface with the API.
 *
 * Starts an emergency call to this emergency contact.
 *
 * This function calls #phosh_emergency_contact_manager_call with the
 * passed emergency_contact_manager parameter and id from self.
 */
void
phosh_emergency_contact_call (PhoshEmergencyContact      *self,
                              PhoshEmergencyCallsManager *emergency_contact_manager)
{
  g_return_if_fail (PHOSH_IS_EMERGENCY_CONTACT (self));

  phosh_emergency_calls_manager_call (emergency_contact_manager, self->id);
}
