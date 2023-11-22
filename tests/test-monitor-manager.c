/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "phosh-display-dbus.h"
#include "shell.h"

#include "testlib-full-shell.h"

#define BUS_NAME "org.gnome.Mutter.DisplayConfig"
#define OBJECT_PATH "/org/gnome/Mutter/DisplayConfig"

#define POP_TIMEOUT 50000000

static void
test_phosh_monitor_manager_current_state (PhoshTestFullShellFixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) err = NULL;
  g_autoptr (PhoshDBusDisplayConfig) proxy = NULL;
  g_autoptr (GVariant) monitors = NULL;
  g_autoptr (GVariant) logical = NULL;
  g_autoptr (GVariant) props = NULL;
  GVariantIter iter;
  guint serial;
  gboolean success;

  /* Wait until comp/shell are up */
  g_assert_nonnull (g_async_queue_timeout_pop (fixture->queue, POP_TIMEOUT));

  proxy = phosh_dbus_display_config_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                            G_DBUS_PROXY_FLAGS_NONE,
                                                            BUS_NAME,
                                                            OBJECT_PATH,
                                                            NULL,
                                                            &err);
  g_assert_no_error (err);
  g_assert_true (PHOSH_DBUS_IS_DISPLAY_CONFIG_PROXY (proxy));

  success = phosh_dbus_display_config_call_get_current_state_sync (proxy,
                                                                   &serial,
                                                                   &monitors,
                                                                   &logical,
                                                                   &props,
                                                                   NULL,
                                                                   &err);
  g_assert_no_error (err);
  g_assert_true (success);
  g_assert_cmpint (serial, ==, 2);

  g_variant_iter_init (&iter, monitors);
  g_assert_cmpint (g_variant_iter_n_children (&iter), ==, 1);

  g_variant_iter_init (&iter, logical);
  g_assert_cmpint (g_variant_iter_n_children (&iter), ==, 1);

  /* Check more API calls */
  g_assert_false (phosh_dbus_display_config_get_night_light_supported (proxy));
  g_assert_false (phosh_dbus_display_config_get_panel_orientation_managed (proxy));
  g_assert_true (phosh_dbus_display_config_get_apply_monitors_config_allowed (proxy));
  g_assert_cmpint (phosh_dbus_display_config_get_power_save_mode (proxy), ==, 0);
}


int
main (int argc, char *argv[])
{
  g_autoptr (PhoshTestFullShellFixtureCfg) cfg = NULL;

  g_test_init (&argc, &argv, NULL);

  cfg = phosh_test_full_shell_fixture_cfg_new ("phosh-monitor-manager");

  PHOSH_FULL_SHELL_TEST_ADD ("/phosh/dbus/monitor-manager/current_state", cfg,
                             test_phosh_monitor_manager_current_state);

  return g_test_run ();
}
