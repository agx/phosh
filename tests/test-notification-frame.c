/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3.0+
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "notifications/notification-frame.c"
#include "stubs/bad-prop.h"


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

  frame = phosh_notification_frame_new ();
  phosh_notification_frame_bind_notification (PHOSH_NOTIFICATION_FRAME (frame),
                                              noti);
}


static void
test_phosh_notification_frame_header_activated (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  GtkWidget *frame = NULL;

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

  frame = phosh_notification_frame_new ();
  phosh_notification_frame_bind_notification (PHOSH_NOTIFICATION_FRAME (frame),
                                              noti);
  g_signal_connect (noti, "actioned", G_CALLBACK (actioned), NULL);

  actioned_called = FALSE;

  header_activated (PHOSH_NOTIFICATION_FRAME (frame),
                    (GdkEventButton *) gdk_event_new (GDK_BUTTON_PRESS));
  g_assert_true (actioned_called);
}


static void
test_phosh_notification_frame_notification_activated (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  GtkWidget *frame = NULL;
  GtkWidget *list = NULL;
  GtkListBoxRow *row = NULL;

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

  frame = phosh_notification_frame_new ();
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
  g_test_add_func ("/phosh/notification-frame/header-activated", test_phosh_notification_frame_header_activated);
  g_test_add_func ("/phosh/notification-frame/notification-activated", test_phosh_notification_frame_notification_activated);

  return g_test_run ();
}
