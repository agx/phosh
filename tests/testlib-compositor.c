/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "testlib.h"
#include "testlib-compositor.h"

/**
 * PhoshTestCompositorFixture:
 *
 * Test fixture for tests that want to run tests against a compatible
 * wayland compositor (phoc).
 */

void
phosh_test_compositor_setup (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  g_autoptr (GError) err = NULL;

  fixture->tmpdir = g_dir_make_tmp ("phosh-test-comp.XXXXXX", &err);
  g_assert_no_error (err);

  fixture->bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  g_test_dbus_up (fixture->bus);

  g_setenv ("NO_AT_BRIDGE", "1", TRUE);
  g_setenv ("XDG_RUNTIME_DIR", fixture->tmpdir, TRUE);

  fixture->state = phosh_test_compositor_new (TRUE);
  g_assert_nonnull (fixture->state);
}

void
phosh_test_compositor_teardown (PhoshTestCompositorFixture *fixture, gconstpointer unused)
{
  g_autoptr (GFile) file = g_file_new_for_path (fixture->tmpdir);

  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);

  g_clear_pointer (&fixture->state, phosh_test_compositor_free);
  g_test_dbus_down (fixture->bus);
  g_clear_object (&fixture->bus);

  phosh_test_remove_tree (file);
  g_clear_pointer (&fixture->tmpdir, g_free);
}
