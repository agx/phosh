/*
 * Copyright © 2020 Lugsole
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "notifications/timestamp-label.h"

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

int
main (int   argc,
      char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/timestamp-label/new", test_phosh_timestamp_label_new);
  g_test_add_func ("/phosh/timestamp-label/get-set-timestamp", test_phosh_timestamp_label_get_set_timestamp);

  g_test_add_func ("/phosh/timestamp-label/test_phosh_timestamp_label_destroy", test_phosh_timestamp_label_destroy);
  return g_test_run ();
}
