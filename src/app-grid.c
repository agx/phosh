/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "phosh-app-grid"

#define ACTIVE_SEARCH_CLASS "search-active"

#include "feedback-manager.h"
#include "app-grid.h"
#include "app-grid-button.h"
#include "app-list-model.h"
#include "favorite-list-model.h"

#include "gtk-list-models/gtksortlistmodel.h"
#include "gtk-list-models/gtkfilterlistmodel.h"

typedef struct _PhoshAppGridPrivate PhoshAppGridPrivate;
struct _PhoshAppGridPrivate {
  GtkFilterListModel *model;

  GtkWidget *search;
  GtkWidget *apps;
  GtkWidget *favs;
  GtkWidget *favs_revealer;
  GtkWidget *scrolled_window;

  char *search_string;
};

G_DEFINE_TYPE_WITH_PRIVATE (PhoshAppGrid, phosh_app_grid, GTK_TYPE_BOX)

enum {
  APP_LAUNCHED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

static void
app_launched_cb (GtkWidget    *widget,
                 GAppInfo     *info,
                 PhoshAppGrid *self)
{
  phosh_trigger_feedback ("button-pressed");
  g_signal_emit (self, signals[APP_LAUNCHED], 0, info);
}


static gint
sort_apps (gconstpointer a,
           gconstpointer b,
           gpointer      data)
{
  GAppInfo *info1 = G_APP_INFO (a);
  GAppInfo *info2 = G_APP_INFO (b);

  g_autofree char *s1 = g_utf8_casefold (g_app_info_get_name (info1), -1);
  g_autofree char *s2 = g_utf8_casefold (g_app_info_get_name (info2), -1);

  return g_strcmp0 (s1, s2);
}

static const char *(*app_attr[]) (GAppInfo *info) = {
  g_app_info_get_display_name,
  g_app_info_get_name,
  g_app_info_get_description,
  g_app_info_get_executable,
};

static const char *(*desktop_attr[]) (GDesktopAppInfo *info) = {
  g_desktop_app_info_get_generic_name,
  g_desktop_app_info_get_categories,
};


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

  search = priv->search_string;

  /* filter out favorites when not searching */
  if (search == NULL || strlen (search) == 0) {
    if (phosh_favorite_list_model_app_is_favorite (NULL, info))
      return FALSE;

    return TRUE;
  }

  for (int i = 0; i < G_N_ELEMENTS (app_attr); i++) {
    g_autofree char *folded = NULL;

    str = app_attr[i] (info);

    if (!str || *str == '\0')
      continue;

    folded = g_utf8_casefold (str, -1);

    if (strstr (folded, search))
      return TRUE;
  }

  if (G_IS_DESKTOP_APP_INFO (info)) {
    const char * const *kwds;

    for (int i = 0; i < G_N_ELEMENTS (desktop_attr); i++) {
      g_autofree char *folded = NULL;

      str = desktop_attr[i] (G_DESKTOP_APP_INFO (info));

      if (!str || *str == '\0')
        continue;

      folded = g_utf8_casefold (str, -1);

      if (strstr (folded, search))
        return TRUE;
    }

    kwds = g_desktop_app_info_get_keywords (G_DESKTOP_APP_INFO (info));

    if (kwds) {
      int i = 0;

      while ((str = kwds[i])) {
        g_autofree char *folded = g_utf8_casefold (str, -1);
        if (strstr (folded, search))
          return TRUE;
        i++;
      }
    }
  }

  return FALSE;
}


static GtkWidget *
create_favorite_launcher (gpointer item,
                          gpointer self)
{
  GtkWidget *btn = phosh_app_grid_button_new_favorite (G_APP_INFO (item));

  g_signal_connect (btn, "app-launched",
                    G_CALLBACK (app_launched_cb), self);

  gtk_widget_show (btn);

  return btn;
}


static void
favorites_changed (GListModel   *list,
                   guint         position,
                   guint         removed,
                   guint         added,
                   PhoshAppGrid *self)
{
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);

  // We don't show favorites in the main list, filter them out
  gtk_filter_list_model_refilter (priv->model);
}


static GtkWidget *
create_launcher (gpointer item,
                 gpointer self)
{
  GtkWidget *btn = phosh_app_grid_button_new (G_APP_INFO (item));

  g_signal_connect (btn, "app-launched",
                    G_CALLBACK (app_launched_cb), self);

  gtk_widget_show (btn);

  return btn;
}


static void
phosh_app_grid_init (PhoshAppGrid *self)
{
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);
  GtkSortListModel *sorted;
  PhoshFavoriteListModel *favorites;

  gtk_widget_init_template (GTK_WIDGET (self));

  favorites = phosh_favorite_list_model_get_default ();

  gtk_flow_box_bind_model (GTK_FLOW_BOX (priv->favs),
                           G_LIST_MODEL (favorites),
                           create_favorite_launcher, self, NULL);
  g_signal_connect (favorites,
                    "items-changed",
                    G_CALLBACK (favorites_changed),
                    self);

  /* fill the grid with apps */
  sorted = gtk_sort_list_model_new (G_LIST_MODEL (phosh_app_list_model_get_default ()),
                                    sort_apps,
                                    NULL,
                                    NULL);
  priv->model = gtk_filter_list_model_new (G_LIST_MODEL (sorted),
                                           search_apps,
                                           self,
                                           NULL);
  gtk_flow_box_bind_model (GTK_FLOW_BOX (priv->apps),
                           G_LIST_MODEL (priv->model),
                           create_launcher, self, NULL);
}


