/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3.0+
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
                                 NULL);

  phosh_notification_source_add (source, noti);

  name = phosh_notification_source_get_name (PHOSH_NOTIFICATION_SOURCE (source));

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (source)), ==, 1);
  g_assert_true (noti == g_list_model_get_item (G_LIST_MODEL (source), 0));
  g_assert_cmpstr (name, ==, "org.gnome.zbrown.KingsCross");

  BAD_PROP (source, phosh_notification_source, PhoshNotificationSource);
}


static void
test_phosh_notification_source_get (void)
{
  g_autoptr (PhoshNotificationSource) source = NULL;
  char *name;

  source = phosh_notification_source_new ("org.gnome.zbrown.KingsCross");

  g_object_get (source, "name", &name, NULL);

  g_assert_cmpstr (name, ==, "org.gnome.zbrown.KingsCross");
}


static void
test_phosh_notification_source_close_invalid (void)
{
  g_autoptr (PhoshNotificationSource) source = NULL;
  g_autoptr (PhoshNotification) noti = NULL;

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
                                 NULL);
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
                                 NULL);

  g_test_expect_message ("phosh-notification-source",
                         G_LOG_LEVEL_CRITICAL,
                         "Can't remove unknown notification *");
  closed (noti, PHOSH_NOTIFICATION_REASON_DISMISSED, source);
}


int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/notification-source/new", test_phosh_notification_source_new);
  g_test_add_func ("/phosh/notification-source/get", test_phosh_notification_source_get);
  g_test_add_func ("/phosh/notification-source/close-invalid", test_phosh_notification_source_close_invalid);

  return g_test_run ();
}
