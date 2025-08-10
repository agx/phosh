/*
 * Copyright © 2025 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Gotam Gorabh <gautamy672@gmail.com>
 */

#include "searchd/search-provider.c"
#include <gio/gio.h>

#define DESKTOP_APP_ID "org.gnome.Phosh.desktop"
#define BUS_NAME "org.gnome.Phosh.MockSearchProvider"
#define BUS_PATH "/org/gnome/Phosh/MockSearchProvider"

typedef struct {
  PhoshSearchProvider *provider;
  GMainLoop *mainloop;
  gboolean initial_finished;
  gboolean subsearch_finished;
  gboolean got_metas_finished;
} TestFixture;

GStrv initial_results = NULL;
GStrv final_results = NULL;
GPtrArray *result_metas = NULL;


static void
fixture_setup (TestFixture *fixture, gconstpointer unused)
{
  fixture->provider = phosh_search_provider_new (DESKTOP_APP_ID,
                                                 NULL,
                                                 BUS_PATH,
                                                 BUS_NAME,
                                                 TRUE,
                                                 FALSE);

  g_assert_null (fixture->mainloop);
  fixture->mainloop = g_main_loop_new (NULL, FALSE);
}


static void
fixture_teardown (TestFixture *fixture, gconstpointer unused)
{
  g_clear_object (&fixture->provider);
  g_clear_pointer (&fixture->mainloop, g_main_loop_unref);
}


static void
got_initial_results (GObject *source, GAsyncResult *res, gpointer user_data)
{
  TestFixture *fixture = user_data;
  g_autoptr (GError) error = NULL;

  initial_results = phosh_search_provider_get_initial_finish (PHOSH_SEARCH_PROVIDER (source),
                                                              res,
                                                              &error);

  g_assert_no_error (error);

  fixture->initial_finished = TRUE;
  g_main_loop_quit (fixture->mainloop);
}


static void
got_subsearch_results (GObject *source, GAsyncResult *res, gpointer user_data)
{
  TestFixture *fixture = user_data;
  g_autoptr (GError) error = NULL;

  final_results = phosh_search_provider_get_subsearch_finish (PHOSH_SEARCH_PROVIDER (source),
                                                              res,
                                                              &error);

  g_assert_no_error (error);

  fixture->subsearch_finished = TRUE;
  g_main_loop_quit (fixture->mainloop);
}


static void
got_result_metas (GObject *source, GAsyncResult *res, gpointer user_data)
{
  TestFixture *fixture = user_data;
  g_autoptr (GError) error = NULL;

  result_metas = phosh_search_provider_get_result_meta_finish (PHOSH_SEARCH_PROVIDER (source),
                                                               res,
                                                               &error);

  g_assert_no_error (error);

  fixture->got_metas_finished = TRUE;
  g_main_loop_quit (fixture->mainloop);
}


static void
test_phosh_search_provider_new (TestFixture *fixture, gconstpointer unused)
{
  g_signal_connect_swapped (fixture->provider, "ready", (GCallback)g_main_loop_quit, fixture->mainloop);
  g_main_loop_run (fixture->mainloop);

  g_assert_nonnull (fixture->provider);
  g_assert_true (phosh_search_provider_get_ready (fixture->provider));
}


static void
test_phosh_search_provider_get_bus_path (TestFixture *fixture, gconstpointer unused)
{
  g_signal_connect_swapped (fixture->provider, "ready", (GCallback)g_main_loop_quit, fixture->mainloop);
  g_main_loop_run (fixture->mainloop);

  g_assert_nonnull (fixture->provider);
  g_assert_cmpstr (phosh_search_provider_get_bus_path (fixture->provider),
                   ==,
                   "/org/gnome/Phosh/MockSearchProvider");

}


static void
test_phosh_search_provider_get_initial_async (TestFixture *fixture, gconstpointer unused)
{
  const char *const query[] = {"sy", NULL};

  g_signal_connect_swapped (fixture->provider, "ready", (GCallback)g_main_loop_quit, fixture->mainloop);
  g_main_loop_run (fixture->mainloop);

  phosh_search_provider_get_initial (fixture->provider, query, got_initial_results, fixture);

  g_main_loop_run (fixture->mainloop);

  g_assert_true (fixture->initial_finished);
  g_assert_nonnull (initial_results);
  g_assert_cmpint (g_strv_length (initial_results), >, 0);
}


