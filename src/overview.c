/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-overview"

#include "config.h"

#include "overview.h"
#include "activity.h"
#include "app-grid.h"
#include "app-grid-button.h"
#include "shell.h"
#include "util.h"
#include "toplevel-manager.h"
#include "toplevel-thumbnail.h"
#include "phosh-private-client-protocol.h"
#include "phosh-wayland.h"

#include <gio/gdesktopappinfo.h>

#define HANDY_USE_UNSTABLE_API
#include <handy.h>

#define OVERVIEW_ICON_SIZE 64

/**
 * SECTION:overview
 * @short_description: The overview shows running apps and the
 * app grid to launch new applications.
 * @Title: PhoshOverview
 *
 * The #PhoshOverview shows running apps (#PhoshActivity) and
 * the app grid (#PhoshAppGrid) to launch new applications.
 */

enum {
  ACTIVITY_LAUNCHED,
  ACTIVITY_RAISED,
  ACTIVITY_CLOSED,
  SELECTION_ABORTED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

typedef struct
{
  /* Running activities */
  GtkWidget *paginator_running_activities;
  GtkWidget *app_grid;
} PhoshOverviewPrivate;


struct _PhoshOverview
{
  GtkBoxClass parent;
};

G_DEFINE_TYPE_WITH_PRIVATE (PhoshOverview, phosh_overview, GTK_TYPE_BOX)

static PhoshToplevel *
get_toplevel_from_activity (PhoshActivity *activity)
{
  PhoshToplevel *toplevel;
  g_return_val_if_fail (PHOSH_IS_ACTIVITY (activity), NULL);
  toplevel = g_object_get_data (G_OBJECT (activity), "toplevel");
  g_return_val_if_fail (PHOSH_IS_TOPLEVEL (toplevel), NULL);

  return toplevel;
}

static GtkWidget *
find_activity_by_toplevel (PhoshOverview        *self,
                           PhoshToplevel        *needle)
{
  g_autoptr(GList) children;
  GtkWidget *activity = NULL;
  PhoshOverviewPrivate *priv = phosh_overview_get_instance_private (self);

  children = gtk_container_get_children (GTK_CONTAINER (priv->paginator_running_activities));
  for (GList *l = children; l; l = l->next) {
    PhoshToplevel *toplevel;

    activity = l->data;
    toplevel = get_toplevel_from_activity (activity);
    if (toplevel == needle)
      break;
  }

  return activity;
}

static void
on_activity_clicked (PhoshOverview *self, PhoshActivity *activity)
{
  PhoshToplevel *toplevel;
  g_return_if_fail (PHOSH_IS_OVERVIEW (self));
  g_return_if_fail (PHOSH_IS_ACTIVITY (activity));

  toplevel = get_toplevel_from_activity (activity);
  g_return_if_fail (toplevel);

  g_debug("Will raise %s (%s)",
          phosh_activity_get_app_id (activity),
          phosh_activity_get_title (activity));

  phosh_toplevel_activate (toplevel, phosh_wayland_get_wl_seat (phosh_wayland_get_default ()));
  g_signal_emit (self, signals[ACTIVITY_RAISED], 0);
}


static void
on_activity_close_clicked (PhoshOverview *self, PhoshActivity *activity)
{
  PhoshToplevel *toplevel;

  g_return_if_fail (PHOSH_IS_OVERVIEW (self));
  g_return_if_fail (PHOSH_IS_ACTIVITY (activity));

  toplevel = g_object_get_data (G_OBJECT (activity), "toplevel");
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));

  g_debug ("Will close %s (%s)",
           phosh_activity_get_app_id (activity),
           phosh_activity_get_title (activity));

  phosh_toplevel_close (toplevel);
  phosh_trigger_feedback ("window-close");
  g_signal_emit (self, signals[ACTIVITY_CLOSED], 0);
}

static void
on_toplevel_closed (PhoshToplevel *toplevel, PhoshActivity *activity)
{
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));
  g_return_if_fail (PHOSH_IS_ACTIVITY (activity));
  gtk_widget_destroy (GTK_WIDGET (activity));
}

static void
on_toplevel_activated_changed (PhoshToplevel *toplevel, GParamSpec *pspec, PhoshOverview *overview)
{
  PhoshActivity *activity;
  PhoshOverviewPrivate *priv;
  g_return_if_fail (PHOSH_IS_OVERVIEW (overview));
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));
  priv = phosh_overview_get_instance_private (overview);

  activity = PHOSH_ACTIVITY (find_activity_by_toplevel (overview, toplevel));
  if (phosh_toplevel_is_activated (toplevel))
    hdy_paginator_scroll_to (HDY_PAGINATOR (priv->paginator_running_activities), GTK_WIDGET (activity));
}

