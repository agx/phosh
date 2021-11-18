/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib-calls-mock.h"

#include "calls-dbus.h"
#include "calls-manager.h"

#define BUS_NAME "org.gnome.Calls"
#define OBJECT_PATH "/org/gnome/Calls"


static void
on_bus_acquired (GDBusConnection *connection,
                 const char      *name,
                 gpointer         user_data)
{
  PhoshTestCallsMock *mock = user_data;
  g_autoptr (PhoshCallsDBusObjectSkeleton) object = NULL;
  g_autoptr (PhoshCallsDBusCallsCall) iface = NULL;


  g_warning ("Owned %s, creating object manager at %s", name, OBJECT_PATH);

  mock->object_manager = g_dbus_object_manager_server_new (OBJECT_PATH);
  g_dbus_object_manager_server_set_connection (mock->object_manager, connection);
  mock->bus_acquired = TRUE;

  /* Add a call object */
  iface = phosh_calls_dbus_calls_call_skeleton_new ();
  phosh_calls_dbus_calls_call_set_state (iface, PHOSH_CALL_STATE_ACTIVE);
  phosh_calls_dbus_calls_call_set_inbound (iface, TRUE);
  phosh_calls_dbus_calls_call_set_display_name (iface, "John Doe");
  phosh_calls_dbus_calls_call_set_id (iface, "123456");
  phosh_calls_dbus_calls_call_set_image_path (iface, TEST_DATA_DIR "/cat.jpg");
  object = phosh_calls_dbus_object_skeleton_new (OBJECT_PATH "/Call/1");
  g_dbus_object_skeleton_add_interface (G_DBUS_OBJECT_SKELETON (object),
                                        G_DBUS_INTERFACE_SKELETON (iface));
  g_dbus_object_manager_server_export (mock->object_manager, G_DBUS_OBJECT_SKELETON (object));
}


PhoshTestCallsMock *
phosh_test_calls_mock_new (void)
{
  return g_new0 (PhoshTestCallsMock, 1);
}


void
phosh_test_calls_mock_dispose (PhoshTestCallsMock *self)
{
  g_clear_handle_id (&self->bus_id,  g_bus_unown_name);
  g_clear_object (&self->object_manager);
  g_free (self);
}


void
phosh_calls_mock_export (PhoshTestCallsMock *mock)
{
  mock->bus_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                 BUS_NAME,
                                 G_BUS_NAME_OWNER_FLAGS_NONE,
                                 on_bus_acquired,
                                 NULL,
                                 NULL,
                                 mock,
                                 NULL);
  g_assert_true (mock->bus_id > 0);
}
