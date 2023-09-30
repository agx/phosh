/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "notifications/notification.c"
#include "stubs/bad-prop.h"

#include <gio/gdesktopappinfo.h>


static gboolean was_notified = FALSE;


static void
notified (GObject *source, GParamSpec *param, gpointer data)
{
  was_notified = TRUE;
}


static void
test_phosh_notification_new_basic (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  GIcon *icon;
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();
  g_autoptr (GDateTime) now2 = g_date_time_new_now_local ();

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

  g_assert_cmpstr (phosh_notification_get_app_name (noti), ==, "Notification");
  g_assert_null (phosh_notification_get_app_info (noti));
  g_assert_cmpstr (phosh_notification_get_summary (noti), ==, "Hey");
  g_assert_cmpstr (phosh_notification_get_body (noti), ==, "Testing");

  icon = phosh_notification_get_app_icon (noti);
  g_assert_true (G_IS_THEMED_ICON (icon));
  g_assert_cmpstr (g_themed_icon_get_names (G_THEMED_ICON (icon))[0],
                   ==,
                   PHOSH_APP_UNKNOWN_ICON);

  g_assert_null (phosh_notification_get_image (noti));
  g_assert_null (phosh_notification_get_actions (noti));

  phosh_notification_set_id (noti, 123);

  g_assert_cmpint (phosh_notification_get_id (noti), ==, 123);

  was_notified = FALSE;
  g_signal_connect (noti, "notify::id", G_CALLBACK (notified), NULL);

  g_object_set (noti, "id", 42, NULL);

  g_assert_true (was_notified);
  g_assert_cmpint (phosh_notification_get_id (noti), ==, 42);

  was_notified = FALSE;

  g_object_set (noti, "id", 42, NULL);

  g_assert_false (was_notified);

  g_signal_handlers_disconnect_by_func (noti, G_CALLBACK (notified), NULL);


  was_notified = FALSE;
  g_signal_connect (noti, "notify::summary", G_CALLBACK (notified), NULL);

  g_object_set (noti, "summary", "Hey", NULL);

  g_assert_false (was_notified);

  g_signal_handlers_disconnect_by_func (noti, G_CALLBACK (notified), NULL);


  was_notified = FALSE;
  g_signal_connect (noti, "notify::body", G_CALLBACK (notified), NULL);

  g_object_set (noti, "body", "Testing", NULL);

  g_assert_false (was_notified);

  g_signal_handlers_disconnect_by_func (noti, G_CALLBACK (notified), NULL);


  g_object_set (noti, "app-name", "Blah", NULL);

  was_notified = FALSE;
  g_signal_connect (noti, "notify::app-name", G_CALLBACK (notified), NULL);

  g_object_set (noti, "app-name", "Blah", NULL);

  g_assert_false (was_notified);

  g_signal_handlers_disconnect_by_func (noti, G_CALLBACK (notified), NULL);

  was_notified = FALSE;
  g_signal_connect (noti, "notify::timestamp", G_CALLBACK (notified), NULL);

  g_object_set (noti, "timestamp", now, NULL);

  g_assert_false (was_notified);

  was_notified = FALSE;
  g_signal_connect (noti, "notify::timestamp", G_CALLBACK (notified), NULL);

  g_object_set (noti, "timestamp", now, NULL);

  g_assert_false (was_notified);

  was_notified = FALSE;
  g_signal_connect (noti, "notify::timestamp", G_CALLBACK (notified), NULL);

  g_object_set (noti, "timestamp", now2, NULL);

  g_assert_true (was_notified);


  g_signal_handlers_disconnect_by_func (noti, G_CALLBACK (notified), NULL);
}


static void
test_phosh_notification_new (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  g_autoptr (GAppInfo) info = NULL;
  g_autoptr (GIcon) icon = NULL;
  g_autoptr (GIcon) image = NULL;
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();
  GIcon *icon2;

  info = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));
  icon = g_themed_icon_new ("should-not-be-seen");

  image = g_themed_icon_new ("image-missing");

  noti = phosh_notification_new (0,
                                 "should-not-be-seen",
                                 info,
                                 "Hey",
                                 "Testing",
                                 icon,
                                 image,
                                 PHOSH_NOTIFICATION_URGENCY_NORMAL,
                                 NULL,
                                 FALSE,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 now);

  g_assert_cmpstr (phosh_notification_get_app_name (noti), ==, "Med");
  g_assert_true (phosh_notification_get_app_info (noti) == info);
  g_assert_cmpstr (phosh_notification_get_summary (noti), ==, "Hey");
  g_assert_cmpstr (phosh_notification_get_body (noti), ==, "Testing");

  icon2 = phosh_notification_get_app_icon (noti);
  g_assert_true (G_IS_THEMED_ICON (icon2));
  g_assert_cmpstr (g_themed_icon_get_names (G_THEMED_ICON (icon2))[0],
                   ==,
                   "org.gnome.zbrown.Med");

  g_assert_true (phosh_notification_get_image (noti) == image);
  g_assert_null (phosh_notification_get_actions (noti));
}


static gboolean actioned_called = FALSE;


static void
actioned (PhoshNotification *noti,
          const char        *action)
{
  actioned_called = TRUE;

  g_assert_cmpstr (action, ==, "app.test");
}


