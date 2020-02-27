/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3.0+
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "notifications/notification-content.c"
#include "stubs/bad-prop.h"


static void
test_phosh_notification_content_new (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  PhoshNotification *noti_test = NULL;
  GtkWidget *content = NULL;

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

  content = phosh_notification_content_new (noti);

  noti_test = phosh_notification_content_get_notification (PHOSH_NOTIFICATION_CONTENT (content));

  g_assert_true (noti == noti_test);

  noti_test = NULL;
  g_object_get (content, "notification", &noti_test, NULL);

  g_assert_true (noti == noti_test);

  BAD_PROP (content, phosh_notification_content, PhoshNotificationContent);
}


static void
test_phosh_notification_content_no_summary (void)
{
  g_autoptr (PhoshNotification) noti = NULL;

  noti = phosh_notification_new (0,
                                 NULL,
                                 NULL,
                                 NULL,
                                 "Testing",
                                 NULL,
                                 NULL,
                                 PHOSH_NOTIFICATION_URGENCY_NORMAL,
                                 NULL,
                                 FALSE,
                                 FALSE,
                                 NULL);

  phosh_notification_content_new (noti);
}


static gboolean actioned_called = FALSE;


static void
actioned (PhoshNotification *noti,
          const char        *action)
{
  actioned_called = TRUE;

  g_assert_cmpstr (action, ==, "demo-time");
}


static void
test_phosh_notification_content_actions (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  GtkWidget *content = NULL;
  GStrv actions = (char *[]) { "app.test", "Test", "default", "Me", NULL };

  noti = phosh_notification_new (0,
                                 NULL,
                                 NULL,
                                 NULL,
                                 "Testing",
                                 NULL,
                                 NULL,
                                 PHOSH_NOTIFICATION_URGENCY_NORMAL,
                                 actions,
                                 FALSE,
                                 FALSE,
                                 NULL);

  content = phosh_notification_content_new (noti);

  g_signal_connect (noti, "actioned", G_CALLBACK (actioned), NULL);

  actioned_called = FALSE;

  g_action_group_activate_action (gtk_widget_get_action_group (content, "noti"),
                                  "activate",
                                  g_variant_new_string ("demo-time"));

  g_assert_true (actioned_called);
}


static void
test_phosh_notification_content_bad_action (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  GStrv actions = (char *[]) { "app.test", NULL };

  noti = phosh_notification_new (0,
                                 NULL,
                                 NULL,
                                 NULL,
                                 "Testing",
                                 NULL,
                                 NULL,
                                 PHOSH_NOTIFICATION_URGENCY_NORMAL,
                                 actions,
                                 FALSE,
                                 FALSE,
                                 NULL);

  g_test_expect_message ("phosh-notification-content",
                         G_LOG_LEVEL_WARNING,
                         "Expected action label at *, got NULL");
  phosh_notification_content_new (noti);
}


int
main (int argc, char **argv)
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/notification-content/new", test_phosh_notification_content_new);
  g_test_add_func ("/phosh/notification-content/no-summary", test_phosh_notification_content_no_summary);
  g_test_add_func ("/phosh/notification-content/actions", test_phosh_notification_content_actions);
  g_test_add_func ("/phosh/notification-content/bad-action", test_phosh_notification_content_bad_action);

  return g_test_run ();
}
