/*
 * Copyright (C) 2021 Purism SPC
 *               2025 The Phosh Developers
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


static void
test_phosh_util_calculate_supported_mode_scales_integer (void)
{
  int num;
  g_autofree float *scales = NULL;

  /* Phone mode (e.g. PinePhone or Librem 5 */
  scales = phosh_util_calculate_supported_mode_scales (720,
                                                       1440,
                                                       &num,
                                                       FALSE);
  g_assert_cmpint (num, ==, 2);
  g_assert_true (G_APPROX_VALUE (scales[0], 1.0, FLT_EPSILON));
  g_assert_true (G_APPROX_VALUE (scales[1], 2.0, FLT_EPSILON));

  g_clear_pointer (&scales, g_free);
  scales = phosh_util_calculate_supported_mode_scales (3840,
                                                       2160,
                                                       &num, FALSE);
  g_assert_cmpint (num, ==, 4);
  g_assert_true (G_APPROX_VALUE (scales[0], 1.0, FLT_EPSILON));
  g_assert_true (G_APPROX_VALUE (scales[1], 2.0, FLT_EPSILON));
  g_assert_true (G_APPROX_VALUE (scales[2], 3.0, FLT_EPSILON));
  g_assert_true (G_APPROX_VALUE (scales[3], 4.0, FLT_EPSILON));
}


static void
test_phosh_util_calculate_supported_mode_scales_fractional (void)
{
  int num;
  g_autofree float *scales = NULL;

  /* 4K Mode */
  scales = phosh_util_calculate_supported_mode_scales (720,
                                                       1440,
                                                       &num,
                                                       TRUE);
  g_assert_cmpint (num, ==, 5);

  g_assert_true (G_APPROX_VALUE (scales[0], 1.0, FLT_EPSILON));
  g_assert_true (G_APPROX_VALUE (scales[1], 1.25, FLT_EPSILON));
  g_assert_true (G_APPROX_VALUE (scales[2], 1.5, FLT_EPSILON));
  g_assert_cmpfloat (scales[3], >=, 1.75);
  g_assert_cmpfloat (scales[3], <=, 1.76);
  g_assert_true (G_APPROX_VALUE (scales[4], 2.0, FLT_EPSILON));

  g_clear_pointer (&scales, g_free);
  scales = phosh_util_calculate_supported_mode_scales (3840,
                                                       2160,
                                                       &num,
                                                       TRUE);
  g_assert_cmpint (num, ==, 13);
  g_assert_true (G_APPROX_VALUE (scales[0], 1.0, FLT_EPSILON));
  g_assert_true (G_APPROX_VALUE (scales[1], 1.25, FLT_EPSILON));
  /* ... */
  g_assert_true (G_APPROX_VALUE (scales[11], 3.75, FLT_EPSILON));
  g_assert_true (G_APPROX_VALUE (scales[12], 4.0, FLT_EPSILON));
}


int
main (int argc, char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/util/escacpe-markup", test_phosh_util_escape_markup);
  g_test_add_func ("/phosh/util/data-uri-to-pixbuf", test_phosh_util_data_uri_to_pixbuf);
  g_test_add_func ("/phosh/util/scale/integer",
                   test_phosh_util_calculate_supported_mode_scales_integer);
  g_test_add_func ("/phosh/util/scale/fractional",
                   test_phosh_util_calculate_supported_mode_scales_fractional);

  return g_test_run ();
}