static void
test_phosh_notification_actions (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  GStrv original_actions = (char *[]) { "app.test", "Test", "app.test", "Me", NULL };
  GStrv actions;
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();

  noti = phosh_notification_new (0,
                                 NULL,
                                 NULL,
                                 "Hey",
                                 "Testing",
                                 NULL,
                                 NULL,
                                 PHOSH_NOTIFICATION_URGENCY_NORMAL,
                                 original_actions,
                                 FALSE,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 now);

  actions = phosh_notification_get_actions (noti);

  g_assert_true (g_strv_equal ((const char *const *) original_actions,
                               (const char *const *) actions));

  g_signal_connect (noti, "actioned", G_CALLBACK (actioned), NULL);
  phosh_notification_activate (noti, "app.test");

  g_assert_true (actioned_called);
}


static void
test_phosh_notification_get (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  guint id = 0;
  g_autofree char *app_name = NULL;
  GAppInfo *app_info = NULL;
  g_autofree char *summary = NULL;
  g_autofree char *body = NULL;
  g_autoptr (GIcon) app_icon = NULL;
  g_autoptr (GIcon) image = NULL;
  g_auto (GStrv) actions = NULL;
  gboolean transient = FALSE;
  gboolean resident = FALSE;
  g_autofree char *category = NULL;
  g_autofree char *profile = NULL;
  PhoshNotificationUrgency urgency = PHOSH_NOTIFICATION_URGENCY_NORMAL;
  g_autoptr (GDateTime) timestamp = NULL;
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();

  noti = phosh_notification_new (123,
                                 NULL,
                                 NULL,
                                 "Hey",
                                 "Testing",
                                 NULL,
                                 NULL,
                                 PHOSH_NOTIFICATION_URGENCY_CRITICAL,
                                 NULL,
                                 TRUE,
                                 TRUE,
                                 "email.arrived",
                                 "none",
                                 now);

  g_object_get (noti,
                "id", &id,
                "app-name", &app_name,
                "app-info", &app_info,
                "summary", &summary,
                "body", &body,
                "app-icon", &app_icon,
                "image", &image,
                "actions", &actions,
                "transient", &transient,
                "resident", &resident,
                "category", &category,
                "profile", &profile,
                "urgency", &urgency,
                "timestamp", &timestamp,
                NULL);

  g_assert_cmpuint (id, ==, 123);
  g_assert_nonnull (app_name);
  g_assert_null (app_info);
  g_assert_nonnull (summary);
  g_assert_nonnull (body);
  g_assert_nonnull (app_icon);
  g_assert_null (image);
  g_assert_null (actions);
  g_assert_true (transient);
  g_assert_true (resident);
  g_assert_cmpstr (category, ==, "email.arrived");
  g_assert_cmpstr (profile, ==, "none");
  g_assert_cmpint (urgency, ==, PHOSH_NOTIFICATION_URGENCY_CRITICAL);
  g_assert_nonnull (timestamp);
  g_assert_true (timestamp == now);
}


static gboolean did_expire = FALSE;


static void
noti_expired (PhoshNotification *self, gpointer data)
{
  GMainLoop *loop = data;

  g_main_loop_quit (loop);

  did_expire = TRUE;
}



static gboolean
timeout (gpointer data)
{
  GMainLoop *loop = data;

  g_main_loop_quit (loop);

  g_test_fail ();

  return G_SOURCE_REMOVE;
}


static void
test_phosh_notification_expires (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  g_autoptr (GMainLoop) loop = NULL;
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

  loop = g_main_loop_new (NULL, FALSE);

  g_timeout_add (130, timeout, loop);

  did_expire = TRUE;

  g_signal_connect (noti, "expired", G_CALLBACK (noti_expired), loop);

  phosh_notification_expires (noti, 100);

  g_main_loop_run (loop);

  g_assert_true (did_expire);

  /* Set it to expire in the future */
  phosh_notification_expires (noti, 100000);

  /* Kill the object taking the timeout with it */
  g_clear_object (&noti);
}


static void
test_phosh_notification_close (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
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

  /* Set it to expire in the future */
  phosh_notification_expires (noti, 1000);

  /* Send a close event, kills any timeout */
  phosh_notification_close (noti, PHOSH_NOTIFICATION_REASON_CLOSED);
}

static void
test_phosh_notification_set_prop_invalid (void)
{
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
  BAD_PROP_SET (noti, phosh_notification, PhoshNotification);
}


static void
test_phosh_notification_get_prop_invalid (void)
{
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
  BAD_PROP_GET (noti, phosh_notification, PhoshNotification);
}


int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/notification/new_basic", test_phosh_notification_new_basic);
  g_test_add_func ("/phosh/notification/new", test_phosh_notification_new);
  g_test_add_func ("/phosh/notification/actions", test_phosh_notification_actions);
  g_test_add_func ("/phosh/notification/get", test_phosh_notification_get);
  g_test_add_func ("/phosh/notification/expires", test_phosh_notification_expires);
  g_test_add_func ("/phosh/notification/close", test_phosh_notification_close);
  g_test_add_func ("/phosh/notification/get_prop_invalid", test_phosh_notification_get_prop_invalid);
  g_test_add_func ("/phosh/notification/set_prop_invalid", test_phosh_notification_set_prop_invalid);

  return g_test_run ();
}
