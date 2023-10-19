/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

/* We use g_test_expect_message */
#undef G_LOG_USE_STRUCTURED

#include "search/search-result-meta.c"
#include "stubs/bad-instance.h"
#include "stubs/evil-icon.h"


static void
test_phosh_search_result_meta_new (void)
{
  g_autoptr (PhoshSearchResultMeta) meta = NULL;
  g_autoptr (GIcon) icon = NULL;

  icon = g_themed_icon_new ("start-here");

  meta = phosh_search_result_meta_new ("test", "Test", "Result", icon, "copy-me");

  g_assert_nonnull (meta);
  g_assert_cmpstr (phosh_search_result_meta_get_id (meta), ==, "test");
  g_assert_cmpstr (phosh_search_result_meta_get_title (meta), ==, "Test");
  g_assert_cmpstr (phosh_search_result_meta_get_description (meta), ==, "Result");
  g_assert_true (phosh_search_result_meta_get_icon (meta) == icon);
  g_assert_cmpstr (phosh_search_result_meta_get_clipboard_text (meta), ==, "copy-me");
}


static void
test_phosh_search_result_meta_refs (void)
{
  PhoshSearchResultMeta *meta;
  PhoshSearchResultMeta *meta_ref;

  meta = phosh_search_result_meta_new (NULL, NULL, NULL, NULL, NULL);

  g_assert_nonnull (meta);

  meta_ref = phosh_search_result_meta_ref (meta);

  g_assert_true (meta == meta_ref);

  phosh_search_result_meta_unref (meta);

  g_clear_pointer (&meta, phosh_search_result_meta_unref);

  g_assert_null (meta);

  NULL_INSTANCE_CALL (phosh_search_result_meta_ref, "self != NULL");
  NULL_INSTANCE_CALL (phosh_search_result_meta_unref, "self != NULL");
}


static void
test_phosh_search_result_meta_boxed (void)
{
  PhoshSearchResultMeta *meta;
  PhoshSearchResultMeta *meta_ref;

  meta = phosh_search_result_meta_new (NULL, NULL, NULL, NULL, NULL);

  g_assert_nonnull (meta);

  meta_ref = g_boxed_copy (PHOSH_TYPE_SEARCH_RESULT_META, meta);

  g_assert_true (meta == meta_ref);

  g_boxed_free (PHOSH_TYPE_SEARCH_RESULT_META, meta_ref);

  g_clear_pointer (&meta, phosh_search_result_meta_unref);

  g_assert_null (meta);
}


static void
test_phosh_search_result_meta_null_instance (void)
{
  g_autoptr (PhoshSearchResultMeta) meta = NULL;

  NULL_INSTANCE_CALL_RETURN (phosh_search_result_meta_get_id, "self != NULL", NULL);
  NULL_INSTANCE_CALL_RETURN (phosh_search_result_meta_get_title, "self != NULL", NULL);
  NULL_INSTANCE_CALL_RETURN (phosh_search_result_meta_get_description, "self != NULL",  NULL);
  NULL_INSTANCE_CALL_RETURN (phosh_search_result_meta_get_icon, "self != NULL", NULL);
  NULL_INSTANCE_CALL_RETURN (phosh_search_result_meta_get_clipboard_text, "self != NULL", NULL);
}


static void
test_phosh_search_result_meta_serialise (void)
{
  g_autoptr (PhoshSearchResultMeta) meta = NULL;
  g_autoptr (GIcon) icon = NULL;
  g_autoptr (GVariant) variant = NULL;
  g_autoptr (GVariant) icon_src = NULL;
  g_autoptr (GVariant) icon_res = NULL;
  g_autoptr (GVariantDict) dict = NULL;
  const char *val;

  icon = g_themed_icon_new ("start-here");

  meta = phosh_search_result_meta_new ("test", "Test", "Result", icon, "copy-me");

  variant = phosh_search_result_meta_serialise (meta);

  g_assert_nonnull (variant);
  g_assert_true (g_variant_is_of_type (variant, G_VARIANT_TYPE_VARDICT));

  dict = g_variant_dict_new (variant);

  g_assert_true (g_variant_dict_lookup (dict, "id", "&s", &val, NULL));
  g_assert_cmpstr (val, ==, "test");

  g_assert_true (g_variant_dict_lookup (dict, "title", "&s", &val, NULL));
  g_assert_cmpstr (val, ==, "Test");

  g_assert_true (g_variant_dict_lookup (dict, "desc", "&s", &val, NULL));
  g_assert_cmpstr (val, ==, "Result");

  icon_src = g_icon_serialize (icon);
  g_assert_nonnull (icon_src);

  icon_res = g_variant_dict_lookup_value (dict, "icon", G_VARIANT_TYPE_ANY);
  g_assert_nonnull (icon_res);

  g_assert_true (g_variant_equal (icon_src, icon_res));

  g_assert_true (g_variant_dict_lookup (dict, "clipboard-text", "&s", &val, NULL));
  g_assert_cmpstr (val, ==, "copy-me");
}


