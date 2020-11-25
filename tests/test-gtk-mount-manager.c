/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-gtk-mountoperation-dbus.h"
#include "log.h"
#include "shell.h"

#include "testlib.h"

#include <handy.h>

#define BUS_NAME "org.gtk.MountOperationHandler"
#define OBJECT_PATH "/org/gtk/MountOperationHandler"

#define POP_TIMEOUT 50000000

typedef struct _Fixture {
  GThread                  *comp_and_shell;
  GAsyncQueue              *queue;
  PhoshTestCompositorState *state;
  struct zwp_virtual_keyboard_v1 *keyboard;
  GTimer                   *timer;
} Fixture;


static gboolean
stop_shell (gpointer unused)
{
  g_debug ("Stopping shell");
  gtk_main_quit ();

  return G_SOURCE_REMOVE;
}


static gpointer
comp_and_shell_thread (gpointer data)
{
  PhoshShell *shell;
  GLogLevelFlags flags;
  Fixture *fixture = (Fixture *)data;

  /* compositor setup in thread since this invokes gdk already */
  fixture->state = phosh_test_compositor_new ();

  /* Virtual keyboard */
  fixture->keyboard = phosh_test_keyboard_new (fixture->state->wl);
  fixture->timer = g_timer_new ();

  gtk_init (NULL, NULL);
  hdy_init ();

  phosh_log_set_log_domains ("phosh-gtk-mount-manager,phosh-gtk-mount-prompt");

  /* Drop warnings from the fatal log mask since there's plenty
   * when running without recommended DBus services */
  flags = g_log_set_always_fatal (0);
  g_log_set_always_fatal (flags & ~G_LOG_LEVEL_WARNING);

  shell = phosh_shell_get_default ();
  g_assert_true (PHOSH_IS_SHELL (shell));

  g_assert_false (phosh_shell_is_startup_finished (shell));

  /* Process events to startup shell */
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);

  g_assert_true (phosh_shell_is_startup_finished (shell));

  g_async_queue_push (fixture->queue, (gpointer)TRUE);

  gtk_main ();

  g_assert_finalize_object (shell);
  phosh_test_compositor_free (fixture->state);

  /* Process events to tear down compositor */
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);

  return NULL;
}


static void
comp_and_shell_setup (Fixture *fixture, gconstpointer unused)
{
  /* Run shell in a thread so we can sync call to the DBus interfaces */
  fixture->queue = g_async_queue_new ();
  fixture->comp_and_shell = g_thread_new ("comp-and-shell-thread", comp_and_shell_thread, fixture);
}


static void
comp_and_shell_teardown (Fixture *fixture, gconstpointer unused)
{
  gdk_threads_add_idle (stop_shell, NULL);
  g_thread_join (fixture->comp_and_shell);
  g_async_queue_unref (fixture->queue);
  g_timer_destroy (fixture->timer);
}


static void
on_ask_password_done (GObject      *source_object,
                      GAsyncResult *res,
                      gpointer      user_data)
{
  guint resp = 0;
  gboolean success;
  g_autoptr (GVariant) detail = NULL;
  g_autoptr (GVariantDict) dict = NULL;
  const gchar *password;

  g_autoptr (GError) err = NULL;
  PhoshDBusMountOperationHandler *proxy = PHOSH_DBUS_MOUNT_OPERATION_HANDLER (source_object);

  success = phosh_dbus_mount_operation_handler_call_ask_password_finish (proxy,
                                                                         &resp,
                                                                         &detail,
                                                                         res,
                                                                         &err);
  g_assert_no_error (err);
  g_assert_true (success);
  g_assert_cmpint (resp, ==, G_MOUNT_OPERATION_HANDLED);

  dict = g_variant_dict_new (detail);
  g_assert_true (g_variant_dict_lookup (dict, "password", "&s", &password));
  g_assert_cmpstr (password, ==, "abc");

  g_main_loop_quit ((GMainLoop *) user_data);

  phosh_dbus_mount_operation_handler_call_close (proxy, NULL, NULL, NULL);
}


