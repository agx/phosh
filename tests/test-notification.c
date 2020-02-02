/*
 * Copyright © 2020 Zander Brown
 * 
 * SPDX-License-Identifier: GPL-3.0+
 * 
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "notification.h"

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

  noti = phosh_notification_new (NULL,
                                 NULL,
                                 "Hey",
                                 "Testing",
                                 NULL,
                                 NULL,
                                 NULL);

  g_assert_cmpstr (phosh_notification_get_app_name (noti), ==, "Notification");
  g_assert_null (phosh_notification_get_app_info (noti));
  g_assert_cmpstr (phosh_notification_get_summary (noti), ==, "Hey");
  g_assert_cmpstr (phosh_notification_get_body (noti), ==, "Testing");

  icon = phosh_notification_get_app_icon (noti);
  g_assert_true (G_IS_THEMED_ICON (icon));
  g_assert_cmpstr (g_themed_icon_get_names (G_THEMED_ICON (icon))[0],
                   ==,
                   "application-x-executable");

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
}


static void
test_phosh_notification_new (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  g_autoptr (GAppInfo) info = NULL;
  GIcon *icon;
  GIcon *image;

  info = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));
  icon = g_themed_icon_new ("should-not-be-seen");

  image = g_themed_icon_new ("image-missing");

  noti = phosh_notification_new ("should-not-be-seen",
                                 info,
                                 "Hey",
                                 "Testing",
                                 icon,
                                 image,
                                 NULL);

  g_assert_cmpstr (phosh_notification_get_app_name (noti), ==, "Med");
  g_assert_true (phosh_notification_get_app_info (noti) == info);
  g_assert_cmpstr (phosh_notification_get_summary (noti), ==, "Hey");
  g_assert_cmpstr (phosh_notification_get_body (noti), ==, "Testing");

  icon = phosh_notification_get_app_icon (noti);
  g_assert_true (G_IS_THEMED_ICON (icon));
  g_assert_cmpstr (g_themed_icon_get_names (G_THEMED_ICON (icon))[0],
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


// g_strv_equal from GLib 2.60
static gboolean
strv_equal (const gchar * const *strv1,
            const gchar * const *strv2)
{
  g_return_val_if_fail (strv1 != NULL, FALSE);
  g_return_val_if_fail (strv2 != NULL, FALSE);

  if (strv1 == strv2)
    return TRUE;

  for (; *strv1 != NULL && *strv2 != NULL; strv1++, strv2++)
    {
      if (!g_str_equal (*strv1, *strv2))
        return FALSE;
    }

  return (*strv1 == NULL && *strv2 == NULL);
}


static void
test_phosh_notification_actions (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  GStrv original_actions = (char *[]) { "app.test", "Test", "app.test", "Me", NULL };
  GStrv actions;

  noti = phosh_notification_new (NULL,
                                 NULL,
                                 "Hey",
                                 "Testing",
                                 NULL,
                                 NULL,
                                 original_actions);

  actions = phosh_notification_get_actions (noti);

  g_assert_true (strv_equal ((const gchar * const*) original_actions,
                             (const gchar * const*) actions));
  
  g_signal_connect (noti, "actioned", G_CALLBACK (actioned), NULL);
  phosh_notification_activate (noti, "app.test");

  g_assert_true (actioned_called);
}


static void
test_phosh_notification_get (void)
{
  g_autoptr (PhoshNotification) noti = NULL;
  guint id = 0;
  const char *app_name = NULL;
  GAppInfo *app_info = NULL;
  const char *summary = NULL;
  const char *body = NULL;
  GIcon *app_icon = NULL;
  GIcon *image = NULL;
  GStrv actions = NULL;

  noti = phosh_notification_new (NULL,
                                 NULL,
                                 "Hey",
                                 "Testing",
                                 NULL,
                                 NULL,
                                 NULL);

  g_object_get (noti,
                "id", &id,
                "app-name", &app_name,
                "app-info", &app_info,
                "summary", &summary,
                "body", &body,
                "app-icon", &app_icon,
                "image", &image,
                "actions", &actions,
                NULL);
  
  g_assert_nonnull (app_name);
  g_assert_null (app_info);
  g_assert_nonnull (summary);
  g_assert_nonnull (body);
  g_assert_nonnull (app_icon);
  g_assert_null (image);
  g_assert_null (actions);

  g_test_expect_message ("GLib-GObject",
                         G_LOG_LEVEL_WARNING,
                         "g_object_get_is_valid_property: object class 'PhoshNotification' has no property named 'bad-prop'");
  g_object_get (noti, "bad-prop", &app_name, NULL);

  g_test_expect_message ("GLib-GObject",
                         G_LOG_LEVEL_WARNING,
                         "g_object_set_is_valid_property: object class 'PhoshNotification' has no property named 'bad-prop'");
  g_object_set (noti, "bad-prop", app_name, NULL);
}


int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/notification/new_basic", test_phosh_notification_new_basic);
  g_test_add_func ("/phosh/notification/new", test_phosh_notification_new);
  g_test_add_func ("/phosh/notification/actions", test_phosh_notification_actions);
  g_test_add_func ("/phosh/notification/get", test_phosh_notification_get);

  return g_test_run ();
}
