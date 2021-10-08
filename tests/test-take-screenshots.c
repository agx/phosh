/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "config.h"
#include "phosh-screenshot-dbus.h"
#include "shell.h"

#include "testlib-full-shell.h"
#include "testlib-calls-mock.h"

#include "phosh-screen-saver-dbus.h"

#define BUS_NAME "org.gnome.Shell.Screenshot"
#define OBJECT_PATH "/org/gnome/Shell/Screenshot"

#define POP_TIMEOUT 50000000


static void
take_screenshot (const gchar *lang, const gchar *what)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (PhoshDBusScreenshot) proxy = NULL;
  g_autofree char *used_name = NULL;
  g_autofree char *dirname = NULL;
  g_autofree char *filename = NULL;
  g_autofree char *path = NULL;
  gboolean success;

  dirname = g_build_filename (TEST_OUTPUT_DIR, "screenshots", lang, NULL);
  filename = g_strdup_printf ("screenshot-%s.png", what);
  path = g_build_filename (dirname, filename, NULL);
  g_assert_cmpint (g_mkdir_with_parents (dirname, 0755), ==, 0);
  if (g_file_test (path, G_FILE_TEST_EXISTS))
    g_assert_false (unlink (path));

  proxy = phosh_dbus_screenshot_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                        G_DBUS_PROXY_FLAGS_NONE,
                                                        BUS_NAME,
                                                        OBJECT_PATH,
                                                        NULL,
                                                        &err);
  g_assert_no_error (err);
  g_assert_true (PHOSH_DBUS_IS_SCREENSHOT_PROXY (proxy));

  success = phosh_dbus_screenshot_call_screenshot_sync (proxy,
                                                        TRUE,
                                                        TRUE,
                                                        path,
                                                        &success,
                                                        &used_name,
                                                        NULL,
                                                        &err);

  g_assert_no_error (err);
  g_assert_true (success);
  g_assert_cmpstr (used_name, ==, path);
  g_test_message ("Screenshot at %s", used_name);
  g_assert_true (g_file_test (used_name, G_FILE_TEST_EXISTS));
}


static void
on_waited (gpointer data)
{
  g_assert_nonnull (data);
  g_main_loop_quit ((GMainLoop *)data);
}


static void
wait_a_bit (GMainContext *context, int secs)
{
  g_autoptr (GMainLoop) loop = NULL;

  loop = g_main_loop_new (context, FALSE);

  g_timeout_add_seconds (secs, (GSourceFunc) on_waited, loop);
  g_main_loop_run (loop);
}


static void
toggle_overview (GMainContext                   *context,
                 struct zwp_virtual_keyboard_v1 *keyboard,
                 GTimer                         *timer)
{
  phosh_test_keyboard_press_modifiers (keyboard, KEY_LEFTMETA);
  phosh_test_keyboard_press_keys (keyboard, timer, KEY_A, NULL);
  phosh_test_keyboard_release_modifiers (keyboard);
  /* Give animation time to finish */
  wait_a_bit (context, 1);
}


static void
toggle_settings (GMainContext                   *context,
                 struct zwp_virtual_keyboard_v1 *keyboard,
                 GTimer                         *timer)
{
  phosh_test_keyboard_press_modifiers (keyboard, KEY_LEFTMETA);
  phosh_test_keyboard_press_keys (keyboard, timer, KEY_M, NULL);
  phosh_test_keyboard_release_modifiers (keyboard);
  /* Give animation time to finish */
  wait_a_bit (context, 1);
}


static void
do_settings (void)
{
  g_autoptr (GSettings) settings = g_settings_new ("org.gnome.desktop.interface");

  g_settings_set_boolean (settings, "clock-show-date", FALSE);
  g_settings_set_boolean (settings, "clock-show-weekday", FALSE);
}


static void
test_take_screenshots (PhoshTestFullShellFixture *fixture, gconstpointer unused)
{
  struct zwp_virtual_keyboard_v1 *keyboard;
  const char *locale = g_getenv ("LANGUAGE");
  g_autoptr (GTimer) timer = g_timer_new ();
  g_autoptr (GMainContext) context = g_main_context_new ();
  g_autoptr (PhoshScreenSaverDBusScreenSaver) ss_proxy = NULL;
  g_autoptr (PhoshTestCallsMock) calls_mock = NULL;
  g_autoptr (GError) err = NULL;
  const char *argv[] = { TEST_TOOLS "/app-buttons", NULL };
  GPid pid;

  g_assert_nonnull (locale);
  /* Wait until compositor and shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  do_settings ();

  /* Give overview animation time to finish */
  wait_a_bit (context, 1);
  take_screenshot (locale, "01-overview-empty");

  keyboard = phosh_test_keyboard_new (phosh_wayland_get_default ());

  /* Give overview animation some time to finish */
  wait_a_bit (context, 1);
  /* Typing will focus search */
  phosh_test_keyboard_press_keys (keyboard, timer, KEY_M, KEY_E, KEY_D, NULL);
  /* Give search time to finish */
  wait_a_bit (context, 1);
  take_screenshot (locale, "02-search");

  g_spawn_async (NULL, (char**) argv, NULL, G_SPAWN_DEFAULT, NULL, NULL, &pid, &err);
  g_assert_no_error (err);
  g_assert_true (pid);
  /* Give app time to start and close overview */
  wait_a_bit (context, 1);
  take_screenshot (locale, "03-running-app");

  toggle_overview (context, keyboard, timer);
  take_screenshot (locale, "04-overview-app");
  kill (pid, SIGTERM);
  g_spawn_close_pid (pid);

  toggle_settings (context, keyboard, timer);
  take_screenshot (locale, "05-settings");

  ss_proxy = phosh_screen_saver_dbus_screen_saver_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                                          G_DBUS_PROXY_FLAGS_NONE,
                                                                          "org.gnome.ScreenSaver",
                                                                          "/org/gnome/ScreenSaver",
                                                                          NULL,
                                                                          &err);
  g_assert_no_error (err);
  g_clear_error (&err);
  phosh_screen_saver_dbus_screen_saver_call_lock_sync (ss_proxy, NULL, &err);
  g_assert_no_error (err);
  wait_a_bit (context, 1);
  take_screenshot (locale, "06-lockscreen");

  calls_mock = phosh_test_calls_mock_new ();
  phosh_calls_mock_export (calls_mock);
  wait_a_bit (context, 1);
  take_screenshot (locale, "07-lockscreen-call");
}


int
main (int argc, char *argv[])
{
  g_autofree gchar *display = NULL;
  g_autoptr (PhoshTestFullShellFixtureCfg) cfg = NULL;

  g_test_init (&argc, &argv, NULL);

  textdomain (GETTEXT_PACKAGE);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bindtextdomain (GETTEXT_PACKAGE, TEST_INSTALLED LOCALEDIR);

  /* Preserve DISPLAY for wlroots x11 backend */
  cfg = phosh_test_full_shell_fixture_cfg_new (g_getenv ("DISPLAY"), "phosh-keyboard-events");

  g_test_add ("/phosh/tests/locale-screenshots", PhoshTestFullShellFixture, cfg,
              phosh_test_full_shell_setup, test_take_screenshots, phosh_test_full_shell_teardown);

  return g_test_run ();
}
