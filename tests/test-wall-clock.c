/*
 * Copyright (C) 2021 Purism SPC
 *               2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "wall-clock-priv.h"

static void
test_phosh_wall_clock_strip_am_pm (void)
{
  char *str;
  g_autoptr (PhoshWallClock) clock = phosh_wall_clock_new ();

  /* 12h clock without seconds */
  str = phosh_wall_clock_strip_am_pm (clock, "11‎∶12 AM");
  g_assert_cmpstr (str, ==, "11‎∶12");
  g_free (str);
  str = phosh_wall_clock_strip_am_pm (clock, " 1‎∶24 PM");
  g_assert_cmpstr (str, ==, "1‎∶24");
  g_free (str);

  str = phosh_wall_clock_strip_am_pm (clock, "11‎∶12 ص");
  g_assert_cmpstr (str, ==, "11‎∶12");
  g_free (str);
  str = phosh_wall_clock_strip_am_pm (clock, " 1‎∶24 م");
  g_assert_cmpstr (str, ==, "1‎∶24");
  g_free (str);

  /* 12h clock with seconds */
  str = phosh_wall_clock_strip_am_pm (clock, "11‎∶12‎∶01 AM");
  g_assert_cmpstr (str, ==, "11‎∶12‎∶01");
  g_free (str);
  str = phosh_wall_clock_strip_am_pm (clock, " 1‎∶24‎∶01 PM");
  g_assert_cmpstr (str, ==, "1‎∶24‎∶01");
  g_free (str);

  str = phosh_wall_clock_strip_am_pm (clock, "11‎∶12‎∶01 ص");
  g_assert_cmpstr (str, ==, "11‎∶12‎∶01");
  g_free (str);
  str = phosh_wall_clock_strip_am_pm (clock, " 1‎∶24‎∶01 م");
  g_assert_cmpstr (str, ==, "1‎∶24‎∶01");
  g_free (str);

  /* 24 clock */
  str = phosh_wall_clock_strip_am_pm (clock, "07‎∶00‎∶00");
  g_assert_cmpstr (str, ==, "07‎∶00‎∶00");
  g_free (str);
  str = phosh_wall_clock_strip_am_pm (clock, "11‎∶12‎∶01");
  g_assert_cmpstr (str, ==, "11‎∶12‎∶01");
  g_free (str);
  str = phosh_wall_clock_strip_am_pm (clock, "13‎∶24‎∶01");
  g_assert_cmpstr (str, ==, "13‎∶24‎∶01");
  g_free (str);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/wall-clock/strip_am_pm", test_phosh_wall_clock_strip_am_pm);

  return g_test_run ();
}
