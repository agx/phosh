/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "shell.h"

#include "testlib-full-shell.h"
#include "calls-dbus.h"

#define BUS_NAME "org.gnome.Calls"
#define OBJECT_PATH "/org/gnome/Calls"

#define POP_TIMEOUT 50000000

typedef struct {
  GDBusObjectManagerServer *object_manager;
  gboolean bus_acquired;
  guint    bus_id;
} CallsMock;


static void
on_bus_acquired (GDBusConnection *connection,
                 const char      *name,
                 gpointer         user_data)
{
  CallsMock * mock = user_data;

  g_test_message ("Owned %s, creating object manager at %s", name, OBJECT_PATH);

  mock->object_manager = g_dbus_object_manager_server_new (OBJECT_PATH);
  g_dbus_object_manager_server_set_connection (mock->object_manager, connection);
  mock->bus_acquired = TRUE;
}


static void
calls_mock_dispose (CallsMock *self)
{
  g_clear_handle_id (&self->bus_id,  g_bus_unown_name);
  g_clear_object (&self->object_manager);
  g_free (self);
}


static CallsMock *
calls_mock_new (void)
{
  return g_new0 (CallsMock, 1);
}


static void
calls_mock_export (CallsMock *mock)
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


/* t1: test_phosh_calls_present */

static void
t1_on_calls_manager_present (PhoshCallsManager *manager,
                             GParamSpec        *pspec,
                             gpointer           user_data)
{
  PhoshCallsManager *cm = phosh_shell_get_calls_manager (phosh_shell_get_default ());
  gboolean present;

  g_assert_true (PHOSH_IS_CALLS_MANAGER (manager));

  present = phosh_calls_manager_get_present (cm);
  g_test_message ("Object manager present: %d", present);
  g_assert_true (present);

  g_main_loop_quit ((GMainLoop *) user_data);
}


static void
test_phosh_calls_present (PhoshTestFullShellFixture *fixture, gconstpointer unused)
{
  guint notify_id;
  CallsMock *mock;
  g_autoptr (GMainLoop) loop = NULL;
  PhoshCallsManager *cm;

  /* Wait until comp/shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  loop = g_main_loop_new (NULL, FALSE);
  cm = phosh_shell_get_calls_manager (phosh_shell_get_default ());
  notify_id = g_signal_connect (cm,
                                "notify::present",
                                G_CALLBACK (t1_on_calls_manager_present),
                                loop);

  mock = calls_mock_new ();
  calls_mock_export (mock);
  /* Run mainloop, main loop will be quit in notify::present callback */
  g_main_loop_run (loop);
  g_assert_true (mock->bus_acquired);
  g_signal_handler_disconnect (cm, notify_id);

  calls_mock_dispose (mock);
}


/* t2: test_phosh_calls_incoming */

static void
t2_on_active_call_changed (PhoshCallsManager *manager,
                           GParamSpec        *pspec,
                           gpointer           user_data)
{
  g_assert_true (PHOSH_IS_CALLS_MANAGER (manager));

  g_test_message ("Active call state changed");

  g_main_loop_quit ((GMainLoop *) user_data);
}


static void
t2_on_calls_manager_present (PhoshCallsManager *manager,
                             GParamSpec        *pspec,
                             gpointer           user_data)
{
  PhoshCallsManager *cm = phosh_shell_get_calls_manager (phosh_shell_get_default ());
  gboolean present;
  g_autoptr (PhoshCallsDBusObjectSkeleton) object = NULL;
  g_autoptr (PhoshCallsDBusCallsCall) iface = NULL;
  CallsMock *calls_mock = user_data;

  g_assert_true (PHOSH_IS_CALLS_MANAGER (manager));

  present = phosh_calls_manager_get_present (cm);
  g_test_message ("Object manager present: %d", present);
  g_assert_true (present);

  object = phosh_calls_dbus_object_skeleton_new (OBJECT_PATH "/Call/1");
  iface = phosh_calls_dbus_calls_call_skeleton_new ();
  phosh_calls_dbus_calls_call_set_state (iface, PHOSH_CALL_STATE_ACTIVE);
  g_dbus_object_skeleton_add_interface (G_DBUS_OBJECT_SKELETON (object),
                                        G_DBUS_INTERFACE_SKELETON (iface));
  g_dbus_object_manager_server_export (calls_mock->object_manager, G_DBUS_OBJECT_SKELETON (object));
}


static void
test_phosh_calls_incoming (PhoshTestFullShellFixture *fixture, gconstpointer unused)
{
  CallsMock *mock;
  g_autoptr (GMainLoop) loop = NULL;
  PhoshCallsManager *cm;
  guint id_present, id_active;

  /* Wait until comp/shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  loop = g_main_loop_new (NULL, FALSE);
  mock = calls_mock_new ();

  cm = phosh_shell_get_calls_manager (phosh_shell_get_default ());
  id_present = g_signal_connect (cm,
                           "notify::present",
                           G_CALLBACK (t2_on_calls_manager_present),
                           mock);

  id_active = g_signal_connect (cm,
                                "notify::active-call",
                                G_CALLBACK (t2_on_active_call_changed),
                                loop);
  calls_mock_export (mock);

  /* Run mainloop, main loop will be quit in active-call callback */
  g_main_loop_run (loop);
  g_assert_true (mock->bus_acquired);

  g_signal_handler_disconnect (cm, id_present);
  g_signal_handler_disconnect (cm, id_active);

  calls_mock_dispose (mock);
}


int
main (int argc, char *argv[])
{
  g_autoptr (PhoshTestFullShellFixtureCfg) cfg = NULL;

  g_test_init (&argc, &argv, NULL);

  cfg = phosh_test_full_shell_fixture_cfg_new ("phosh-calls-manager");

  PHOSH_FULL_SHELL_TEST_ADD ("/phosh/dbus/calls-manager/present", cfg, test_phosh_calls_present);
  PHOSH_FULL_SHELL_TEST_ADD ("/phosh/dbus/calls-manager/incoming", cfg, test_phosh_calls_incoming);

  return g_test_run ();
}
