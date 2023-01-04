/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "notifications/notification-frame.c"


static gboolean actioned_called = FALSE;


static void
actioned (PhoshNotification *noti,
          const char        *action)
{
  actioned_called = TRUE;

  g_assert_cmpstr (action, ==, "default");
}



static void
test_phosh_notification_frame_new (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  GtkWidget *frame = NULL;
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

  frame = phosh_notification_frame_new (TRUE, NULL);
  phosh_notification_frame_bind_notification (PHOSH_NOTIFICATION_FRAME (frame),
                                              noti);
}


static void
test_phosh_notification_frame_new_filter (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  PhoshNotificationFrame *frame = NULL;
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

  frame = PHOSH_NOTIFICATION_FRAME (phosh_notification_frame_new (TRUE, filters));
  phosh_notification_frame_bind_notification (PHOSH_NOTIFICATION_FRAME (frame),
                                              noti);
  g_assert_cmpstr (phosh_notification_frame_get_action_filter_keys (frame)[0], ==, "X-Phosh-Foo");
  g_assert_cmpstr (phosh_notification_frame_get_action_filter_keys (frame)[1], ==, "X-Phosh-Bar");
  g_assert_cmpstr (phosh_notification_frame_get_action_filter_keys (frame)[2], ==, NULL);
}


static void
test_phosh_notification_frame_notification_activated (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  GtkWidget *frame = NULL;
  GtkWidget *list = NULL;
  GtkListBoxRow *row = NULL;
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

  frame = phosh_notification_frame_new (TRUE, NULL);
  phosh_notification_frame_bind_notification (PHOSH_NOTIFICATION_FRAME (frame),
                                              noti);
  g_signal_connect (noti, "actioned", G_CALLBACK (actioned), NULL);

  actioned_called = FALSE;

  list = PHOSH_NOTIFICATION_FRAME (frame)->list_notifs;
  row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (list), 0);

  notification_activated (PHOSH_NOTIFICATION_FRAME (frame),
                          row,
                          GTK_LIST_BOX (list));
  g_assert_true (actioned_called);
}


int
main (int argc, char **argv)
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/notification-frame/new", test_phosh_notification_frame_new);
  g_test_add_func ("/phosh/notification-frame/new-filter", test_phosh_notification_frame_new_filter);
  g_test_add_func ("/phosh/notification-frame/notification-activated", test_phosh_notification_frame_notification_activated);

  return g_test_run ();
}
