/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-config.h"
#include "phosh-gnome-shell-dbus.h"
#include "phosh-screenshot-dbus.h"
#include "phosh-test-resources.h"
#include "portal-dbus.h"
#include "shell.h"

#include "testlib-wall-clock-mock.h"
#include "testlib-full-shell.h"
#include "testlib-calls-mock.h"
#include "testlib-mpris-mock.h"
#include "testlib-emergency-calls.h"
#include "testlib-wait-for-shell-state.h"

#include "phosh-screen-saver-dbus.h"

#define BUS_NAME "org.gnome.Shell.Screenshot"
#define OBJECT_PATH "/org/gnome/Shell/Screenshot"

#define POP_TIMEOUT 50000000
#define WAIT_TIMEOUT 30000


static void
take_screenshot (const char *what, int num, const char *where)
{
  g_autoptr (GError) err = NULL;
  PhoshDBusScreenshot *proxy = NULL;
  g_autofree char *used_name = NULL;
  g_autofree char *dirname = NULL;
  g_autofree char *filename = NULL;
  g_autofree char *path = NULL;
  gboolean success;

  /* libcall-ui has no idea that we're picking the translations from odd places
   * so help it along */
  bindtextdomain ("call-ui", LOCALEDIR);

  dirname = g_build_filename (TEST_OUTPUT_DIR, "screenshots", what, NULL);
  filename = g_strdup_printf ("screenshot-%.2d-%s.png", num, where);
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
wait_a_bit (GMainLoop *loop, int msecs)
{
  gint id;

  id = g_timeout_add (msecs, (GSourceFunc) on_waited, loop);
  g_source_set_name_by_id (id, "[TestTakeScreenshot] wait");
  g_main_loop_run (loop);
}


static void
toggle_overview (GMainLoop                      *loop,
                 struct zwp_virtual_keyboard_v1 *keyboard,
                 GTimer                         *timer,
                 PhoshTestWaitForShellState     *waiter)
{
  phosh_test_keyboard_press_modifiers (keyboard, KEY_LEFTMETA);
  phosh_test_keyboard_press_keys (keyboard, timer, KEY_A, NULL);
  phosh_test_keyboard_release_modifiers (keyboard);
  phosh_test_wait_for_shell_state_wait (waiter, PHOSH_STATE_OVERVIEW, TRUE, WAIT_TIMEOUT);
  /* Give animation time to finish */
  wait_a_bit (loop, 500);
}


static void
toggle_settings (GMainLoop                      *loop,
                 struct zwp_virtual_keyboard_v1 *keyboard,
                 GTimer                         *timer,
                 PhoshTestWaitForShellState     *waiter,
                 gboolean                        should_be_visible)
{
  phosh_test_keyboard_press_modifiers (keyboard, KEY_LEFTMETA);
  phosh_test_keyboard_press_keys (keyboard, timer, KEY_M, NULL);
  phosh_test_keyboard_release_modifiers (keyboard);
  phosh_test_wait_for_shell_state_wait (waiter, PHOSH_STATE_SETTINGS, should_be_visible,
                                        WAIT_TIMEOUT);
  /* Give animation time to finish */
  wait_a_bit (loop, 500);
}


static void
activate_lockscreen_plugins (gboolean                        activate,
                             GMainLoop                      *loop,
                             struct zwp_virtual_keyboard_v1 *keyboard,
                             GTimer                         *timer)
{
  guint key = activate ? KEY_LEFT : KEY_RIGHT;

  phosh_test_keyboard_press_modifiers (keyboard, KEY_LEFTCTRL);
  phosh_test_keyboard_press_keys (keyboard, timer, key, NULL);
  phosh_test_keyboard_release_modifiers (keyboard);
  /* Give animation time to finish */
  wait_a_bit (loop, 500);
}


static void
show_run_command_dialog (GMainLoop                      *loop,
                         struct zwp_virtual_keyboard_v1 *keyboard,
                         GTimer                         *timer,
                         PhoshTestWaitForShellState     *waiter,
                         gboolean                        show)
{
  if (show) {
    phosh_test_keyboard_press_modifiers (keyboard, KEY_LEFTALT);
    phosh_test_keyboard_press_keys (keyboard, timer, KEY_F2, NULL);
    phosh_test_keyboard_release_modifiers (keyboard);
  } else {
    phosh_test_keyboard_press_keys (keyboard, timer, KEY_ESC, NULL);
  }
  phosh_test_wait_for_shell_state_wait (waiter, PHOSH_STATE_MODAL_SYSTEM_PROMPT, show,
                                        WAIT_TIMEOUT);
  /* Give animation time to finish */
  wait_a_bit (loop, 500);
  /* Even more time for powerbar fade animation to finish */
  wait_a_bit (loop, 500);
}


static void
do_settings (void)
{
  g_autoptr (GSettings) settings = g_settings_new ("org.gnome.desktop.interface");

  g_settings_set_boolean (settings, "clock-show-date", FALSE);
  g_settings_set_boolean (settings, "clock-show-weekday", FALSE);

  /* Enable emergency-calls until it's on by default */
  g_clear_object (&settings);
  settings = g_settings_new ("sm.puri.phosh.emergency-calls");
  g_settings_set_boolean (settings, "enabled", TRUE);

  /* Enable emergency-calls until it's on by default */
  g_clear_object (&settings);
  settings = g_settings_new ("sm.puri.phosh.plugins");
  g_settings_set_strv (settings, "lock-screen",
                       (const char *const[]) { "emergency-info", "launcher-box", NULL });

  /* Enable quick setting plugins */
  g_settings_set_strv (settings, "quick-settings",
                       (const char *const[]) { "caffeine-quick-setting",
                                               "simple-custom-quick-setting",
                                               NULL });
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
on_osd_finish (GObject      *source_object,
               GAsyncResult *res,
               gpointer      user_data)
{
  gboolean success;
  g_autoptr (GError) err = NULL;

  PhoshDBusGnomeShell *proxy = PHOSH_DBUS_GNOME_SHELL (source_object);
  success = phosh_dbus_gnome_shell_call_show_osd_finish (proxy, res, &err);
  g_assert_true (success);
  g_assert_no_error (err);
}


static GPid
run_plugin_prefs (void)
{
  g_autoptr (GError) err = NULL;
  const char *argv[] = { TEST_TOOLS "/plugin-prefs", NULL };
  gboolean ret;
  GPid pid;

  ret = g_spawn_async (NULL, (char**) argv, NULL, G_SPAWN_DEFAULT, NULL, NULL, &pid, &err);
  g_assert_no_error (err);
  g_assert_true (ret);
  g_assert_true (pid);

  return pid;
}


static int
screenshot_plugin_prefs (GMainLoop                      *loop,
                         const char                     *what,
                         int                             num,
                         struct zwp_virtual_keyboard_v1 *keyboard,
                         GTimer                         *timer,
                         PhoshTestWaitForShellState     *waiter)
{
  GPid pid;

  pid = run_plugin_prefs ();
  /* Wait for overview to close */
  phosh_test_wait_for_shell_state_wait (waiter, PHOSH_STATE_OVERVIEW, FALSE, WAIT_TIMEOUT);
  /* Give app time to start */
  wait_a_bit (loop, 2000);
  phosh_test_keyboard_press_modifiers (keyboard, KEY_LEFTCTRL);
  phosh_test_keyboard_press_keys (keyboard, timer, KEY_T, NULL);
  phosh_test_keyboard_release_modifiers (keyboard);
  wait_a_bit (loop, 1000);
  take_screenshot (what, num++, "plugin-prefs-ticket-box");
  phosh_test_keyboard_press_keys (keyboard, timer, KEY_ESC, NULL);
  wait_a_bit (loop, 500);
  phosh_test_keyboard_press_modifiers (keyboard, KEY_LEFTCTRL);
  phosh_test_keyboard_press_keys (keyboard, timer, KEY_E, NULL);
  phosh_test_keyboard_release_modifiers (keyboard);
  wait_a_bit (loop, 500);
  take_screenshot (what, num++, "plugin-prefs-emergncy-info");
  g_assert_no_errno (kill (pid, SIGTERM));
  g_spawn_close_pid (pid);

  /* wait for app to quit and overview to be visible again */
  phosh_test_wait_for_shell_state_wait (waiter, PHOSH_STATE_OVERVIEW, TRUE, WAIT_TIMEOUT);

  return num;
}


static void
on_end_session_dialog_open_finish (GObject      *source_object,
                                   GAsyncResult *res,
                                   gpointer      user_data)
{
  gboolean success;
  g_autoptr (GError) err = NULL;

  g_assert (PHOSH_DBUS_IS_END_SESSION_DIALOG (source_object));

  success = phosh_dbus_end_session_dialog_call_open_finish (
    PHOSH_DBUS_END_SESSION_DIALOG (source_object),
    res,
    &err);

  g_assert_no_error (err);
  g_assert_true (success);
}


static int
screenshot_end_session_dialog (GMainLoop                      *loop,
                               const char                     *what,
                               int                             num,
                               struct zwp_virtual_keyboard_v1 *keyboard,
                               GTimer                         *timer,
                               PhoshTestWaitForShellState     *waiter)
{
  g_autoptr (PhoshDBusEndSessionDialog) proxy = NULL;
  g_autoptr (GError) err = NULL;
  g_autoptr (GPtrArray) inhibitors = g_ptr_array_new_with_free_func (g_free);

  proxy = phosh_dbus_end_session_dialog_proxy_new_for_bus_sync (
    G_BUS_TYPE_SESSION,
    G_DBUS_PROXY_FLAGS_NONE,
    "org.gnome.Shell",
    "/org/gnome/SessionManager/EndSessionDialog",
    NULL,
    &err);
  g_assert_no_error (err);

  for (int i = 0; i < 10; i++) {
    char *sym = g_strdup_printf ("/org/example/foo%d", i);
    g_ptr_array_add (inhibitors, sym);
  }
  g_ptr_array_add (inhibitors, NULL);

  phosh_dbus_end_session_dialog_call_open (proxy,
                                           0,
                                           0,
                                           30,
                                           (const gchar * const*)inhibitors->pdata,
                                           NULL,
                                           on_end_session_dialog_open_finish,
                                           NULL);
  wait_a_bit (loop, 500);
  take_screenshot (what, num++, "end-session-dialog");

  phosh_test_keyboard_press_keys (keyboard, timer, KEY_ESC, NULL);
  phosh_test_wait_for_shell_state_wait (waiter, PHOSH_STATE_MODAL_SYSTEM_PROMPT, FALSE,
                                        WAIT_TIMEOUT);
  wait_a_bit (loop, 500);

  return num;
}


static int
screenshot_osd (GMainLoop *loop, const char *what, int num, PhoshTestWaitForShellState *waiter)
{
  g_autoptr (PhoshDBusGnomeShell) proxy = NULL;
  g_autoptr (GError) err = NULL;
  GVariantBuilder builder;

  proxy = phosh_dbus_gnome_shell_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                         G_DBUS_PROXY_FLAGS_NONE,
                                                         "org.gnome.Shell",
                                                         "/org/gnome/Shell",
                                                         NULL,
                                                         &err);
  g_assert_no_error (err);
  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_add (&builder, "{sv}", "connector",
                         g_variant_new_string ("DSI-1"));
  g_variant_builder_add (&builder, "{sv}", "label",
                         g_variant_new_string ("HDMI / DisplayPort"));
  g_variant_builder_add (&builder, "{sv}", "icon",
                         g_variant_new_string ("audio-volume-medium-symbolic"));
  g_variant_builder_add (&builder, "{sv}", "level",
                         g_variant_new_double (0.5));
  phosh_dbus_gnome_shell_call_show_osd (proxy,
                                        g_variant_builder_end (&builder),
                                        NULL,
                                        on_osd_finish,
                                        NULL);
  g_assert_no_error (err);
  take_screenshot (what, num++, "osd");

  /* wait for OSD to clear */
  phosh_test_wait_for_shell_state_wait (waiter, PHOSH_STATE_MODAL_SYSTEM_PROMPT, FALSE,
                                        WAIT_TIMEOUT);
  wait_a_bit (loop, 500);

  return num;
}


static int
screenshot_portal_access (GMainLoop                      *loop,
                          const char                     *what,
                          int                             num,
                          struct zwp_virtual_keyboard_v1 *keyboard,
                          GTimer                         *timer,
                          PhoshTestWaitForShellState     *waiter)
{
  g_autoptr (PhoshDBusImplPortalAccess) proxy = NULL;
  g_autoptr (GVariant) options = NULL;
  g_autoptr (GError) err = NULL;
  gboolean success = FALSE;

  proxy = phosh_dbus_impl_portal_access_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                                G_DBUS_PROXY_FLAGS_NONE,
                                                                "sm.puri.Phosh.Portal",
                                                                "/org/freedesktop/portal/desktop",
                                                                NULL,
                                                                &err);
  g_assert_no_error (err);
  options = get_portal_access_options ("audio-input-microphone-symbolic");
  phosh_dbus_impl_portal_access_call_access_dialog (
    proxy,
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
  wait_a_bit (loop, 500);
  take_screenshot (what, num++, "portal-access");
  /* Close dialog */
  phosh_test_keyboard_press_keys (keyboard, timer, KEY_ENTER, NULL);
  phosh_test_wait_for_shell_state_wait (waiter, PHOSH_STATE_MODAL_SYSTEM_PROMPT, FALSE,
                                        WAIT_TIMEOUT);
  wait_a_bit (loop, 500);
  g_assert_true (success);

  return num;
}


static void
on_ask_question_finish (GObject      *source_object,
                        GAsyncResult *res,
                        gpointer      user_data)
{
  gboolean success;
  g_autoptr (GError) err = NULL;
  g_autoptr (GVariant) details = NULL;
  guint response;

  g_assert (PHOSH_DBUS_IS_MOUNT_OPERATION_HANDLER (source_object));

  success = phosh_dbus_mount_operation_handler_call_ask_question_finish (
    PHOSH_DBUS_MOUNT_OPERATION_HANDLER (source_object),
    &response,
    &details,
    res,
    &err);

  g_assert_no_error (err);
  g_assert_true (success);
  g_assert_cmpint (response, ==, 0);
}



static int
screenshot_mount_prompt (GMainLoop                      *loop,
                         const char                     *what,
                         int                             num,
                         struct zwp_virtual_keyboard_v1 *keyboard,
                         GTimer                         *timer)
{
  g_autoptr (PhoshDBusMountOperationHandler) proxy = NULL;
  g_autoptr (GError) err = NULL;
  const char *choices[] = { "Yes", "Maybe", NULL };

  proxy = phosh_dbus_mount_operation_handler_proxy_new_for_bus_sync (
    G_BUS_TYPE_SESSION,
    G_DBUS_PROXY_FLAGS_NONE,
    "org.gtk.MountOperationHandler",
    "/org/gtk/MountOperationHandler",
    NULL,
    &err);
  g_assert_no_error (err);

  phosh_dbus_mount_operation_handler_call_ask_question (
    proxy,
    "OpId0q",
    "What do you want to do?\nThere's so many questions.",
    "drive-harddisk",
    choices,
    NULL,
    on_ask_question_finish,
    NULL);

  wait_a_bit (loop, 500);
  take_screenshot (what, num++, "mount-prompt");

  phosh_test_keyboard_press_keys (keyboard, timer, KEY_ENTER, NULL);
  wait_a_bit (loop, 500);

  return num;
}

static int
screenshot_emergency_calls (GMainLoop                      *loop,
                            const char                     *locale,
                            int                             num,
                            struct zwp_virtual_keyboard_v1 *keyboard,
                            GTimer                         *timer)
{
  g_autoptr (PhoshTestEmergencyCallsMock) emergency_calls_mock = NULL;

  emergency_calls_mock = phosh_test_emergency_calls_mock_new ();
  phosh_test_emergency_calls_mock_export (emergency_calls_mock);

  phosh_test_keyboard_press_timeout (keyboard, timer, KEY_POWER, 3000);
  wait_a_bit (loop, 500);
  take_screenshot (locale, num++, "power-menu");

  phosh_test_keyboard_press_keys (keyboard, timer, KEY_TAB, KEY_TAB, KEY_ENTER, NULL);
  phosh_test_keyboard_release_modifiers (keyboard);
  wait_a_bit (loop, 500);
  take_screenshot (locale, num++, "emergency-dialpad");

  phosh_test_keyboard_press_modifiers (keyboard, KEY_LEFTALT);
  phosh_test_keyboard_press_keys (keyboard, timer, KEY_C, NULL);
  phosh_test_keyboard_release_modifiers (keyboard);
  wait_a_bit (loop, 1000);
  take_screenshot (locale, num++, "emergency-contacts");

  phosh_test_keyboard_press_keys (keyboard, timer, KEY_ESC, NULL);
  wait_a_bit (loop, 500);

  return num;
}


static void
test_take_screenshots (PhoshTestFullShellFixture *fixture, gconstpointer unused)
{
  struct zwp_virtual_keyboard_v1 *keyboard;
  const char *what = g_getenv ("PHOSH_TEST_TYPE");
  g_autoptr (GTimer) timer = g_timer_new ();
  g_autoptr (GMainContext) context = g_main_context_new ();
  g_autoptr (GMainLoop) loop = NULL;
  g_autoptr (PhoshDBusDisplayConfig) dc_proxy = NULL;
  g_autoptr (PhoshDBusScreenSaver) ss_proxy = NULL;
  g_autoptr (PhoshTestCallsMock) calls_mock = NULL;
  g_autoptr (PhoshTestMprisMock) mpris_mock = NULL;
  g_autoptr (GError) err = NULL;
  g_autoptr (PhoshTestWaitForShellState) waiter = NULL;

  const char *argv[] = { TEST_TOOLS "/app-buttons", NULL };
  GPid pid;
  int i = 1;

  g_assert_nonnull (what);
  /* Wait until compositor and shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  loop = g_main_loop_new (context, FALSE);
  waiter = phosh_test_wait_for_shell_state_new (phosh_shell_get_default ());

  /* Give overview animation time to finish */
  phosh_test_wait_for_shell_state_wait (waiter, PHOSH_STATE_SETTINGS, FALSE, WAIT_TIMEOUT);
  wait_a_bit (loop, 500);
  take_screenshot (what, i++, "overview-empty");

  keyboard = phosh_test_keyboard_new (phosh_wayland_get_default ());

  /* Give overview animation some time to finish */
  wait_a_bit (loop, 500);
  /* Typing will focus search */
  phosh_test_keyboard_press_keys (keyboard, timer, KEY_M, KEY_E, KEY_D, NULL);
  /* Give search time to finish */
  wait_a_bit (loop, 500);
  take_screenshot (what, i++, "search");

  g_spawn_async (NULL, (char**) argv, NULL, G_SPAWN_DEFAULT, NULL, NULL, &pid, &err);
  g_assert_no_error (err);
  g_assert_true (pid);
  /* Wait for overview to close */
  phosh_test_wait_for_shell_state_wait (waiter, PHOSH_STATE_OVERVIEW, FALSE, WAIT_TIMEOUT);
  /* Give app time to start */
  wait_a_bit (loop, 500);
  take_screenshot (what, i++, "running-app");

  toggle_overview (loop, keyboard, timer, waiter);
  take_screenshot (what, i++, "overview-app");
  kill (pid, SIGTERM);
  g_spawn_close_pid (pid);

  i = screenshot_plugin_prefs (loop, what, i, keyboard, timer, waiter);

  show_run_command_dialog (loop, keyboard, timer, waiter, TRUE);
  take_screenshot (what, i++, "run-command");
  show_run_command_dialog (loop, keyboard, timer, waiter, FALSE);

  toggle_settings (loop, keyboard, timer, waiter, TRUE);
  take_screenshot (what, i++, "settings");
  toggle_settings (loop, keyboard, timer, waiter, FALSE);


  i = screenshot_osd (loop, what, i, waiter);
  i = screenshot_portal_access (loop, what, i, keyboard, timer, waiter);
  i = screenshot_end_session_dialog (loop, what, i, keyboard, timer, waiter);
  i = screenshot_mount_prompt (loop, what, i, keyboard, timer);

  ss_proxy = phosh_dbus_screen_saver_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                             G_DBUS_PROXY_FLAGS_NONE,
                                                             "org.gnome.ScreenSaver",
                                                             "/org/gnome/ScreenSaver",
                                                             NULL,
                                                             &err);
  g_assert_no_error (err);
  phosh_dbus_screen_saver_call_lock_sync (ss_proxy, NULL, &err);
  g_assert_no_error (err);

  dc_proxy = phosh_dbus_display_config_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                               G_DBUS_PROXY_FLAGS_NONE,
                                                               "org.gnome.Mutter.DisplayConfig",
                                                               "/org/gnome/Mutter/DisplayConfig",
                                                               NULL,
                                                               &err);
  wait_a_bit (loop, 1000);
  phosh_dbus_display_config_set_power_save_mode (dc_proxy, 0);
  wait_a_bit (loop, 500);
  take_screenshot (what, i++, "lockscreen-status");

  activate_lockscreen_plugins (TRUE, loop,  keyboard,timer);
  take_screenshot (what, i++, "lockscreen-plugins");
  activate_lockscreen_plugins (FALSE, loop,  keyboard,timer);

  mpris_mock = phosh_test_mpris_mock_new ();
  phosh_mpris_mock_export (mpris_mock);
  wait_a_bit (loop, 500);
  take_screenshot (what, i++, "lockscreen-media-player");

  phosh_test_keyboard_press_keys (keyboard, timer, KEY_SPACE, NULL);
  wait_a_bit (loop, 500);
  take_screenshot (what, i++, "lockscreen-keypad");

  i = screenshot_emergency_calls (loop, what, i, keyboard, timer);

  calls_mock = phosh_test_calls_mock_new ();
  phosh_calls_mock_export (calls_mock);
  wait_a_bit (loop, 500);
  take_screenshot (what, i++, "lockscreen-call");

  zwp_virtual_keyboard_v1_destroy (keyboard);
}


int
main (int argc, char *argv[])
{
  g_autoptr (PhoshTestFullShellFixtureCfg) cfg = NULL;

  g_test_init (&argc, &argv, NULL);

  textdomain (GETTEXT_PACKAGE);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  phosh_test_register_resource ();

  do_settings ();

  cfg = phosh_test_full_shell_fixture_cfg_new ("phosh-keyboard-events,phosh-media-player");

  g_test_add ("/phosh/tests/take-screenshots", PhoshTestFullShellFixture, cfg,
              phosh_test_full_shell_setup, test_take_screenshots, phosh_test_full_shell_teardown);

  return g_test_run ();
}