static void
test_phosh_search_result_meta_serialise_no_desc (void)
{
  g_autoptr (PhoshSearchResultMeta) meta = NULL;
  g_autoptr (GIcon) icon = NULL;
  g_autoptr (GVariant) variant = NULL;
  g_autoptr (GVariantDict) dict = NULL;

  icon = g_themed_icon_new ("start-here");

  meta = phosh_search_result_meta_new ("test", "Test", NULL, icon, "copy-me");

  variant = phosh_search_result_meta_serialise (meta);

  g_assert_nonnull (variant);
  g_assert_true (g_variant_is_of_type (variant, G_VARIANT_TYPE_VARDICT));

  dict = g_variant_dict_new (variant);

  g_assert_true (g_variant_dict_contains (dict, "id"));
  g_assert_true (g_variant_dict_contains (dict, "title"));
  g_assert_false (g_variant_dict_contains (dict, "desc"));
  g_assert_true (g_variant_dict_contains (dict, "icon"));
  g_assert_true (g_variant_dict_contains (dict, "clipboard-text"));
}


static void
test_phosh_search_result_meta_serialise_no_icon (void)
{
  g_autoptr (PhoshSearchResultMeta) meta = NULL;
  g_autoptr (GVariant) variant = NULL;
  g_autoptr (GVariantDict) dict = NULL;

  meta = phosh_search_result_meta_new ("test", "Test", "Result", NULL, "copy-me");

  variant = phosh_search_result_meta_serialise (meta);

  g_assert_nonnull (variant);
  g_assert_true (g_variant_is_of_type (variant, G_VARIANT_TYPE_VARDICT));

  dict = g_variant_dict_new (variant);

  g_assert_true (g_variant_dict_contains (dict, "id"));
  g_assert_true (g_variant_dict_contains (dict, "title"));
  g_assert_true (g_variant_dict_contains (dict, "desc"));
  g_assert_false (g_variant_dict_contains (dict, "icon"));
  g_assert_true (g_variant_dict_contains (dict, "clipboard-text"));
}


static void
test_phosh_search_result_meta_serialise_bad_icon (void)
{
  g_autoptr (PhoshSearchResultMeta) meta = NULL;
  g_autoptr (GVariant) variant = NULL;
  g_autoptr (GIcon) icon = NULL;
  g_autoptr (GVariantDict) dict = NULL;

  icon = phosh_evil_icon_new ();

  meta = phosh_search_result_meta_new ("test", "Test", "Result", icon, "copy-me");

  g_test_expect_message (G_LOG_DOMAIN,
                         G_LOG_LEVEL_WARNING,
                         "Can't serialise icon of type PhoshEvilIcon");
  variant = phosh_search_result_meta_serialise (meta);

  g_assert_nonnull (variant);
  g_assert_true (g_variant_is_of_type (variant, G_VARIANT_TYPE_VARDICT));

  dict = g_variant_dict_new (variant);

  g_assert_true (g_variant_dict_contains (dict, "id"));
  g_assert_true (g_variant_dict_contains (dict, "title"));
  g_assert_true (g_variant_dict_contains (dict, "desc"));
  g_assert_false (g_variant_dict_contains (dict, "icon"));
  g_assert_true (g_variant_dict_contains (dict, "clipboard-text"));
}


static void
test_phosh_search_result_meta_serialise_no_clipboard (void)
{
  g_autoptr (PhoshSearchResultMeta) meta = NULL;
  g_autoptr (GIcon) icon = NULL;
  g_autoptr (GVariant) variant = NULL;
  g_autoptr (GVariantDict) dict = NULL;

  icon = g_themed_icon_new ("start-here");

  meta = phosh_search_result_meta_new ("test", "Test", "Result", icon, NULL);

  variant = phosh_search_result_meta_serialise (meta);

  g_assert_nonnull (variant);
  g_assert_true (g_variant_is_of_type (variant, G_VARIANT_TYPE_VARDICT));

  dict = g_variant_dict_new (variant);

  g_assert_true (g_variant_dict_contains (dict, "id"));
  g_assert_true (g_variant_dict_contains (dict, "title"));
  g_assert_true (g_variant_dict_contains (dict, "desc"));
  g_assert_true (g_variant_dict_contains (dict, "icon"));
  g_assert_false (g_variant_dict_contains (dict, "clipboard-text"));
}


