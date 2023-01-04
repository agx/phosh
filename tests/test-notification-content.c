/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();

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

  content = phosh_notification_content_new (noti, TRUE, NULL);

  noti_test = phosh_notification_content_get_notification (PHOSH_NOTIFICATION_CONTENT (content));

  g_assert_true (noti == noti_test);

  noti_test = NULL;
  g_object_get (content, "notification", &noti_test, NULL);

  g_assert_true (noti == noti_test);
}


static void
test_phosh_notification_content_no_summary (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();

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
                                 NULL,
                                 NULL,
                                 now);

  phosh_notification_content_new (noti, TRUE, NULL);
}


static void
test_phosh_notification_content_new_filter (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  PhoshNotification *noti_test = NULL;
  GtkWidget *content = NULL;
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();
  const char *filters[] = { "X-Phosh-Foo", "X-Phosh-Bar", NULL };

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

  content = phosh_notification_content_new (noti, TRUE, filters);

  noti_test = phosh_notification_content_get_notification (PHOSH_NOTIFICATION_CONTENT (content));

  g_assert_true (noti == noti_test);

  noti_test = NULL;
  g_object_get (content, "notification", &noti_test, NULL);

  g_assert_true (noti == noti_test);
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
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();

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
                                 NULL,
                                 NULL,
                                 now);

  content = phosh_notification_content_new (noti, TRUE, NULL);

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
  if (g_test_subprocess ()) {
    g_autoptr (PhoshNotification) noti = NULL;
    GStrv actions = (char *[]) { "app.test", NULL };
    g_autoptr (GDateTime) now = g_date_time_new_now_local ();

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
                                   NULL,
                                   NULL,
                                   now);
    /* Boom */
    phosh_notification_content_new (noti, TRUE, NULL);
  }
  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*: phosh-notification-content-WARNING *"
                             "Expected action label at 0, got NULL\n");
}


static void
test_phosh_notification_content_set_prop_invalid (void)
{
  GtkWidget *content = NULL;
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();
  g_autoptr (PhoshNotification) noti = phosh_notification_new (0,
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

  content = phosh_notification_content_new (noti, TRUE, NULL);
  BAD_PROP_SET (content, phosh_notification_content, PhoshNotificationContent);
}


static void
test_phosh_notification_content_get_prop_invalid (void)
{
  GtkWidget *content = NULL;
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();
  g_autoptr (PhoshNotification) noti = phosh_notification_new (0,
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

  content = phosh_notification_content_new (noti, TRUE, NULL);
  BAD_PROP_GET (content, phosh_notification_content, PhoshNotificationContent);
}


int
main (int argc, char **argv)
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/notification-content/new", test_phosh_notification_content_new);
  g_test_add_func ("/phosh/notification-content/no-summary", test_phosh_notification_content_no_summary);
  g_test_add_func ("/phosh/notification-content/new-filter", test_phosh_notification_content_new_filter);
  g_test_add_func ("/phosh/notification-content/actions", test_phosh_notification_content_actions);
  g_test_add_func ("/phosh/notification-content/bad-action", test_phosh_notification_content_bad_action);
  g_test_add_func ("/phosh/notification-content/get_prop_invalid", test_phosh_notification_content_get_prop_invalid);
  g_test_add_func ("/phosh/notification-content/set_prop_invalid", test_phosh_notification_content_set_prop_invalid);
  return g_test_run ();
}
