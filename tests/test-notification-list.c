/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "notifications/notification-list.c"


static void
test_phosh_notification_list_new (void)
{
  g_autoptr (PhoshNotificationList) list = NULL;
  g_autoptr (PhoshNotification) noti = NULL;
  g_autoptr (PhoshNotificationSource) source = NULL;
  const char *name;
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();

  list = phosh_notification_list_new ();

  noti = phosh_notification_new (456,
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

  phosh_notification_list_add (list, "org.gnome.zbrown.KingsCross", noti);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (list)), ==, 1);
  g_assert_true (g_list_model_get_item_type (G_LIST_MODEL (list)) == PHOSH_TYPE_NOTIFICATION_SOURCE);

  source = g_list_model_get_item (G_LIST_MODEL (list), 0);

  g_assert_nonnull (source);

  name = phosh_notification_source_get_name (PHOSH_NOTIFICATION_SOURCE (source));

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (source)), ==, 1);
  g_assert_true (noti == g_list_model_get_item (G_LIST_MODEL (source), 0));
  g_assert_cmpstr (name, ==, "org.gnome.zbrown.KingsCross");
}


static void
test_phosh_notification_list_get_by (void)
{
  g_autoptr (PhoshNotificationList) list = NULL;
  g_autoptr (PhoshNotification) noti = NULL;
  PhoshNotification *test_noti = NULL;
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();

  list = phosh_notification_list_new ();

  noti = phosh_notification_new (456,
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

  phosh_notification_list_add (list, "org.gnome.zbrown.KingsCross", noti);

  test_noti = phosh_notification_list_get_by_id (list, 456);

  g_assert_true (noti == test_noti);
}


static void
test_phosh_notification_list_latest_on_top (void)
{
  g_autoptr (PhoshNotificationList) list = NULL;
  g_autoptr (PhoshNotification) noti = NULL;
  g_autoptr (PhoshNotificationSource) source = NULL;
  g_autoptr (PhoshNotification) test_noti = NULL;
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();

  list = phosh_notification_list_new ();

  noti = phosh_notification_new (456,
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
  phosh_notification_list_add (list, "org.gnome.zbrown.KingsCross", noti);

  noti = phosh_notification_new (567,
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
  phosh_notification_list_add (list, "org.gnome.design.Palette", noti);

  noti = phosh_notification_new (678,
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
  phosh_notification_list_add (list, "org.gnome.zbrown.KingsCross", noti);

  noti = phosh_notification_new (789,
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
  phosh_notification_list_add (list, "org.gnome.zbrown.KingsCross", noti);

  source = g_list_model_get_item (G_LIST_MODEL (list), 0);

  g_assert_cmpstr (phosh_notification_source_get_name (source), ==, "org.gnome.zbrown.KingsCross");

  test_noti = g_list_model_get_item (G_LIST_MODEL (source), 0);

  g_assert_true (noti == test_noti);
}


static gboolean changed = FALSE;


static void
items_changed (GListModel *list,
               guint       position,
               guint       removed,
               guint       added,
               gpointer    data)
{
  g_assert_cmpuint (position, ==, 0);
  g_assert_cmpuint (removed, ==, 1);
  g_assert_cmpuint (added, ==, 0);

  changed = TRUE;
}


static void
test_phosh_notification_list_source_empty (void)
{
  g_autoptr (PhoshNotificationSource) source = NULL;
  g_autoptr (PhoshNotificationList) list = NULL;
  g_autoptr (PhoshNotification) noti = NULL;
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();

  list = phosh_notification_list_new ();

  noti = phosh_notification_new (456,
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

  phosh_notification_list_add (list, "org.gnome.zbrown.KingsCross", noti);

  source = g_list_model_get_item (G_LIST_MODEL (list), 0);
  g_assert_nonnull (source);

  changed = FALSE;

  g_signal_connect (list, "items-changed", G_CALLBACK (items_changed), NULL);
  phosh_notification_close (noti, PHOSH_NOTIFICATION_REASON_DISMISSED);

  g_assert_true (changed);

  g_assert_null (g_list_model_get_item (G_LIST_MODEL (list), 0));
}


static void
test_phosh_notification_list_seek (void)
{
  g_autoptr (PhoshNotificationSource) first = NULL;
  g_autoptr (PhoshNotificationList) list = NULL;
  g_autoptr (PhoshNotification) noti = NULL;
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();

  list = phosh_notification_list_new ();

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
  phosh_notification_list_add (list, "org.gnome.zbrown.KingsCross", noti);

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
  phosh_notification_list_add (list, "org.gnome.design.Palette", noti);

  first = g_list_model_get_item (G_LIST_MODEL (list), 0);
  g_assert_nonnull (first);

  /* Forwards */
  for (int i = 0; i < 2; i++) {
    g_autoptr (PhoshNotificationSource) item = NULL;

    item = g_list_model_get_item (G_LIST_MODEL (list), i);

    g_assert_nonnull (item);
  }

  /* Backwards */
  for (int i = 1; i >= 0; i--) {
    g_autoptr (PhoshNotificationSource) item = NULL;

    item = g_list_model_get_item (G_LIST_MODEL (list), i);

    g_assert_nonnull (item);
  }
}


int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/notification-list/new", test_phosh_notification_list_new);
  g_test_add_func ("/phosh/notification-list/get-by", test_phosh_notification_list_get_by);
  g_test_add_func ("/phosh/notification-list/latest-on-top", test_phosh_notification_list_latest_on_top);
  g_test_add_func ("/phosh/notification-list/source-empty", test_phosh_notification_list_source_empty);
  g_test_add_func ("/phosh/notification-list/seek", test_phosh_notification_list_seek);

  return g_test_run ();
}
