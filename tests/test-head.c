/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "monitor/head.h"


static void
test_phosh_head_scale_integer (void)
{
  int num;
  g_autofree int *scales;

  PhoshHeadMode phone_mode = {
    .width = 720,
    .height = 1440,
  };

  PhoshHeadMode fourk_mode = {
    .width = 3840,
    .height = 2160,
  };

  scales = phosh_head_calculate_supported_mode_scales (NULL, &phone_mode, &num);
  g_assert_cmpint (num, ==, 2);
  g_assert_cmpint (scales[0], ==, 1);
  g_assert_cmpint (scales[1], ==, 2);

  g_clear_pointer (&scales, g_free);
  scales = phosh_head_calculate_supported_mode_scales (NULL, &fourk_mode, &num);
  g_assert_cmpint (num, ==, 4);
  g_assert_cmpint (scales[0], ==, 1);
  g_assert_cmpint (scales[1], ==, 2);
  g_assert_cmpint (scales[2], ==, 3);
  g_assert_cmpint (scales[3], ==, 4);
}


int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func("/phosh/head/scale/integer", test_phosh_head_scale_integer);

  return g_test_run();
}
