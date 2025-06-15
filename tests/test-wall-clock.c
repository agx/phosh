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

  str = phosh_wall_clock_strip_am_pm ("11‎∶00 AM");
  g_assert_cmpstr (str, ==, "11‎∶00");
  g_free (str);
  str = phosh_wall_clock_strip_am_pm (" 1‎∶00 PM");
  g_assert_cmpstr (str, ==, "1‎∶00");
  g_free (str);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/wall-clock/strip_am_pm", test_phosh_wall_clock_strip_am_pm);

  return g_test_run ();
}
