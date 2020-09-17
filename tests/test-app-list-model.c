/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "app-list-model.h"

static void
test_phosh_app_list_model_get_default(void)
{
  PhoshAppListModel *model1 = phosh_app_list_model_get_default ();
  PhoshAppListModel *model2 = phosh_app_list_model_get_default ();

  g_assert_true (PHOSH_IS_APP_LIST_MODEL (model1));
  g_assert_true (model1 == model2);
  g_object_unref (model1);

  /* test we can do that twice in a row */
  model1 = phosh_app_list_model_get_default ();
  g_assert_true (PHOSH_IS_APP_LIST_MODEL (model1));
  g_object_unref (model1);

}


static void
test_phosh_app_list_model_g_list_iface (void)
{
  PhoshAppListModel *model = phosh_app_list_model_get_default ();

  g_assert_true (PHOSH_IS_APP_LIST_MODEL (model));
  g_assert_true (G_IS_LIST_MODEL (model));
  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (model)), ==, 0);
  g_assert_true (g_list_model_get_item_type (G_LIST_MODEL (model)) == G_TYPE_APP_INFO);
}


int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func("/phosh/app-list-model/new", test_phosh_app_list_model_get_default);
  g_test_add_func("/phosh/app-list-model/g_list_iface", test_phosh_app_list_model_g_list_iface);
  return g_test_run();
}