static void
phosh_app_grid_finalize (GObject *object)
{
  PhoshAppGrid *self = PHOSH_APP_GRID (object);
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);

  g_clear_object (&priv->model);
  g_clear_pointer (&priv->search_string, g_free);

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
do_search (PhoshAppGrid *self)
{
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);
  GtkAdjustment *adjustment;

  if (priv->search_string && *priv->search_string != '\0') {
    gtk_revealer_set_reveal_child (GTK_REVEALER (priv->favs_revealer), FALSE);
    gtk_style_context_add_class (gtk_widget_get_style_context (priv->apps),
                                 ACTIVE_SEARCH_CLASS);
  } else {
    gtk_revealer_set_reveal_child (GTK_REVEALER (priv->favs_revealer), TRUE);
    adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->scrolled_window));
    gtk_adjustment_set_value (adjustment, 0);
    gtk_style_context_remove_class (gtk_widget_get_style_context (priv->apps),
                                    ACTIVE_SEARCH_CLASS);
  }

  gtk_filter_list_model_refilter (priv->model);
}


static void
search_changed (GtkSearchEntry *entry,
                PhoshAppGrid   *self)
{
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);
  const char *search = gtk_entry_get_text (GTK_ENTRY (entry));

  g_clear_pointer (&priv->search_string, g_free);

  if (search && *search != '\0')
      priv->search_string = g_utf8_casefold (search, -1);

  do_search (self);
}

static void
search_preedit_changed (GtkSearchEntry *entry,
                        const char     *preedit,
                        PhoshAppGrid   *self)
{
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);

  g_clear_pointer (&priv->search_string, g_free);

  if (preedit && *preedit != '\0')
      priv->search_string = g_utf8_casefold (preedit, -1);

  do_search (self);
}


static void
search_activated (GtkSearchEntry *entry,
                  PhoshAppGrid   *self)
{
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);
  GtkFlowBoxChild *child;

  if (!gtk_widget_has_focus (GTK_WIDGET (entry)))
    return;

  // Don't activate when there isn't an active search
  if (!priv->search_string || *priv->search_string == '\0') {
    return;
  }

  child = gtk_flow_box_get_child_at_index (GTK_FLOW_BOX (priv->apps), 0);

  // No results
  if (child == NULL) {
    return;
  }

  if (G_LIKELY (PHOSH_IS_APP_GRID_BUTTON (child))) {
    gtk_widget_activate (GTK_WIDGET (child));
  } else {
    g_critical ("Unexpected child type, %s",
                g_type_name (G_TYPE_FROM_INSTANCE (child)));
  }
}


static gboolean
search_lost_focus (GtkWidget    *widget,
                   GdkEvent     *event,
                   PhoshAppGrid *self)
{
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);

  gtk_style_context_remove_class (gtk_widget_get_style_context (priv->apps),
                                  ACTIVE_SEARCH_CLASS);

  return GDK_EVENT_PROPAGATE;
}


static gboolean
search_gained_focus (GtkWidget    *widget,
                     GdkEvent     *event,
                     PhoshAppGrid *self)
{
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);

  if (priv->search_string && *priv->search_string != '\0') {
    gtk_style_context_add_class (gtk_widget_get_style_context (priv->apps),
                                 ACTIVE_SEARCH_CLASS);
  }

  return GDK_EVENT_PROPAGATE;
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
  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGrid, favs);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGrid, favs_revealer);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGrid, scrolled_window);

  gtk_widget_class_bind_template_callback (widget_class, search_changed);
  gtk_widget_class_bind_template_callback (widget_class, search_preedit_changed);
  gtk_widget_class_bind_template_callback (widget_class, search_activated);
  gtk_widget_class_bind_template_callback (widget_class, search_gained_focus);
  gtk_widget_class_bind_template_callback (widget_class, search_lost_focus);

  signals[APP_LAUNCHED] = g_signal_new ("app-launched",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        0, NULL, NULL, NULL,
                                        G_TYPE_NONE, 1, G_TYPE_APP_INFO);

  gtk_widget_class_set_css_name (widget_class, "phosh-app-grid");
}


void
phosh_app_grid_reset (PhoshAppGrid *self)
{
  PhoshAppGridPrivate *priv;
  GtkAdjustment *adjustment;

  g_return_if_fail (PHOSH_IS_APP_GRID (self));

  priv = phosh_app_grid_get_instance_private (self);

  adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->scrolled_window));

  gtk_adjustment_set_value (adjustment, 0);
  gtk_entry_set_text (GTK_ENTRY (priv->search), "");
  g_clear_pointer (&priv->search_string, g_free);
}


GtkWidget *
phosh_app_grid_new (void)
{
  return g_object_new (PHOSH_TYPE_APP_GRID, NULL);
}