static gboolean
ask_password_input (gpointer user_data)
{
  Fixture *fixture = (Fixture *) user_data;

  g_test_message ("Ask password input");
  phosh_test_keyboard_press_keys (fixture->keyboard,
                                  fixture->timer,
                                  KEY_A, KEY_B, KEY_C, KEY_ENTER, NULL);

  return G_SOURCE_REMOVE;
}


static void
on_ask_password_new_prompt (PhoshGtkMountManager *manager, gpointer user_data)
{
  Fixture *fixture = (Fixture *) user_data;
  g_return_if_fail (PHOSH_IS_GTK_MOUNT_MANAGER (manager));

  g_test_message ("Ask password new prompt");

  /* Process all events before closing the dialog */
  g_idle_add_full (G_PRIORITY_LOW, ask_password_input, fixture, NULL);
}


static void
test_phosh_gtk_mount_manager_ask_password (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (PhoshDBusMountOperationHandler) proxy = NULL;
  g_autoptr (GMainLoop) loop = NULL;

  /* Wait until comp/shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  proxy = phosh_dbus_mount_operation_handler_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                                     G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                                                     BUS_NAME,
                                                                     OBJECT_PATH,
                                                                     NULL,
                                                                     &err);

  g_assert_no_error (err);
  g_assert_true (PHOSH_DBUS_IS_MOUNT_OPERATION_HANDLER (proxy));

  loop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (phosh_shell_get_gtk_mount_manager (phosh_shell_get_default ()),
                    "new-prompt",
                    G_CALLBACK (on_ask_password_new_prompt),
                    fixture);

  phosh_dbus_mount_operation_handler_call_ask_password (proxy,
                                                        "p-id",
                                                        "pw-message",
                                                        "drive-harddisk",
                                                        "",
                                                        "",
                                                        G_ASK_PASSWORD_NEED_PASSWORD,
                                                        NULL,
                                                        on_ask_password_done,
                                                        loop);
  /* Run mainloop, dialog will be closed in callback */
  g_main_loop_run (loop);
}


static void
on_ask_question_done (GObject      *source_object,
                      GAsyncResult *res,
                      gpointer      user_data)
{
  guint resp = 0;
  g_autoptr (GVariant) detail = NULL;
  g_autoptr (GVariantDict) dict = NULL;
  int choice;
  gboolean success;

  g_autoptr (GError) err = NULL;
  PhoshDBusMountOperationHandler *proxy = PHOSH_DBUS_MOUNT_OPERATION_HANDLER (source_object);

  success = phosh_dbus_mount_operation_handler_call_ask_question_finish (proxy,
                                                                         &resp,
                                                                         &detail,
                                                                         res,
                                                                         &err);
  g_assert_true (success);
  g_assert_cmpint (resp, ==, G_MOUNT_OPERATION_HANDLED);
  g_assert_no_error (err);

  dict = g_variant_dict_new (detail);
  g_assert_true (g_variant_dict_lookup (dict, "choice", "i", &choice));
  g_assert_cmpint (choice, ==, 1);

  g_main_loop_quit ((GMainLoop *) user_data);
}


static gboolean
ask_question_input (gpointer user_data)
{
  Fixture *fixture = (Fixture *) user_data;

  g_test_message ("Ask password input");
  phosh_test_keyboard_press_keys (fixture->keyboard,
                                  fixture->timer,
                                  KEY_TAB, KEY_ENTER, NULL);

  return G_SOURCE_REMOVE;
}


static void
on_ask_question_new_prompt (PhoshGtkMountManager *manager, gpointer user_data)
{
  Fixture *fixture = (Fixture *) user_data;
  g_return_if_fail (PHOSH_IS_GTK_MOUNT_MANAGER (manager));

  g_test_message ("Ask question new prompt");

  /* Process all events before closing the dialog */
  g_idle_add_full (G_PRIORITY_LOW, ask_question_input, fixture, NULL);
}


