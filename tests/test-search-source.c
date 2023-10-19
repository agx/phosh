/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "search/search-source.c"
#include "stubs/bad-instance.h"


static void
test_phosh_search_source_new (void)
{
  g_autoptr (PhoshSearchSource) source = NULL;
  g_autoptr (GAppInfo) info = NULL;

  info = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));
  g_assert_nonnull (info);

  source = phosh_search_source_new ("test", info);
  g_assert_nonnull (source);

  g_assert_cmpstr (phosh_search_source_get_id (source), ==, "test");
  g_assert_true (phosh_search_source_get_app_info (source) == info);
}


static void
test_phosh_search_source_refs (void)
{
  PhoshSearchSource *source;
  PhoshSearchSource *source_ref;

  source = phosh_search_source_new ("test", NULL);
  g_assert_nonnull (source);

  source_ref = phosh_search_source_ref (source);

  g_assert_true (source == source_ref);

  phosh_search_source_unref (source);

  g_clear_pointer (&source, phosh_search_source_unref);

  g_assert_null (source);

  NULL_INSTANCE_CALL (phosh_search_source_ref, "self != NULL");
  NULL_INSTANCE_CALL (phosh_search_source_unref, "self != NULL");
}


static void
test_phosh_search_source_boxed (void)
{
  PhoshSearchSource *source;
  PhoshSearchSource *source_ref;

  source = phosh_search_source_new ("test", NULL);
  g_assert_nonnull (source);

  source_ref = g_boxed_copy (PHOSH_TYPE_SEARCH_SOURCE, source);

  g_assert_true (source == source_ref);

  g_boxed_free (PHOSH_TYPE_SEARCH_SOURCE, source_ref);

  g_clear_pointer (&source, phosh_search_source_unref);

  g_assert_null (source);
}


static void
test_phosh_search_source_null_instance (void)
{
  g_autoptr (PhoshSearchSource) source = NULL;

  NULL_INSTANCE_CALL_RETURN (phosh_search_source_get_id, "self != NULL", NULL);
  NULL_INSTANCE_CALL_RETURN (phosh_search_source_get_app_info, "self != NULL", NULL);
}


static void
test_phosh_search_source_serialise (void)
{
  g_autoptr (PhoshSearchSource) source = NULL;
  g_autoptr (GAppInfo) info = NULL;
  g_autoptr (GVariant) variant = NULL;
  g_autofree char *id = NULL;
  g_autofree char *app_info_id = NULL;
  g_autoptr (GAppInfo) app_info = NULL;
  guint position;

  info = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));
  g_assert_nonnull (info);

  source = phosh_search_source_new ("test", info);
  g_assert_nonnull (source);

  phosh_search_source_set_position (source, 42);

  variant = phosh_search_source_serialise (source);

  g_assert_nonnull (source);
  g_assert_true (g_variant_is_of_type (variant, G_VARIANT_TYPE_TUPLE));
  g_assert_true (g_variant_is_of_type (variant, G_VARIANT_TYPE ("(ssu)")));

  g_variant_get (variant, "(ssu)", &id, &app_info_id, &position);

  g_assert_cmpstr (id, ==, "test");
  g_assert_cmpstr (app_info_id, ==, "demo.app.Second.desktop");
  g_assert_cmpuint (position, ==, 42);
}


static void
test_phosh_search_source_deserialise (void)
{
  g_autoptr (PhoshSearchSource) source = NULL;
  g_autoptr (GVariant) src = NULL;
  GAppInfo *info = NULL;

  src = g_variant_new ("(ssu)", "test", "demo.app.Second.desktop", 42);
  g_assert_nonnull (src);

  source = phosh_search_source_deserialise (src);

  g_assert_nonnull (source);
  g_assert_cmpstr (phosh_search_source_get_id (source), ==, "test");
  info = phosh_search_source_get_app_info (source);
  g_assert_cmpstr (g_app_info_get_id (info), ==, "demo.app.Second.desktop");
  g_assert_cmpuint (phosh_search_source_get_position (source), ==, 42);
}


int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/search-source/new",
                   test_phosh_search_source_new);
  g_test_add_func ("/phosh/search-source/refs",
                   test_phosh_search_source_refs);
  g_test_add_func ("/phosh/search-source/boxed",
                   test_phosh_search_source_boxed);
  g_test_add_func ("/phosh/search-source/null-instance",
                   test_phosh_search_source_null_instance);
  g_test_add_func ("/phosh/search-source/serialise",
                   test_phosh_search_source_serialise);
  g_test_add_func ("/phosh/search-source/deserialise",
                   test_phosh_search_source_deserialise);

  return g_test_run ();
}
