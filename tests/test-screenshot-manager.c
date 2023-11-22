/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-screenshot-dbus.h"
#include "shell.h"

#include "testlib-full-shell.h"

#define BUS_NAME "org.gnome.Shell.Screenshot"
#define OBJECT_PATH "/org/gnome/Shell/Screenshot"

#define POP_TIMEOUT 50000000

static void
test_phosh_screenshot_png (PhoshTestFullShellFixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (PhoshDBusScreenshot) proxy = NULL;
  g_autofree char *used_name = NULL;
  g_autofree char *dirname = NULL;
  g_autofree char *filename = NULL;
  g_autofree char *path = NULL;
  gboolean success1, success2;

  dirname = g_build_filename (TEST_OUTPUT_DIR, __func__, NULL);
  filename = g_strdup_printf ("screenshot-%d.png", g_test_rand_int ());
  path = g_build_filename (dirname, filename, NULL);
  g_assert_cmpint (g_mkdir_with_parents (dirname, 0755), ==, 0);

  /* Wait until comp/shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  proxy = phosh_dbus_screenshot_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                        G_DBUS_PROXY_FLAGS_NONE,
                                                        BUS_NAME,
                                                        OBJECT_PATH,
                                                        NULL,
                                                        &err);
  g_assert_no_error (err);
  g_assert_true (PHOSH_DBUS_IS_SCREENSHOT_PROXY (proxy));

  success1 = phosh_dbus_screenshot_call_screenshot_sync (proxy,
                                                         FALSE,
                                                         TRUE,
                                                         path,
                                                         &success2,
                                                         &used_name,
                                                         NULL,
                                                         &err);

  g_assert_no_error (err);
  g_assert_true (success1);
  g_assert_true (success2);
  g_assert_cmpstr (used_name, ==, path);
  g_test_message ("Screenshot at %s", used_name);
  g_assert_true (g_file_test (used_name, G_FILE_TEST_EXISTS));

  success1 = phosh_dbus_screenshot_call_screenshot_sync (proxy,
                                                         FALSE,
                                                         TRUE,
                                                         path,
                                                         &success2,
                                                         &used_name,
                                                         NULL,
                                                         &err);

  g_assert_no_error (err);
  g_assert_true (success1);
  /* 2nd screenshot with identical file name fails */
  g_assert_false (success2);

  g_assert_cmpint (unlink (path), ==, 0);
}


int
main (int argc, char *argv[])
{
  g_autoptr (PhoshTestFullShellFixtureCfg) cfg = NULL;

  g_test_init (&argc, &argv, NULL);

  cfg = phosh_test_full_shell_fixture_cfg_new ("phosh-screenshot-manager");

  PHOSH_FULL_SHELL_TEST_ADD ("/phosh/dbus/screenshot-manager/png", cfg, test_phosh_screenshot_png);

  return g_test_run ();
}
