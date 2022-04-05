/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "config.h"
#include "phosh-screenshot-dbus.h"
#include "portal-dbus.h"
#include "shell.h"

#include "testlib-full-shell.h"
#include "testlib-calls-mock.h"
#include "testlib-mpris-mock.h"

#include "phosh-screen-saver-dbus.h"

#define BUS_NAME "org.gnome.Shell.Screenshot"
#define OBJECT_PATH "/org/gnome/Shell/Screenshot"

#define POP_TIMEOUT 50000000


static void
take_screenshot (const char *lang, int num, const char *what)
{
  g_autoptr (GError) err = NULL;
  PhoshDBusScreenshot *proxy = NULL;
  g_autofree char *used_name = NULL;
  g_autofree char *dirname = NULL;
  g_autofree char *filename = NULL;
  g_autofree char *path = NULL;
  gboolean success;

  /* libcall-ui has no idea we're picking the translations from odd
   * places so help it along */
  bindtextdomain ("call-ui", TEST_INSTALLED LOCALEDIR);

  dirname = g_build_filename (TEST_OUTPUT_DIR, "screenshots", lang, NULL);
  filename = g_strdup_printf ("screenshot-%.2d-%s.png", num, what);
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
  g_assert_finalize_object (proxy);
}


static void
on_waited (gpointer data)
{
  GMainLoop *loop = data;

  g_assert_nonnull (data);
  g_main_loop_quit (loop);
}


static void
wait_a_bit (GMainLoop *loop, int secs)
{
  g_timeout_add_seconds (secs, (GSourceFunc) on_waited, loop);
  g_main_loop_run (loop);
}


#ifdef HAVE_THUMBNAILS
static void
toggle_overview (GMainLoop                      *loop,
                 struct zwp_virtual_keyboard_v1 *keyboard,
                 GTimer                         *timer)
{
  phosh_test_keyboard_press_modifiers (keyboard, KEY_LEFTMETA);
  phosh_test_keyboard_press_keys (keyboard, timer, KEY_A, NULL);
  phosh_test_keyboard_release_modifiers (keyboard);
  /* Give animation time to finish */
  wait_a_bit (loop, 1);
}
#endif


static void
toggle_settings (GMainLoop                      *loop,
                 struct zwp_virtual_keyboard_v1 *keyboard,
                 GTimer                         *timer)
{
  phosh_test_keyboard_press_modifiers (keyboard, KEY_LEFTMETA);
  phosh_test_keyboard_press_keys (keyboard, timer, KEY_M, NULL);
  phosh_test_keyboard_release_modifiers (keyboard);
  /* Give animation time to finish */
  wait_a_bit (loop, 1);
}


static void
do_settings (void)
{
  g_autoptr (GSettings) settings = g_settings_new ("org.gnome.desktop.interface");

  g_settings_set_boolean (settings, "clock-show-date", FALSE);
  g_settings_set_boolean (settings, "clock-show-weekday", FALSE);
}


static GVariant *
get_portal_access_options (const char *icon)
{
  GVariantDict dict;

  g_variant_dict_init (&dict, NULL);
  g_variant_dict_insert_value (&dict, "icon", g_variant_new_string(icon));
  return g_variant_dict_end (&dict);
}


static void
on_portal_access_dialog (GObject      *source_object,
                         GAsyncResult *res,
                         gpointer      user_data)
{
  gboolean success;
  g_autoptr (GError) err = NULL;
  guint out;

  PhoshDBusImplPortalAccess *proxy = PHOSH_DBUS_IMPL_PORTAL_ACCESS (source_object);
  success = phosh_dbus_impl_portal_access_call_access_dialog_finish (proxy, &out, NULL, res, &err);
  g_assert_true (success);
  g_assert_no_error (err);
  *(gboolean*)user_data = success;
}


