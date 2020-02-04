/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3.0+
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "notifications/notify-manager.c"
#include "stubs/bad-prop.h"


static GTestDBus *test_bus;


typedef struct {
  GDBusConnection    *bus;
  PhoshNotifyManager *obj;
  GMainLoop          *loop;
} PhoshNotifyManagerTest;


static void
test_phosh_notify_manager_setup_test (PhoshNotifyManagerTest *test,
                                      gconstpointer           data)
{
  g_autoptr (GError) error = NULL;

  test->bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);

  g_assert_no_error (error);

  test->obj = phosh_notify_manager_get_default ();
  g_message ("Setup %i", ((GObject *) test->obj)->ref_count);

  test->loop = g_main_loop_new (NULL, FALSE);
}


static void
test_phosh_notify_manager_teardown_test (PhoshNotifyManagerTest *test,
                                         gconstpointer           data)
{
  // g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (test->obj));
  g_message ("B clear %i", ((GObject *) test->obj)->ref_count);
  g_object_unref (test->obj);
  g_clear_object (&test->obj);
  g_clear_object (&test->bus);
}


static void
test_phosh_notify_manager_default (PhoshNotifyManagerTest *test,
                                   gconstpointer           data)
{
  PhoshNotifyManager *manager = NULL;

  g_message ("A");
  manager = phosh_notify_manager_get_default ();

  g_message ("%i", ((GObject *) manager)->ref_count);

  g_message ("B");
  g_assert_true (manager == test->obj);

  g_message ("C");
  BAD_PROP (manager, phosh_notify_manager, PhoshNotifyManager);

  g_message ("D");
}


int
main (int argc, char **argv)
{
  int res = -1;

  gtk_test_init (&argc, &argv, NULL);

  g_test_add ("/phosh/notify-manager/default",
              PhoshNotifyManagerTest,
              NULL,
              test_phosh_notify_manager_setup_test,
              test_phosh_notify_manager_default,
              test_phosh_notify_manager_teardown_test);

  test_bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  g_test_dbus_up (test_bus);

  res = g_test_run ();

  g_test_dbus_down (test_bus);
  g_clear_object (&test_bus);

  return res;
}
