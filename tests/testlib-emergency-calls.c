/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Thomas Booker <tw.booker@outlook.com>
 */

#include "testlib-emergency-calls.h"

#define OBJECT_PATH "/org/gnome/Calls"
#define BUS_NAME "org.gnome.Calls"

static void
on_bus_acquired (GDBusConnection *connection,
                 const char      *name,
                 gpointer         user_data)
{
  PhoshTestEmergencyCallsMock *self = user_data;
  g_autoptr (GError) err = NULL;

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self->skel),
                                    connection,
                                    OBJECT_PATH,
                                    &err);
  g_assert_no_error (err);
}


#define CONTACT_FORMAT "(ssia{sv})"
#define CONTACTS_FORMAT "a" CONTACT_FORMAT

static gboolean
on_handle_get_emergency_contacts (PhoshEmergencyCalls         *interface,
                                  GDBusMethodInvocation       *invocation,
                                  PhoshTestEmergencyCallsMock *self)
{
  /* a(ssia{sv}) */
  GVariant *contacts;
  GVariantBuilder contacts_builder;

  g_variant_builder_init (&contacts_builder, G_VARIANT_TYPE (CONTACTS_FORMAT));

  g_variant_builder_open (&contacts_builder, G_VARIANT_TYPE (CONTACT_FORMAT));
  /* https://www.police.govt.nz/call-111 */
  /* Call triple one (111) when you need an emergency response from Police, Fire or Ambulance.  */

  /* id: The identifier to dial (usually the phone number) */
  g_variant_builder_add (&contacts_builder, "s", "111");
  /* name: The contacts name */
  g_variant_builder_add (&contacts_builder, "s", "Emergency Services");
  /* source: The contacts source (e.g. addresbook, sim, ...) */
  g_variant_builder_add (&contacts_builder, "i", 0);
  /* a{sv} properties: Additional properties */
  g_variant_builder_add (&contacts_builder, "a{sv}", NULL);
  g_variant_builder_close (&contacts_builder);

  g_variant_builder_open(&contacts_builder, G_VARIANT_TYPE (CONTACT_FORMAT));
  /* https://www.police.govt.nz/use-105 */
  /* 105 is the number for Police non-emergencies. */

  /* id: The identifier to dial (usually the phone number) */
  g_variant_builder_add (&contacts_builder, "s", "105");
  /* name: The contacts name */
  g_variant_builder_add (&contacts_builder, "s", "Police Non-Emergency");
  /* source: The contacts source (e.g. addresbook, sim, ...) */
  g_variant_builder_add (&contacts_builder, "i", 0);
  /* a{sv} properties: Additional properties */
  g_variant_builder_add (&contacts_builder, "a{sv}", NULL);
  g_variant_builder_close (&contacts_builder);

  g_variant_builder_open(&contacts_builder, G_VARIANT_TYPE (CONTACT_FORMAT));
  /* https://en.wikipedia.org/wiki/Fictitious_telephone_number */

  /* id: The identifier to dial (usually the phone number) */
  g_variant_builder_add (&contacts_builder, "s", "+61 02 5550 3492");
  /* name: The contacts name */
  g_variant_builder_add (&contacts_builder, "s", "Bob (Parent)");
  /* source: The contacts source (e.g. addresbook, sim, ...) */
  g_variant_builder_add (&contacts_builder, "i", 0);
  /* a{sv} properties: Additional properties */
  g_variant_builder_add (&contacts_builder, "a{sv}", NULL);
  g_variant_builder_close(&contacts_builder);

  g_variant_builder_open(&contacts_builder, G_VARIANT_TYPE (CONTACT_FORMAT));
  /* https://en.wikipedia.org/wiki/Fictitious_telephone_number */

  /* id: The identifier to dial (usually the phone number) */
  g_variant_builder_add (&contacts_builder, "s", "+61 02 7010 9462");
  /* name: The contacts name */
  g_variant_builder_add (&contacts_builder, "s", "My General Practitioner");
  /* source: The contacts source (e.g. addresbook, sim, ...) */
  g_variant_builder_add (&contacts_builder, "i", 0);
  /* a{sv} properties: Additional properties */
  g_variant_builder_add (&contacts_builder, "a{sv}", NULL);
  g_variant_builder_close (&contacts_builder);

  contacts = g_variant_builder_end (&contacts_builder);

  phosh_emergency_calls_complete_get_emergency_contacts (interface, invocation, contacts);

  return TRUE;
}


PhoshTestEmergencyCallsMock *
phosh_test_emergency_calls_mock_new (void)
{
  PhoshTestEmergencyCallsMock *self;

  self = g_new0 (PhoshTestEmergencyCallsMock, 1);
  self->skel = phosh_emergency_calls_skeleton_new ();

  g_signal_connect (self->skel,
                    "handle-get-emergency-contacts",
                    G_CALLBACK (on_handle_get_emergency_contacts),
                    self);

  return self;
}


void
phosh_test_emergency_calls_mock_dispose (PhoshTestEmergencyCallsMock *self)
{
  g_clear_handle_id (&self->bus_id,  g_bus_unown_name);
  g_clear_object (&self->skel);
  g_free (self);
}


void
phosh_test_emergency_calls_mock_export (PhoshTestEmergencyCallsMock *self)
{
  self->bus_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                 BUS_NAME,
                                 G_BUS_NAME_OWNER_FLAGS_NONE,
                                 on_bus_acquired,
                                 NULL,
                                 NULL,
                                 self,
                                 NULL);
  g_assert_true (self->bus_id > 0);
}
