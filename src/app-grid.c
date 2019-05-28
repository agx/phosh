/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#define G_LOG_DOMAIN "phosh-app-grid"

#include "app-grid.h"
#include "app-grid-button.h"
#include "app-list-model.h"

#include "gtk-list-models/gtksortlistmodel.h"
#include "gtk-list-models/gtkfilterlistmodel.h"

typedef struct _PhoshAppGridPrivate PhoshAppGridPrivate;
struct _PhoshAppGridPrivate {
  GtkFilterListModel *model;

  GtkWidget *search;
  GtkWidget *apps;
};

G_DEFINE_TYPE_WITH_PRIVATE (PhoshAppGrid, phosh_app_grid, GTK_TYPE_BOX)

static gint
sort_apps (gconstpointer a,
           gconstpointer b,
           gpointer      data)
{
  GAppInfo *info1 = G_APP_INFO (a);
  GAppInfo *info2 = G_APP_INFO (b);
  gchar *s1, *s2;
  gint ret;

  s1 = g_utf8_casefold (g_app_info_get_display_name (info1), -1);
  s2 = g_utf8_casefold (g_app_info_get_display_name (info2), -1);

  ret = g_strcmp0 (s1, s2);

  g_free (s1);
  g_free (s2);

  return ret;
}

static gboolean
search_apps (gpointer item, gpointer data)
{
  PhoshAppGrid *self = data;
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);
  GAppInfo *info = item;
  const char *search = NULL;
  const char *str = NULL;

  g_return_val_if_fail (priv != NULL, TRUE);
  g_return_val_if_fail (priv->search != NULL, TRUE);

  search = gtk_entry_get_text (GTK_ENTRY (priv->search));

  if (search == NULL)
    return TRUE;

  if ((str = g_app_info_get_display_name (info)) && strstr (str, search))
    return TRUE;

  if ((str = g_app_info_get_name (info)) && strstr (str, search))
    return TRUE;

  if ((str = g_app_info_get_description (info)) && strstr (str, search))
    return TRUE;

  if ((str = g_app_info_get_executable (info)) && strstr (str, search))
    return TRUE;

  if (G_IS_DESKTOP_APP_INFO (info)) {
    const char * const *kwds;
    int i = 0;

    if ((str = g_desktop_app_info_get_generic_name (G_DESKTOP_APP_INFO (info))) &&
         strstr (str, search))
      return TRUE;

    if ((str = g_desktop_app_info_get_categories (G_DESKTOP_APP_INFO (info))) &&
         strstr (str, search))
      return TRUE;

    kwds = g_desktop_app_info_get_keywords (G_DESKTOP_APP_INFO (info));

    if (kwds) {
      while ((str = kwds[i])) {
        if (strstr (str, search))
          return TRUE;
        i++;
      }
    }
  }

  return FALSE;
}

static GtkWidget *
create_launcher (gpointer item,
                 gpointer user_data)
{
  GtkWidget *btn = phosh_app_grid_button_new (G_APP_INFO (item));

  gtk_widget_show (btn);

  return btn;
}

static void
phosh_app_grid_init (PhoshAppGrid *self)
{
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);
  GtkSortListModel *sorted;

  gtk_widget_init_template (GTK_WIDGET (self));

  /* fill the grid with apps */
  sorted = gtk_sort_list_model_new (G_LIST_MODEL (phosh_app_list_model_get_default ()),
                                    sort_apps,
                                    NULL,
                                    NULL);
  priv->model = gtk_filter_list_model_new (G_LIST_MODEL (sorted),
                                           search_apps,
                                           self,
                                           NULL);
  gtk_flow_box_bind_model (GTK_FLOW_BOX (priv->apps), G_LIST_MODEL (priv->model),
                           create_launcher, self, NULL);
}

static void
phosh_app_grid_finalize (GObject *object)
{
  PhoshAppGrid *self = PHOSH_APP_GRID (object);
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);

  g_clear_object (&priv->model);

  G_OBJECT_CLASS (phosh_app_grid_parent_class)->finalize (object);
}

static gboolean
phosh_app_grid_key_press_event (GtkWidget   *widget,
                              GdkEventKey *event)
{
  PhoshAppGrid *self = PHOSH_APP_GRID (widget);
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);

  return gtk_search_entry_handle_event (GTK_SEARCH_ENTRY (priv->search), 
                                        (GdkEvent *) event);
}

static void
search_changed (GtkSearchEntry *entry,
                PhoshAppGrid    *self)
{
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);

  gtk_filter_list_model_refilter (priv->model);
}

static void
phosh_app_grid_class_init (PhoshAppGridClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_app_grid_finalize;

  widget_class->key_press_event = phosh_app_grid_key_press_event;

  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/phosh/ui/app-grid.ui");

  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGrid, search);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGrid, apps);

  gtk_widget_class_bind_template_callback (widget_class, search_changed);

  gtk_widget_class_set_css_name (widget_class, "phosh-app-grid");
}

GtkWidget *
phosh_app_grid_new (void)
{
  return g_object_new (PHOSH_TYPE_APP_GRID, NULL);
}
