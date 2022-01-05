/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "util.h"


static void
test_phosh_util_escape_markup (void)
{
  /* correct markup */
  g_assert_cmpstr (phosh_util_escape_markup ("&amp;&amp;", TRUE), ==, "&amp;&amp;");
  g_assert_cmpstr (phosh_util_escape_markup ("&quot;&quot;", TRUE), ==, "&quot;&quot;");
  g_assert_cmpstr (phosh_util_escape_markup ("&apos;&apos;", TRUE), ==, "&apos;&apos;");
  g_assert_cmpstr (phosh_util_escape_markup ("&lt;&lt;", TRUE), ==, "&lt;&lt;");
  g_assert_cmpstr (phosh_util_escape_markup ("&gt;&gt;", TRUE), ==, "&gt;&gt;");
  g_assert_cmpstr (phosh_util_escape_markup ("<b>bold</b>", TRUE), ==, "<b>bold</b>");
  g_assert_cmpstr (phosh_util_escape_markup ("<i>italic</i>", TRUE), ==, "<i>italic</i>");
  g_assert_cmpstr (phosh_util_escape_markup ("<u>underline</u>", TRUE), ==, "<u>underline</u>");
  g_assert_cmpstr (phosh_util_escape_markup ("<u>&amp;</u>", TRUE), ==, "<u>&amp;</u>");

  /* unknown tags and entities */
  g_assert_cmpstr (phosh_util_escape_markup ("&foo;&foo;", TRUE), ==, "&amp;foo;&amp;foo;");
  g_assert_cmpstr (phosh_util_escape_markup ("<p>para</p>", TRUE), ==, "&lt;p>para&lt;/p>");
  /* Make sure we match the full tag */
  g_assert_cmpstr (phosh_util_escape_markup ("<pp>para</pp>", TRUE), ==, "&lt;pp>para&lt;/pp>");
  g_assert_cmpstr (phosh_util_escape_markup ("&lt;", FALSE), ==, "&amp;lt;");

  /* broken markups */
  /* unbalanced markup */
  g_assert_cmpstr (phosh_util_escape_markup ("<p>para</i>", TRUE), ==, "&lt;p&gt;para&lt;/i&gt;");
  g_assert_cmpstr (phosh_util_escape_markup ("<p>para</pp>", TRUE), ==, "&lt;p>para&lt;/pp>");
}


int
main (int argc, char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/util/escacpe-markup", test_phosh_util_escape_markup);

  return g_test_run ();
}
