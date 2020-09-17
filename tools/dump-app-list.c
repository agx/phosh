/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 *
 * Print the contents of PhoshAppListModel
 */

#include <app-list-model.h>

int
main (int argc, char **argv)
{
  PhoshAppListModel *list = phosh_app_list_model_get_default ();
  int i = 0;
  GAppInfo *info;
  GMainLoop *loop;

  g_print ("Populating...\n");

  /* Let the main loop run a bit for the timeouts/idle callbacks to run */
  loop = g_main_loop_new (NULL, FALSE);
  g_timeout_add_seconds (2, G_SOURCE_FUNC (g_main_loop_quit), loop);
  g_main_loop_run (loop);

  while ((info = g_list_model_get_item (G_LIST_MODEL (list), i))) {
    if (G_IS_DESKTOP_APP_INFO (info)) {
      g_print ("%s\n - %s\n",
               g_app_info_get_id (info),
               g_desktop_app_info_get_filename (G_DESKTOP_APP_INFO (info)));
    } else {
      /* Unlikely but handle it just in case */
      g_print ("%s\n - %s\n",
               g_app_info_get_id (info),
               G_OBJECT_TYPE_NAME (info));
    }

    i++;
    g_clear_object (&info);
  }

  g_print ("=== %i items\n", i);

  return 0;
}
