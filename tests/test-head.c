/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "monitor/head-priv.h"

PhoshHeadMode phone_mode = {
  .width = 720,
  .height = 1440,
};

PhoshHeadMode small_mode = {
  .width = 1024,
  .height = 768,
};

PhoshHeadMode fourk_mode = {
  .width = 3840,
  .height = 2160,
};

static PhoshHead *
stub_head_new (PhoshHeadMode *mode)
{
  /* We don't use `phosh_head_new()` so we don't have to bother about wayland protcols, etc */
  PhoshHead *head = g_new0 (PhoshHead, 1);

  head->pending.scale = 1.0;
  head->pending.mode = mode;
  return head;
}

static void
stub_head_destroy (PhoshHead *head)
{
  g_free (head);
}


static GPtrArray *
get_layout (float scale)
{
  PhoshHead *head;
  PhoshHead *other_head;
  GPtrArray *heads;

  head = stub_head_new (&small_mode);
  head->pending.scale = scale;

  other_head = stub_head_new (&fourk_mode);
  heads = g_ptr_array_new_with_free_func ((GDestroyNotify) (stub_head_destroy));

  g_assert_nonnull (head->pending.mode);
  g_assert_nonnull (other_head->pending.mode);

  g_ptr_array_add (heads, head);
  g_ptr_array_add (heads, other_head);

  return heads;
}


static void
test_phosh_head_set_transform (void)
{
  PhoshHead *head;
  PhoshHead *other_head;
  GPtrArray *heads;

  /* other_head right of head */
  for (float scale = 1.0; scale <= 2.0; scale += 0.5) {
    heads = get_layout (scale);
    head = g_ptr_array_index (heads, 0);
    other_head = g_ptr_array_index (heads, 1);

    other_head->pending.x = head->pending.mode->width / scale;
    g_assert_cmpint (head->pending.x, ==, 0);
    g_assert_cmpint (head->pending.y, ==, 0);
    g_assert_cmpint (other_head->pending.x, ==, small_mode.width / scale);
    g_assert_cmpint (other_head->pending.y, ==, 0);

    phosh_head_set_pending_transform (head, PHOSH_MONITOR_TRANSFORM_90, heads);
    g_assert_cmpint (head->pending.x, ==, 0);
    g_assert_cmpint (head->pending.y, ==, 0);
    g_assert_cmpint (head->pending.transform, ==, PHOSH_MONITOR_TRANSFORM_90);
    g_assert_cmpint (other_head->pending.x, ==, small_mode.height / scale);
    g_assert_cmpint (other_head->pending.y, ==, 0);

    g_ptr_array_free (heads, TRUE);
  }

  /* other_head below head */
  heads = get_layout (1.0);
  head = g_ptr_array_index (heads, 0);
  other_head = g_ptr_array_index (heads, 1);

  other_head->pending.y = head->pending.mode->height;
  g_assert_cmpint (head->pending.x, ==, 0);
  g_assert_cmpint (head->pending.y, ==, 0);
  g_assert_cmpint (other_head->pending.x, ==, 0);
  g_assert_cmpint (other_head->pending.y, ==, small_mode.height);

  phosh_head_set_pending_transform (head, PHOSH_MONITOR_TRANSFORM_90, heads);
  g_assert_cmpint (head->pending.x, ==, 0);
  g_assert_cmpint (head->pending.y, ==, 0);
  g_assert_cmpint (head->pending.transform, ==, PHOSH_MONITOR_TRANSFORM_90);
  g_assert_cmpint (other_head->pending.x, ==, 0);
  g_assert_cmpint (other_head->pending.y, ==, small_mode.width);

  g_ptr_array_free (heads, TRUE);

  /* other_head left of head */
  heads = get_layout (1.0);
  head = g_ptr_array_index (heads, 0);
  other_head = g_ptr_array_index (heads, 1);

  head->pending.x = other_head->pending.mode->width;
  g_assert_cmpint (head->pending.x, ==, fourk_mode.width);
  g_assert_cmpint (head->pending.y, ==, 0);
  g_assert_cmpint (other_head->pending.x, ==, 0);
  g_assert_cmpint (other_head->pending.y, ==, 0);

  phosh_head_set_pending_transform (head, PHOSH_MONITOR_TRANSFORM_90, heads);
  /* Nothing should have changed */
  g_assert_cmpint (head->pending.x, ==, fourk_mode.width);
  g_assert_cmpint (head->pending.y, ==, 0);
  g_assert_cmpint (head->pending.transform, ==, PHOSH_MONITOR_TRANSFORM_90);
  g_assert_cmpint (other_head->pending.x, ==, 0);
  g_assert_cmpint (other_head->pending.y, ==, 0);

  g_ptr_array_free (heads, TRUE);
}


static void
test_phosh_head_scale_integer (void)
{
  int num;
  g_autofree float *scales;

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

  scales = phosh_head_calculate_supported_mode_scales (NULL, &phone_mode, &num, TRUE);
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

  g_test_add_func("/phosh/head/layout/set_transform", test_phosh_head_set_transform);
  g_test_add_func("/phosh/head/scale/integer", test_phosh_head_scale_integer);
  g_test_add_func("/phosh/head/scale/fractional", test_phosh_head_scale_fractional);

  return g_test_run();
}
