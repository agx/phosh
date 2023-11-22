/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "monitor/gamma-table.h"

#define RAMP_SIZE 2

static void
test_phosh_gamma_table_fill (void)
{
  guint16 table[RAMP_SIZE * 3];

  phosh_gamma_table_fill (table, RAMP_SIZE, 6500);

  g_assert_cmpint (table[0], ==, 0);
  g_assert_cmpint (table[RAMP_SIZE], ==, 0);
  g_assert_cmpint (table[2 * RAMP_SIZE], ==, 0);

  g_assert_cmpint (table[1], ==, 32768);
  g_assert_cmpint (table[1 + RAMP_SIZE], ==, 32768);
  g_assert_cmpint (table[1 + 2 * RAMP_SIZE], ==, 32768);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func("/phosh/gamma-table/fill", test_phosh_gamma_table_fill);
  return g_test_run();
}
