/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "notifications/notification-banner.c"

#include "testlib-compositor.h"

static gboolean was_notified = FALSE;

static void
notified (GObject *source, GParamSpec *param, gpointer data)
{
  was_notified = TRUE;
}


static void
test_phosh_notification_banner_new (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  g_autoptr (PhoshNotification) noti = NULL;
  PhoshNotification *noti_test = NULL;
  GtkWidget *banner = NULL;
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

  banner = phosh_notification_banner_new (noti);

  noti_test = phosh_notification_banner_get_notification (PHOSH_NOTIFICATION_BANNER (banner));
  g_assert_true (noti == noti_test);

  noti_test = NULL;

  g_object_get (banner, "notification", &noti_test, NULL);
  g_assert_true (noti == noti_test);
}


static void
test_phosh_notification_banner_closed (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  g_autoptr (PhoshNotification) noti = NULL;
  GtkWidget *banner = NULL;
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

  banner = phosh_notification_banner_new (noti);

  was_notified = FALSE;

  g_signal_connect (banner, "destroy", G_CALLBACK (notified), NULL);

  phosh_notification_close (noti, PHOSH_NOTIFICATION_REASON_CLOSED);

  g_assert_true (was_notified);
}


static gboolean
timeout (gpointer data)
{
  GMainLoop *loop = data;

  g_main_loop_quit (loop);

  return G_SOURCE_REMOVE;
}


static void
test_phosh_notification_banner_expired (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  g_autoptr (PhoshNotification) noti = NULL;
  g_autoptr (GMainLoop) loop = NULL;
  GtkWidget *banner = NULL;
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

  banner = phosh_notification_banner_new (noti);

  was_notified = FALSE;

  g_signal_connect (banner, "destroy", G_CALLBACK (notified), NULL);

  loop = g_main_loop_new (NULL, FALSE);

  g_timeout_add (130, timeout, loop);

  phosh_notification_expires (noti, 100);

  g_main_loop_run (loop);

  g_assert_true (was_notified);
}


int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  PHOSH_COMPOSITOR_TEST_ADD ("/phosh/notification-banner/new",
                             test_phosh_notification_banner_new);
  PHOSH_COMPOSITOR_TEST_ADD ("/phosh/notification-banner/closed",
                             test_phosh_notification_banner_closed);
  PHOSH_COMPOSITOR_TEST_ADD ("/phosh/notification-banner/expired",
                             test_phosh_notification_banner_expired);

  return g_test_run ();
}
