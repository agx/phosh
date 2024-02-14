/*
 * Copyright © 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "notifications/mount-notification.h"
#include "stubs/bad-prop.h"

#include <gio/gdesktopappinfo.h>

#define PHOSH_TYPE_TEST_DUMMY_MOUNT (phosh_test_dummy_mount_get_type ())
G_DECLARE_FINAL_TYPE (PhoshTestDummyMount, phosh_test_dummy_mount, PHOSH, TEST_DUMMY_MOUNT, GObject)

typedef struct _PhoshTestDummyMount {
  GObject parent;

} PhoshTestDummyMount;


static char *
test_dummy_get_name (GMount *mount)
{
  return g_strdup ("dummy");
}


static GFile *
test_dummy_get_root (GMount *mount)
{
  return g_file_new_for_path ("/");
}


static void
mount_iface_init (GMountIface *iface)
{
  /* just what we need for the test */
  iface->get_root = test_dummy_get_root;
  iface->get_name = test_dummy_get_name;
}

G_DEFINE_TYPE_WITH_CODE (PhoshTestDummyMount, phosh_test_dummy_mount, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_MOUNT, mount_iface_init))

static void
phosh_test_dummy_mount_class_init (PhoshTestDummyMountClass *klass)
{
}

static void
phosh_test_dummy_mount_init (PhoshTestDummyMount *self)
{
}

static void
test_phosh_mount_notification_new (void)
{
  g_autoptr (PhoshMountNotification) mn = NULL;
  g_autoptr (GMount) mount = g_object_new (PHOSH_TYPE_TEST_DUMMY_MOUNT, NULL);
  GIcon *icon = NULL;
  g_autofree char *name = NULL;

  g_assert_true (G_IS_MOUNT (mount));
  name = g_mount_get_name (mount);
  g_debug ("mount: %s", name);

  mn = phosh_mount_notification_new_from_mount (1, mount);

  g_assert_cmpstr (phosh_notification_get_summary (PHOSH_NOTIFICATION (mn)), ==, "dummy");

  icon = phosh_notification_get_app_icon (PHOSH_NOTIFICATION (mn));
  g_assert_true (G_IS_THEMED_ICON (icon));
  g_assert_true (g_strcmp0 (g_themed_icon_get_names (G_THEMED_ICON (icon))[0],
                            "applications-system-symbolic") == 0 ||
                 g_strcmp0 (g_themed_icon_get_names (G_THEMED_ICON (icon))[0],
                            "sm.puri.Phosh") == 0);

  icon = phosh_notification_get_image (PHOSH_NOTIFICATION (mn));
  g_assert_true (G_IS_THEMED_ICON (icon));
  g_assert_cmpstr (g_themed_icon_get_names (G_THEMED_ICON (icon))[0],
                   ==,
                   "folder-remote-symbolic");
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/mount-notification/new", test_phosh_mount_notification_new);

  return g_test_run ();
}
