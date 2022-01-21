/*
 * Copyright © 2020 Lugsole
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "notifications/timestamp-label.h"
#include "notifications/timestamp-label-priv.h"

#include <locale.h>

static void
test_phosh_timestamp_label_new (void)
{
  g_autoptr (PhoshTimestampLabel) widget = phosh_timestamp_label_new ();
  g_autoptr (GDateTime) item_set_to = NULL;


  g_assert_true (PHOSH_IS_TIMESTAMP_LABEL (widget));

  item_set_to = phosh_timestamp_label_get_timestamp (PHOSH_TIMESTAMP_LABEL (widget));
  g_assert_null (item_set_to);

  g_object_ref_sink (widget);
}


static void
test_phosh_timestamp_label_get_set_timestamp (void)
{
  g_autoptr (PhoshTimestampLabel) widget = phosh_timestamp_label_new ();
  GDateTime  *item_set_to = NULL;
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();

  g_assert_true (PHOSH_IS_TIMESTAMP_LABEL (widget));

  item_set_to = phosh_timestamp_label_get_timestamp (PHOSH_TIMESTAMP_LABEL (widget));
  g_assert_null (item_set_to);

  phosh_timestamp_label_set_timestamp (PHOSH_TIMESTAMP_LABEL (widget), now);
  item_set_to = phosh_timestamp_label_get_timestamp (PHOSH_TIMESTAMP_LABEL (widget));
  g_assert_true (item_set_to == now);

  g_object_ref_sink (widget);
}


static void
test_phosh_timestamp_label_destroy (void)
{
  PhoshTimestampLabel *widget = phosh_timestamp_label_new ();
  GDateTime  *item_set_to = NULL;
  g_autoptr (GDateTime) now = g_date_time_new_now_local ();

  g_assert_true (PHOSH_IS_TIMESTAMP_LABEL (widget));

  item_set_to = phosh_timestamp_label_get_timestamp (PHOSH_TIMESTAMP_LABEL (widget));
  g_assert_null (item_set_to);

  phosh_timestamp_label_set_timestamp (PHOSH_TIMESTAMP_LABEL (widget), now);
  item_set_to = phosh_timestamp_label_get_timestamp (PHOSH_TIMESTAMP_LABEL (widget));
  g_assert_true (item_set_to == now);

  gtk_widget_destroy (GTK_WIDGET(widget));
  item_set_to = phosh_timestamp_label_get_timestamp (PHOSH_TIMESTAMP_LABEL (widget));
  g_assert_null (item_set_to);
  g_object_ref_sink (widget);
}


static void
test_phosh_time_diff_in_words (void)
{
  g_autofree char *str = NULL;
  g_autoptr (GDateTime) dt = NULL;
  g_autoptr (GDateTime) dt_now = g_date_time_new_local (2020, 12, 31, 23, 00, 00);
  locale_t locale;
  locale_t save_locale;

  locale = newlocale (LC_ALL_MASK, "C", (locale_t) 0);
  g_assert_true (locale != (locale_t)0);
  save_locale = uselocale (locale);

  dt = g_date_time_new_local (2020, 12, 31, 23, 00, 00);
  str = phosh_time_diff_in_words (dt, dt_now);
  g_assert_cmpstr (str, ==, "now");

  g_date_time_unref (dt);
  dt = g_date_time_new_local (2020, 12, 31, 22, 59, 31);
  g_free (str);
  str = phosh_time_diff_in_words (dt, dt_now);
  g_assert_cmpstr (str, ==, "<30s");

  g_date_time_unref (dt);
  dt = g_date_time_new_local (2020, 12, 31, 22, 59, 29);
  g_free (str);
  str = phosh_time_diff_in_words (dt, dt_now);
  g_assert_cmpstr (str, ==, "<1m");

  g_date_time_unref (dt);
  dt = g_date_time_new_local (2020, 12, 31, 22, 30, 00);
  g_free (str);
  str = phosh_time_diff_in_words (dt, dt_now);
  g_assert_cmpstr (str, ==, "30m");

  g_date_time_unref (dt);
  dt = g_date_time_new_local (2020, 12, 31, 21, 00, 00);
  g_free (str);
  str = phosh_time_diff_in_words (dt, dt_now);
  g_assert_cmpstr (str, ==, "~2h");

  g_date_time_unref (dt);
  dt = g_date_time_new_local (2020, 12, 30, 21, 00, 00);
  g_free (str);
  str = phosh_time_diff_in_words (dt, dt_now);
  g_assert_cmpstr (str, ==, "~1d");

  g_date_time_unref (dt);
  dt = g_date_time_new_local (2020, 12, 29, 21, 00, 00);
  g_free (str);
  str = phosh_time_diff_in_words (dt, dt_now);
  g_assert_cmpstr (str, ==, "2d");

  g_date_time_unref (dt);
  dt = g_date_time_new_local (2020, 11, 29, 21, 00, 00);
  g_free (str);
  str = phosh_time_diff_in_words (dt, dt_now);
  g_assert_cmpstr (str, ==, "~1mo");

  g_date_time_unref (dt);
  dt = g_date_time_new_local (2020, 10, 29, 21, 00, 00);
  g_free (str);
  str = phosh_time_diff_in_words (dt, dt_now);
  g_assert_cmpstr (str, ==, "2mos");

  g_date_time_unref (dt);
  dt = g_date_time_new_local (2019, 12, 31, 21, 00, 00);
  g_free (str);
  str = phosh_time_diff_in_words (dt, dt_now);
  g_assert_cmpstr (str, ==, "~1y");

  g_date_time_unref (dt);
  dt = g_date_time_new_local (2019, 3, 30, 21, 00, 00);
  g_free (str);
  str = phosh_time_diff_in_words (dt, dt_now);
  g_assert_cmpstr (str, ==, "Almost 2y");

  g_date_time_unref (dt);
  dt = g_date_time_new_local (2018, 12, 31, 21, 00, 00);
  g_free (str);
  str = phosh_time_diff_in_words (dt, dt_now);
  g_assert_cmpstr (str, ==, "~2y");

  g_date_time_unref (dt);
  dt = g_date_time_new_local (2018, 6, 30, 21, 00, 00);
  g_free (str);
  str = phosh_time_diff_in_words (dt, dt_now);
  g_assert_cmpstr (str, ==, "Over 2y");

  /* Restore previous locale */
  uselocale (save_locale);
  freelocale (locale);
}


int
main (int   argc,
      char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/timestamp-label/new", test_phosh_timestamp_label_new);
  g_test_add_func ("/phosh/timestamp-label/get-set-timestamp", test_phosh_timestamp_label_get_set_timestamp);

  g_test_add_func ("/phosh/timestamp-label/test_phosh_timestamp_label_destroy", test_phosh_timestamp_label_destroy);
  g_test_add_func ("/phosh/timestamp-label/test_phosh_time_diff_in_words", test_phosh_time_diff_in_words);
  return g_test_run ();
}