static void
on_thumbnail_ready_changed (PhoshThumbnail *thumbnail, GParamSpec *pspec, PhoshActivity *activity)
{
  g_return_if_fail (PHOSH_IS_THUMBNAIL (thumbnail));
  g_return_if_fail (PHOSH_IS_ACTIVITY (activity));

  phosh_activity_set_thumbnail (activity, thumbnail);
}


static void
request_thumbnail (PhoshOverview *self, PhoshToplevel *toplevel)
{
  PhoshToplevelThumbnail *thumbnail;
  gint width, height;
  PhoshActivity *activity = PHOSH_ACTIVITY (find_activity_by_toplevel (self, toplevel));
  gint scale = gtk_widget_get_scale_factor (GTK_WIDGET (activity));
  g_object_get (activity, "win-width", &width, "win-height", &height, NULL);
  thumbnail = phosh_toplevel_thumbnail_new_from_toplevel (toplevel, width * scale, height * scale);
  g_signal_connect_object (thumbnail, "notify::ready", G_CALLBACK (on_thumbnail_ready_changed), activity, 0);
}

static void
add_activity (PhoshOverview *self, PhoshToplevel *toplevel)
{
  PhoshMonitor *monitor = phosh_shell_get_primary_monitor (phosh_shell_get_default ());
  PhoshOverviewPrivate *priv;
  GtkWidget *activity;
  const gchar *app_id, *title;

  g_return_if_fail (PHOSH_IS_OVERVIEW (self));
  priv = phosh_overview_get_instance_private (self);

  app_id = phosh_toplevel_get_app_id (toplevel);
  title = phosh_toplevel_get_title (toplevel);

  g_debug ("Building activator for '%s' (%s)", app_id, title);
  activity = phosh_activity_new (app_id, title);
  g_object_set (activity,
                "win-width", monitor->width / monitor->scale,  // TODO: Get the real size somehow
                "win-height", monitor->height / monitor->scale,
                "maximized", phosh_toplevel_is_maximized (toplevel),
                NULL);
  g_object_set_data (G_OBJECT (activity), "toplevel", toplevel);

  gtk_container_add (GTK_CONTAINER (priv->paginator_running_activities), activity);
  gtk_widget_show (activity);

  g_signal_connect_swapped (activity, "clicked", G_CALLBACK (on_activity_clicked), self);
  g_signal_connect_swapped (activity, "close-clicked",
                            G_CALLBACK (on_activity_close_clicked), self);

  g_signal_connect_object (toplevel, "closed", G_CALLBACK (on_toplevel_closed), activity, 0);
  g_signal_connect_object (toplevel, "notify::activated", G_CALLBACK (on_toplevel_activated_changed), self, 0);
  g_object_bind_property (toplevel, "maximized", activity, "maximized", G_BINDING_DEFAULT);

  phosh_connect_button_feedback (GTK_BUTTON (activity));

  if (phosh_toplevel_is_activated (toplevel))
    hdy_paginator_scroll_to (HDY_PAGINATOR (priv->paginator_running_activities), activity);
}

static void
get_running_activities (PhoshOverview *self)
{
  PhoshOverviewPrivate *priv;
  PhoshToplevelManager *toplevel_manager = phosh_shell_get_toplevel_manager (phosh_shell_get_default ());
  guint toplevels_num = phosh_toplevel_manager_get_num_toplevels (toplevel_manager);
  g_return_if_fail (PHOSH_IS_OVERVIEW (self));
  priv = phosh_overview_get_instance_private (self);

  if (toplevels_num == 0)
    gtk_widget_hide (priv->paginator_running_activities);

  for (guint i = 0; i < toplevels_num; i++) {
    PhoshToplevel *toplevel = phosh_toplevel_manager_get_toplevel (toplevel_manager, i);
    add_activity (self, toplevel);
  }
}

static void
toplevel_added_cb (PhoshOverview        *self,
                   PhoshToplevel        *toplevel,
                   PhoshToplevelManager *manager)
{
  g_return_if_fail (PHOSH_IS_OVERVIEW (self));
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));
  g_return_if_fail (PHOSH_IS_TOPLEVEL_MANAGER (manager));
  add_activity (self, toplevel);
}

static void
toplevel_changed_cb (PhoshOverview        *self,
                     PhoshToplevel        *toplevel,
                     PhoshToplevelManager *manager)
{
  GtkWidget *activity;

  g_return_if_fail (PHOSH_IS_OVERVIEW (self));
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));
  g_return_if_fail (PHOSH_IS_TOPLEVEL_MANAGER (manager));

  activity = find_activity_by_toplevel (self, toplevel);
  g_return_if_fail (GTK_IS_WIDGET (activity));

  /* TODO: update other properties */
  phosh_activity_set_title (PHOSH_ACTIVITY (activity),
                            phosh_toplevel_get_title (toplevel));
  request_thumbnail (self, toplevel);
}

