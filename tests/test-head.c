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
  g_autofree float *scales;

  PhoshHeadMode phone_mode = {
    .width = 720,
    .height = 1440,
  };

  PhoshHeadMode fourk_mode = {
    .width = 3840,
    .height = 2160,
  };

  scales = phosh_head_calculate_supported_mode_scales (NULL, &phone_mode, &num, FALSE);
  g_assert_cmpint (num, ==, 2);
  g_assert_true (G_APPROX_VALUE (scales[0], 1.0, FLT_EPSILON));
  g_assert_true (G_APPROX_VALUE (scales[1], 2.0, FLT_EPSILON));

  g_clear_pointer (&scales, g_free);
  scales = phosh_head_calculate_supported_mode_scales (NULL, &fourk_mode, &num, FALSE);
  g_assert_cmpint (num, ==, 4);
  g_assert_true (G_APPROX_VALUE (scales[0], 1.0, FLT_EPSILON));
  g_assert_true (G_APPROX_VALUE (scales[1], 2.0, FLT_EPSILON));
  g_assert_true (G_APPROX_VALUE (scales[2], 3.0, FLT_EPSILON));
  g_assert_true (G_APPROX_VALUE (scales[3], 4.0, FLT_EPSILON));
}


static void
test_phosh_head_scale_fractional (void)
{
  int num;
  g_autofree float *scales;

  PhoshHeadMode mode = {
    .width = 720,
    .height = 1440,
  };

  PhoshHeadMode fourk_mode = {
    .width = 3840,
    .height = 2160,
  };

  scales = phosh_head_calculate_supported_mode_scales (NULL, &mode, &num, TRUE);
  g_assert_cmpint (num, ==, 5);

  g_assert_true (G_APPROX_VALUE (scales[0], 1.0, FLT_EPSILON));
  g_assert_true (G_APPROX_VALUE (scales[1], 1.25, FLT_EPSILON));
  g_assert_true (G_APPROX_VALUE (scales[2], 1.5, FLT_EPSILON));
  g_assert_cmpfloat (scales[3], >=, 1.75);
  g_assert_cmpfloat (scales[3], <=, 1.76);
  g_assert_true (G_APPROX_VALUE (scales[4], 2.0, FLT_EPSILON));

  g_clear_pointer (&scales, g_free);
  scales = phosh_head_calculate_supported_mode_scales (NULL, &fourk_mode, &num, TRUE);
  g_assert_cmpint (num, ==, 13);
  g_assert_true (G_APPROX_VALUE (scales[0], 1.0, FLT_EPSILON));
  g_assert_true (G_APPROX_VALUE (scales[1], 1.25, FLT_EPSILON));
  /* ... */
  g_assert_true (G_APPROX_VALUE (scales[11], 3.75, FLT_EPSILON));
  g_assert_true (G_APPROX_VALUE (scales[12], 4.0, FLT_EPSILON));
}


int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func("/phosh/head/scale/integer", test_phosh_head_scale_integer);
  g_test_add_func("/phosh/head/scale/fractional", test_phosh_head_scale_fractional);

  return g_test_run();
}
