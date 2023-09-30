/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "notifications/notification-source.c"
#include "stubs/bad-prop.h"


static void
test_phosh_notification_source_new (void)
{
  g_autoptr (PhoshNotificationSource) source = NULL;
  g_autoptr (PhoshNotification) noti = NULL;
  const char *name;
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();

  source = phosh_notification_source_new ("org.gnome.zbrown.KingsCross");

  noti = phosh_notification_new (0,
                                 NULL,
                                 NULL,
                                 "Hey",
                                 "Testing",
                                 NULL,
                                 NULL,
                                 PHOSH_NOTIFICATION_URGENCY_NORMAL,
                                 NULL,
                                 FALSE,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 now);

  phosh_notification_source_add (source, noti);

  name = phosh_notification_source_get_name (PHOSH_NOTIFICATION_SOURCE (source));

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (source)), ==, 1);
  g_assert_true (noti == g_list_model_get_item (G_LIST_MODEL (source), 0));
  g_assert_cmpstr (name, ==, "org.gnome.zbrown.KingsCross");
}


static void
test_phosh_notification_source_get (void)
{
  g_autoptr (PhoshNotificationSource) source = NULL;
  g_autofree char *name;

  source = phosh_notification_source_new ("org.gnome.zbrown.KingsCross");

  g_object_get (source, "name", &name, NULL);

  g_assert_cmpstr (name, ==, "org.gnome.zbrown.KingsCross");
}


static void
test_phosh_notification_source_close_invalid (void)
{
  if (g_test_subprocess ()) {
    g_autoptr (PhoshNotificationSource) source = NULL;
    g_autoptr (PhoshNotification) noti = NULL;

    g_autoptr (GDateTime) now = g_date_time_new_now_local ();

    source = phosh_notification_source_new ("org.gnome.zbrown.KingsCross");

    noti = phosh_notification_new (0,
                                   NULL,
                                   NULL,
                                   "Hey",
                                   "Testing",
                                   NULL,
                                   NULL,
                                   PHOSH_NOTIFICATION_URGENCY_NORMAL,
                                   NULL,
                                   FALSE,
                                   FALSE,
                                   NULL,
                                   NULL,
                                   now);
    phosh_notification_source_add (source, noti);
    noti = phosh_notification_new (1,
                                   NULL,
                                   NULL,
                                   "Hey",
                                   "Testing",
                                   NULL,
                                   NULL,
                                   PHOSH_NOTIFICATION_URGENCY_NORMAL,
                                   NULL,
                                   FALSE,
                                   FALSE,
                                   NULL,
                                   NULL,
                                   now);
    /* Boom */
    closed (source, PHOSH_NOTIFICATION_REASON_DISMISSED, noti);
  }
  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*: phosh-notification-source-CRITICAL *"
                             "Can't remove unknown notification *\n");
}


static void
test_phosh_notification_source_set_prop_invalid (void)
{
  g_autoptr (PhoshNotificationSource) source =
    phosh_notification_source_new ("org.gnome.zbrown.KingsCross");

  BAD_PROP_SET (source, phosh_notification_source, PhoshNotificationSource);
}


static void
test_phosh_notification_source_get_prop_invalid (void)
{
  g_autoptr(PhoshNotificationSource) source =
    phosh_notification_source_new ("org.gnome.zbrown.KingsCross");

  BAD_PROP_GET (source, phosh_notification_source, PhoshNotificationSource);
}


int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/notification-source/new", test_phosh_notification_source_new);
  g_test_add_func ("/phosh/notification-source/get", test_phosh_notification_source_get);
  g_test_add_func ("/phosh/notification-source/close-invalid", test_phosh_notification_source_close_invalid);
  g_test_add_func ("/phosh/notification-source/set_prop_invalid", test_phosh_notification_source_set_prop_invalid);
  g_test_add_func ("/phosh/notification-source/get_prop_invalid", test_phosh_notification_source_get_prop_invalid);

  return g_test_run ();
}