static void
test_phosh_search_result_meta_deserialise (void)
{
  g_autoptr (PhoshSearchResultMeta) meta = NULL;
  g_autoptr (GVariant) src = NULL;
  g_autoptr (GIcon) icon = NULL;
  g_autoptr (GVariant) icon_variant = NULL;
  g_autoptr (GVariantDict) dict = NULL;

  icon = g_themed_icon_new ("start-here");
  icon_variant = g_icon_serialize (icon);
  g_assert_nonnull (icon_variant);

  dict = g_variant_dict_new (NULL);
  g_variant_dict_insert (dict, "id", "s", "test", NULL);
  g_variant_dict_insert (dict, "title", "s", "Test", NULL);
  g_variant_dict_insert (dict, "desc", "s", "Result", NULL);
  g_variant_dict_insert_value (dict, "icon", icon_variant);
  g_variant_dict_insert (dict, "clipboard-text", "s", "copy-me", NULL);

  src = g_variant_dict_end (dict);
  g_assert_nonnull (src);

  meta = phosh_search_result_meta_deserialise (src);

  g_assert_nonnull (meta);
  g_assert_cmpstr (phosh_search_result_meta_get_id (meta), ==, "test");
  g_assert_cmpstr (phosh_search_result_meta_get_title (meta), ==, "Test");
  g_assert_cmpstr (phosh_search_result_meta_get_description (meta), ==, "Result");
  g_assert_true (g_icon_equal (phosh_search_result_meta_get_icon (meta), icon));
  g_assert_cmpstr (phosh_search_result_meta_get_clipboard_text (meta), ==, "copy-me");
}


static void
test_phosh_search_result_meta_deserialise_no_icon (void)
{
  g_autoptr (PhoshSearchResultMeta) meta = NULL;
  g_autoptr (GVariant) src = NULL;
  g_autoptr (GVariantDict) dict = NULL;

  dict = g_variant_dict_new (NULL);
  g_variant_dict_insert (dict, "id", "s", "test", NULL);
  g_variant_dict_insert (dict, "title", "s", "Test", NULL);
  g_variant_dict_insert (dict, "desc", "s", "Result", NULL);
  g_variant_dict_insert (dict, "clipboard-text", "s", "copy-me", NULL);

  src = g_variant_dict_end (dict);
  g_assert_nonnull (src);

  meta = phosh_search_result_meta_deserialise (src);

  g_assert_nonnull (meta);
  g_assert_nonnull (phosh_search_result_meta_get_id (meta));
  g_assert_nonnull (phosh_search_result_meta_get_title (meta));
  g_assert_nonnull (phosh_search_result_meta_get_description (meta));
  g_assert_null (phosh_search_result_meta_get_icon (meta));
  g_assert_nonnull (phosh_search_result_meta_get_clipboard_text (meta));
}


int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/search-result-meta/new",
                   test_phosh_search_result_meta_new);
  g_test_add_func ("/phosh/search-result-meta/refs",
                   test_phosh_search_result_meta_refs);
  g_test_add_func ("/phosh/search-result-meta/boxed",
                   test_phosh_search_result_meta_boxed);
  g_test_add_func ("/phosh/search-result-meta/null-instance",
                   test_phosh_search_result_meta_null_instance);
  g_test_add_func ("/phosh/search-result-meta/serialise",
                   test_phosh_search_result_meta_serialise);
  g_test_add_func ("/phosh/search-result-meta/serialise/no-description",
                   test_phosh_search_result_meta_serialise_no_desc);
  g_test_add_func ("/phosh/search-result-meta/serialise/no-icon",
                   test_phosh_search_result_meta_serialise_no_icon);
  g_test_add_func ("/phosh/search-result-meta/serialise/bad-icon",
                   test_phosh_search_result_meta_serialise_bad_icon);
  g_test_add_func ("/phosh/search-result-meta/serialise/no-clipboard-text",
                   test_phosh_search_result_meta_serialise_no_clipboard);
  g_test_add_func ("/phosh/search-result-meta/deserialise",
                   test_phosh_search_result_meta_deserialise);
  g_test_add_func ("/phosh/search-result-meta/deserialise/no-icon",
                   test_phosh_search_result_meta_deserialise_no_icon);

  return g_test_run ();
}