static void
num_toplevels_cb (PhoshOverview        *self,
                   GParamSpec *pspec,
                   PhoshToplevelManager *manager)
{
  PhoshOverviewPrivate *priv;
  g_return_if_fail (PHOSH_IS_OVERVIEW (self));
  g_return_if_fail (PHOSH_IS_TOPLEVEL_MANAGER (manager));
  priv = phosh_overview_get_instance_private (self);
  if (phosh_toplevel_manager_get_num_toplevels (manager)) {
    gtk_widget_show (priv->paginator_running_activities);
  } else {
    gtk_widget_hide (priv->paginator_running_activities);
  }
}

static void
phosh_overview_size_allocate (GtkWidget     *widget,
                              GtkAllocation *alloc)
{
  PhoshOverview *self = PHOSH_OVERVIEW (widget);
  PhoshOverviewPrivate *priv = phosh_overview_get_instance_private (self);
  GList *children, *l;

  children = gtk_container_get_children (GTK_CONTAINER (priv->paginator_running_activities));

  for (l = children; l; l = l->next) {
    gint old_width, old_height;
    g_object_get (l->data,
                 "win-width", &old_width,
                 "win-height", &old_height,
                 NULL);
    if (old_width != alloc->width || old_height != alloc->height) {
      g_object_set (l->data,
                    "win-width", alloc->width,
                    "win-height", alloc->height,
                    NULL);
      request_thumbnail (self, get_toplevel_from_activity (PHOSH_ACTIVITY (l->data)));
    }
  }

  g_list_free (children);

  GTK_WIDGET_CLASS (phosh_overview_parent_class)->size_allocate (widget, alloc);
}

static void
app_launched_cb (PhoshOverview *self,
                 GAppInfo      *info,
                 GtkWidget     *widget)
{
  g_return_if_fail (PHOSH_IS_OVERVIEW (self));

  g_signal_emit (self, signals[ACTIVITY_LAUNCHED], 0);
}


static void
phosh_overview_constructed (GObject *object)
{
  PhoshOverview *self = PHOSH_OVERVIEW (object);
  PhoshOverviewPrivate *priv = phosh_overview_get_instance_private (self);
  PhoshToplevelManager *toplevel_manager =
      phosh_shell_get_toplevel_manager (phosh_shell_get_default ());

  G_OBJECT_CLASS (phosh_overview_parent_class)->constructed (object);

  g_signal_connect_object (toplevel_manager, "toplevel-added",
                           G_CALLBACK (toplevel_added_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (toplevel_manager, "toplevel-changed",
                           G_CALLBACK (toplevel_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (toplevel_manager, "notify::num-toplevels",
                           G_CALLBACK (num_toplevels_cb),
                           self,
                           G_CONNECT_SWAPPED);

  get_running_activities (self);

  g_signal_connect_swapped (priv->app_grid, "app-launched",
                            G_CALLBACK (app_launched_cb), self);
}


static void
phosh_overview_class_init (PhoshOverviewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_overview_constructed;
  widget_class->size_allocate = phosh_overview_size_allocate;

  gtk_widget_class_set_css_name (widget_class, "phosh-overview");

  /* ensure used custom types */
  PHOSH_TYPE_APP_GRID;
  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/overview.ui");

  gtk_widget_class_bind_template_child_private (widget_class, PhoshOverview, paginator_running_activities);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshOverview, app_grid);

  signals[ACTIVITY_LAUNCHED] = g_signal_new ("activity-launched",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
  signals[ACTIVITY_RAISED] = g_signal_new ("activity-raised",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
  signals[SELECTION_ABORTED] = g_signal_new ("selection-aborted",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
  signals[ACTIVITY_CLOSED] = g_signal_new ("activity-closed",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  gtk_widget_class_set_css_name (widget_class, "phosh-overview");
}


static void
phosh_overview_init (PhoshOverview *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


void
phosh_overview_reset (PhoshOverview *self)
{
  PhoshOverviewPrivate *priv;
  g_return_if_fail(PHOSH_IS_OVERVIEW (self));
  priv = phosh_overview_get_instance_private (self);
  phosh_app_grid_reset (PHOSH_APP_GRID (priv->app_grid));
}


GtkWidget *
phosh_overview_new (void)
{
  return g_object_new (PHOSH_TYPE_OVERVIEW, NULL);
}
