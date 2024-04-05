/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "util.h"

#define phosh_test_assert_cmp_markup(in, esc, cmp)                        \
  do {                                                        \
    g_autofree char *escaped = phosh_util_escape_markup (in, esc);   \
    g_assert_cmpstr (escaped, ==, cmp);                              \
  } while (0);

static void
test_phosh_util_escape_markup (void)
{
  /* correct markup */
  phosh_test_assert_cmp_markup ("&amp;&amp;", TRUE, "&amp;&amp;");
  phosh_test_assert_cmp_markup ("&quot;&quot;", TRUE, "&quot;&quot;");
  phosh_test_assert_cmp_markup ("&apos;&apos;", TRUE, "&apos;&apos;");
  phosh_test_assert_cmp_markup ("&lt;&lt;", TRUE, "&lt;&lt;");
  phosh_test_assert_cmp_markup ("&gt;&gt;", TRUE, "&gt;&gt;");
  phosh_test_assert_cmp_markup ("<b>bold</b>", TRUE, "<b>bold</b>");
  phosh_test_assert_cmp_markup ("<i>italic</i>", TRUE, "<i>italic</i>");
  phosh_test_assert_cmp_markup ("<u>underline</u>", TRUE, "<u>underline</u>");
  phosh_test_assert_cmp_markup ("<u>&amp;</u>", TRUE, "<u>&amp;</u>");

  /* unknown tags and entities */
  phosh_test_assert_cmp_markup ("&foo;&foo;", TRUE, "&amp;foo;&amp;foo;");
  phosh_test_assert_cmp_markup ("<p>para</p>", TRUE, "&lt;p>para&lt;/p>");
  /* Make sure we match the full tag */
  phosh_test_assert_cmp_markup ("<pp>para</pp>", TRUE, "&lt;pp>para&lt;/pp>");
  phosh_test_assert_cmp_markup ("&lt;", FALSE, "&amp;lt;");

  /* broken markups */
  /* unbalanced markup */
  phosh_test_assert_cmp_markup ("<p>para</i>", TRUE, "&lt;p&gt;para&lt;/i&gt;");
  phosh_test_assert_cmp_markup ("<p>para</pp>", TRUE, "&lt;p>para&lt;/pp>");
}


static void
test_phosh_util_data_uri_to_pixbuf (void)
{
  /* smallest valid png. see https://evanhahn.com/worlds-smallest-png/ */
  const char *valid_uri = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABAQAAAAA3bvkkAAAAC"
    "klEQVR4AWNgAAAAAgABc3UBGAAAAABJRU5ErkJggg==";
  const char *not_a_png = "data:image/png;base64,aGkgdGhlcmUh";
  GdkPixbuf* pixbuf = NULL;
  g_autoptr (GError) error = NULL;

  pixbuf = phosh_util_data_uri_to_pixbuf (valid_uri, &error);
  g_assert_no_error (error);
  g_assert_nonnull (pixbuf);
  g_assert_finalize_object (pixbuf);

  pixbuf = phosh_util_data_uri_to_pixbuf (not_a_png, &error);
  g_assert_null (pixbuf);
  g_assert_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_UNKNOWN_TYPE);

  g_clear_error (&error);
  pixbuf = phosh_util_data_uri_to_pixbuf ("not-an_uri", &error);
  g_assert_null (pixbuf);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_FAILED);
}


int
main (int argc, char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/util/escacpe-markup", test_phosh_util_escape_markup);
  g_test_add_func ("/phosh/util/data-uri-to-pixbuf", test_phosh_util_data_uri_to_pixbuf);

  return g_test_run ();
}