static void
test_phosh_gtk_mount_manager_ask_question (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (PhoshDBusMountOperationHandler) proxy = NULL;
  g_autoptr (GMainLoop) loop = NULL;
  const char *const *choices = (const char * []) { "ok", "maybe-ok", "not-ok", NULL };

  /* Wait until comp/shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  proxy = phosh_dbus_mount_operation_handler_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                                     G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                                                     BUS_NAME,
                                                                     OBJECT_PATH,
                                                                     NULL,
                                                                     &err);

  g_assert_no_error (err);
  g_assert_true (PHOSH_DBUS_IS_MOUNT_OPERATION_HANDLER (proxy));

  loop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (phosh_shell_get_gtk_mount_manager (phosh_shell_get_default ()),
                    "new-prompt",
                    G_CALLBACK (on_ask_question_new_prompt),
                    fixture);

  phosh_dbus_mount_operation_handler_call_ask_question (proxy,
                                                        "q-id",
                                                        "question-message\ndetails",
                                                        "drive-harddisk",
                                                        choices,
                                                        NULL,
                                                        on_ask_question_done,
                                                        loop);
  g_main_loop_run (loop);
}


static void
on_show_processes_done (GObject      *source_object,
                        GAsyncResult *res,
                        gpointer      user_data)
{
  guint resp = 0;
  gboolean success;

  g_autoptr (GError) err = NULL;
  PhoshDBusMountOperationHandler *proxy = PHOSH_DBUS_MOUNT_OPERATION_HANDLER (source_object);

  success = phosh_dbus_mount_operation_handler_call_show_processes_finish (proxy,
                                                                           &resp,
                                                                           NULL,
                                                                           res,
                                                                           &err);
  g_assert_true (success);
  g_assert_cmpint (resp, ==, G_MOUNT_OPERATION_UNHANDLED);
  g_assert_no_error (err);
  g_main_loop_quit ((GMainLoop *) user_data);
}


static void
on_show_processes_new_prompt (PhoshGtkMountManager *manager, gpointer user_data)
{
  PhoshDBusMountOperationHandler *proxy = (PhoshDBusMountOperationHandler *) user_data;

  g_test_message ("New prompt");
  g_return_if_fail (PHOSH_IS_GTK_MOUNT_MANAGER (manager));

  phosh_dbus_mount_operation_handler_call_close (proxy, NULL, NULL, NULL);
}


static void
test_phosh_gtk_mount_manager_show_processes (Fixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (PhoshDBusMountOperationHandler) proxy = NULL;
  g_autoptr (GMainLoop) loop = NULL;
  GVariant *pids = g_variant_new_array (G_VARIANT_TYPE_INT32, NULL, 0);
  const char *const *choices = (const char * []) { "ok", "maybe-ok", "not-ok", NULL };

  /* Wait until comp/shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  proxy = phosh_dbus_mount_operation_handler_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                                     G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                                                     BUS_NAME,
                                                                     OBJECT_PATH,
                                                                     NULL,
                                                                     &err);

  g_assert_no_error (err);
  g_assert_true (PHOSH_DBUS_IS_MOUNT_OPERATION_HANDLER (proxy));

  loop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (phosh_shell_get_gtk_mount_manager (phosh_shell_get_default ()),
                    "new-prompt",
                    G_CALLBACK (on_show_processes_new_prompt),
                    proxy);

  phosh_dbus_mount_operation_handler_call_show_processes (proxy,
                                                          "P-id",
                                                          "process-message",
                                                          "drive-harddisk",
                                                          pids,
                                                          choices,
                                                          NULL,
                                                          on_show_processes_done,
                                                          loop);
  g_main_loop_run (loop);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/phosh/dbus/gtk-mount-manager/ask-password", Fixture, NULL,
              comp_and_shell_setup, test_phosh_gtk_mount_manager_ask_password, comp_and_shell_teardown);
  g_test_add ("/phosh/dbus/gtk-mount-manager/ask-question", Fixture, NULL,
              comp_and_shell_setup, test_phosh_gtk_mount_manager_ask_question, comp_and_shell_teardown);
  g_test_add ("/phosh/dbus/gtk-mount-manager/show-processes", Fixture, NULL,
              comp_and_shell_setup, test_phosh_gtk_mount_manager_show_processes, comp_and_shell_teardown);

  return g_test_run ();
}