static void
test_take_screenshots (PhoshTestFullShellFixture *fixture, gconstpointer unused)
{
  struct zwp_virtual_keyboard_v1 *keyboard;
  const char *locale = g_getenv ("LANGUAGE");
  g_autoptr (GTimer) timer = g_timer_new ();
  g_autoptr (GMainContext) context = g_main_context_new ();
  g_autoptr (GMainLoop) loop = NULL;
  g_autoptr (PhoshScreenSaverDBusScreenSaver) ss_proxy = NULL;
  g_autoptr (PhoshTestCallsMock) calls_mock = NULL;
  g_autoptr (PhoshTestMprisMock) mpris_mock = NULL;
  g_autoptr (PhoshDBusImplPortalAccess) portal_access_proxy = NULL;
  g_autoptr (GVariant) options = NULL;
  g_autoptr (GError) err = NULL;
#ifdef HAVE_THUMBNAILS
  const char *argv[] = { TEST_TOOLS "/app-buttons", NULL };
  GPid pid;
#endif
  gboolean success = FALSE;
  int i = 1;

  g_assert_nonnull (locale);
  /* Wait until compositor and shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  do_settings ();

  loop = g_main_loop_new (context, FALSE);

  /* Give overview animation time to finish */
  wait_a_bit (loop, 1);
  take_screenshot (locale, i++, "overview-empty");

  keyboard = phosh_test_keyboard_new (phosh_wayland_get_default ());

  /* Give overview animation some time to finish */
  wait_a_bit (loop, 1);
  /* Typing will focus search */
  phosh_test_keyboard_press_keys (keyboard, timer, KEY_M, KEY_E, KEY_D, NULL);
  /* Give search time to finish */
  wait_a_bit (loop, 1);
  take_screenshot (locale, i++, "search");

#ifdef HAVE_THUMBNAILS
  g_spawn_async (NULL, (char**) argv, NULL, G_SPAWN_DEFAULT, NULL, NULL, &pid, &err);
  g_assert_no_error (err);
  g_assert_true (pid);
  /* Give app time to start and close overview */
  wait_a_bit (loop, 1);
  take_screenshot (locale, i++, "running-app");

  toggle_overview (loop, keyboard, timer);
  take_screenshot (locale, i++, "overview-app");
  kill (pid, SIGTERM);
  g_spawn_close_pid (pid);
#endif

  toggle_settings (loop, keyboard, timer);
  take_screenshot (locale, i++, "settings");
  toggle_settings (loop, keyboard, timer);

  portal_access_proxy = phosh_dbus_impl_portal_access_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                                              G_DBUS_PROXY_FLAGS_NONE,
                                                                              "sm.puri.Phosh.Portal",
                                                                              "/org/freedesktop/portal/desktop",
                                                                              NULL,
                                                                              &err);
  g_assert_no_error (err);
  g_clear_error (&err);
  options = get_portal_access_options ("audio-input-microphone-symbolic");
  phosh_dbus_impl_portal_access_call_access_dialog (
    portal_access_proxy,
    "/sm/puri/Phosh/Access",
    "sm.puri.Phosh",
    "",
    "Give FooBar Microphone and Storage Access?",
    "FooBar wants to use your microphone and storage.",
    "Access can be changed at any time from the privacy settings.",
    g_variant_ref_sink (options),
    NULL,
    on_portal_access_dialog,
    (gpointer)&success);
  wait_a_bit (loop, 1);
  take_screenshot (locale, i++, "portal-access");
  /* Close dialog */
  phosh_test_keyboard_press_keys (keyboard, timer, KEY_ENTER, NULL);
  wait_a_bit (loop, 1);
  g_assert_true (success);

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
  wait_a_bit (loop, 1);
  take_screenshot (locale, i++, "lockscreen-status");

  mpris_mock = phosh_test_mpris_mock_new ();
  phosh_mpris_mock_export (mpris_mock);
  wait_a_bit (loop, 1);
  take_screenshot (locale, i++, "lockscreen-media-player");

  phosh_test_keyboard_press_keys (keyboard, timer, KEY_SPACE, NULL);
  wait_a_bit (loop, 1);
  take_screenshot (locale, i++, "lockscreen-keypad");

  calls_mock = phosh_test_calls_mock_new ();
  phosh_calls_mock_export (calls_mock);
  wait_a_bit (loop, 1);
  take_screenshot (locale, i++, "lockscreen-call");

  zwp_virtual_keyboard_v1_destroy (keyboard);
}


int
main (int argc, char *argv[])
{
  g_autofree char *display = NULL;
  g_autoptr (PhoshTestFullShellFixtureCfg) cfg = NULL;

  g_test_init (&argc, &argv, NULL);

  textdomain (GETTEXT_PACKAGE);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bindtextdomain (GETTEXT_PACKAGE, TEST_INSTALLED LOCALEDIR);

  /* Preserve DISPLAY for wlroots x11 backend */
  cfg = phosh_test_full_shell_fixture_cfg_new (g_getenv ("DISPLAY"), "phosh-keyboard-events,phosh-media-player");

  g_test_add ("/phosh/tests/locale-screenshots", PhoshTestFullShellFixture, cfg,
              phosh_test_full_shell_setup, test_take_screenshots, phosh_test_full_shell_teardown);

  return g_test_run ();
}