static void
test_phosh_search_provider_get_subsearch_async (TestFixture *fixture, gconstpointer unused)
{
  const char *const sub_query[] = {"sys", NULL};

  g_signal_connect_swapped (fixture->provider, "ready", (GCallback)g_main_loop_quit, fixture->mainloop);
  g_main_loop_run (fixture->mainloop);

  phosh_search_provider_get_subsearch (fixture->provider,
                                       (const char *const *) initial_results,
                                       sub_query,
                                       got_subsearch_results,
                                       fixture);

  g_main_loop_run (fixture->mainloop);

  g_assert_true (fixture->subsearch_finished);
  g_assert_nonnull (final_results);
  g_assert_cmpint (g_strv_length (final_results), >, 0);
}


static void
test_phosh_search_provider_get_result_meta_async (TestFixture *fixture, gconstpointer unused)
{
  GVariant *first_meta;

  g_signal_connect_swapped (fixture->provider, "ready", (GCallback)g_main_loop_quit, fixture->mainloop);
  g_main_loop_run (fixture->mainloop);

  phosh_search_provider_get_result_meta (fixture->provider, final_results, got_result_metas, fixture);

  g_main_loop_run (fixture->mainloop);

  g_assert_true (fixture->got_metas_finished);
  g_assert_nonnull (result_metas);
  /* After subsearch, it has only 2 final result */
  g_assert_cmpint (result_metas->len, ==, 2);

  first_meta = g_ptr_array_index (result_metas, 0);
  g_assert_nonnull (first_meta);
}


static void
test_phosh_search_provider_limit_results (TestFixture *fixture, gconstpointer unused)
{
  g_autoptr (GPtrArray) limited_results = NULL;

  g_signal_connect_swapped (fixture->provider, "ready", (GCallback)g_main_loop_quit, fixture->mainloop);
  g_main_loop_run (fixture->mainloop);

  limited_results = phosh_search_provider_limit_results (final_results, 1);

  g_assert_nonnull (limited_results);
  g_assert_cmpint (limited_results->len, ==, 1);

  /* cleanup after testing */
  g_clear_pointer (&initial_results, g_strfreev);
  g_clear_pointer (&final_results, g_strfreev);
  g_ptr_array_free (result_metas, TRUE);
}


gint
main (gint argc, char *argv[])
{
  g_autoptr(GTestDBus) dbus = NULL;
  g_autofree char *relative = NULL, *servicesdir = NULL;
  const char *display;
  int ret = -1;

  g_test_init (&argc, &argv, NULL);

  dbus = g_test_dbus_new (G_TEST_DBUS_NONE);
  display = g_getenv ("DISPLAY");
  relative = g_test_build_filename (G_TEST_BUILT, "services", NULL);
  servicesdir = g_canonicalize_filename (relative, NULL);

  g_test_dbus_add_service_dir (dbus, servicesdir);
  g_test_dbus_up (dbus);
  g_setenv ("DISPLAY", display, TRUE);

  g_test_add ("/phosh/search-provider/new", TestFixture, NULL,
              fixture_setup,
              test_phosh_search_provider_new,
              fixture_teardown);

  g_test_add ("/phosh/search-provider/get_bus_path", TestFixture, NULL,
              fixture_setup,
              test_phosh_search_provider_get_bus_path,
              fixture_teardown);

  g_test_add ("/phosh/search-provider/get_initial_async", TestFixture, NULL,
              fixture_setup,
              test_phosh_search_provider_get_initial_async,
              fixture_teardown);

  g_test_add ("/phosh/search-provider/get_subsearch_async", TestFixture, NULL,
              fixture_setup,
              test_phosh_search_provider_get_subsearch_async,
              fixture_teardown);

  g_test_add ("/phosh/search-provider/get_result_meta_async", TestFixture, NULL,
              fixture_setup,
              test_phosh_search_provider_get_result_meta_async,
              fixture_teardown);

  g_test_add ("/phosh/search-provider/limit_results", TestFixture, NULL,
              fixture_setup,
              test_phosh_search_provider_limit_results,
              fixture_teardown);

  ret = g_test_run ();
  g_test_dbus_down (dbus);

  return ret;
}
