/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3.0+
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-notification-frame"

#include "config.h"
#include "notification-content.h"
#include "notification-frame.h"
#include "notification-source.h"
#include "util.h"

/**
 * SECTION:notification-frame
 * @short_description: A frame containing one or more notifications
 * @Title: PhoshNotificationFrame
 */


struct _PhoshNotificationFrame {
  GtkBox parent;

  GListModel *model;
  gulong      model_watch;

  GBinding *bind_name;
  GBinding *bind_icon;

  GtkWidget *lbl_app_name;
  GtkWidget *img_icon;
  GtkWidget *list_notifs;
};
typedef struct _PhoshNotificationFrame PhoshNotificationFrame;


G_DEFINE_TYPE (PhoshNotificationFrame, phosh_notification_frame, GTK_TYPE_BOX)


enum {
  SIGNAL_EMPTY,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


static void
phosh_notification_frame_finalize (GObject *object)
{
  PhoshNotificationFrame *self = PHOSH_NOTIFICATION_FRAME (object);

  // Don't clear bindings, they're already unref'd before here

  phosh_clear_handler (&self->model_watch, self->model);

  g_clear_object (&self->model);

  G_OBJECT_CLASS (phosh_notification_frame_parent_class)->finalize (object);
}


// When the title row is clicked we proxy it to the first item
static gboolean
header_activated (PhoshNotificationFrame *self, GdkEventButton *event)
{
  g_autoptr (PhoshNotification) notification = NULL;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION_FRAME (self), FALSE);

  notification = g_list_model_get_item (self->model, 0);

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (notification), FALSE);

  phosh_notification_activate (notification,
                               PHOSH_NOTIFICATION_DEFAULT_ACTION);

  return FALSE;
}


static void
notification_activated (PhoshNotificationFrame *self,
                        GtkListBoxRow          *row,
                        GtkListBox             *list)
{
  PhoshNotificationContent *content;
  PhoshNotification *notification;

  g_return_if_fail (PHOSH_IS_NOTIFICATION_CONTENT (row));

  content = PHOSH_NOTIFICATION_CONTENT (row);
  notification = phosh_notification_content_get_notification (content);

  phosh_notification_activate (notification,
                               PHOSH_NOTIFICATION_DEFAULT_ACTION);
}


static void
phosh_notification_frame_class_init (PhoshNotificationFrameClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_notification_frame_finalize;

  signals[SIGNAL_EMPTY] = g_signal_new ("empty",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        0,
                                        NULL,
                                        NULL,
                                        NULL,
                                        G_TYPE_NONE,
                                        0);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/notification-frame.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationFrame, lbl_app_name);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationFrame, img_icon);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationFrame, list_notifs);

  gtk_widget_class_bind_template_callback (widget_class, header_activated);
  gtk_widget_class_bind_template_callback (widget_class, notification_activated);

  gtk_widget_class_set_css_name (widget_class, "phosh-notification-frame");
}


static void
phosh_notification_frame_init (PhoshNotificationFrame *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_notification_frame_new (void)
{
  return g_object_new (PHOSH_TYPE_NOTIFICATION_FRAME, NULL);
}


static GtkWidget *
create_row (gpointer item, gpointer data)
{
  PhoshNotification *notification = item;

  return phosh_notification_content_new (notification);
}


static void
items_changed (GListModel             *list,
               guint                   position,
               guint                   removed,
               guint                   added,
               PhoshNotificationFrame *self)
{
  g_autoptr (PhoshNotification) notification = NULL;

  g_return_if_fail (PHOSH_IS_NOTIFICATION_FRAME (self));

  // Disconnect from the last notification (if any)
  g_clear_object (&self->bind_name);
  g_clear_object (&self->bind_icon);

  // Get the latest notification in the model
  notification = g_list_model_get_item (self->model, 0);

  if (notification == NULL) {
    /* No first notification means no notfications aka we're empty
     * and should be removed from $thing we're in (banner or list)
     */
    g_signal_emit (self, signals[SIGNAL_EMPTY], 0);

    return;
  }

  // Bind to the new one
  self->bind_name = g_object_bind_property (notification,       "app-name",
                                            self->lbl_app_name, "label",
                                            G_BINDING_SYNC_CREATE);

  self->bind_icon = g_object_bind_property (notification,   "app-icon",
                                            self->img_icon, "gicon",
                                            G_BINDING_SYNC_CREATE);
}


void
phosh_notification_frame_bind_model (PhoshNotificationFrame *self,
                                     GListModel             *model)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION_FRAME (self));
  g_return_if_fail (G_IS_LIST_MODEL (model));
  g_return_if_fail (g_type_is_a (g_list_model_get_item_type (model),
                                 PHOSH_TYPE_NOTIFICATION));

  g_set_object (&self->model, model);

  gtk_list_box_bind_model (GTK_LIST_BOX (self->list_notifs),
                           model,
                           create_row,
                           self,
                           NULL);

  self->model_watch = g_signal_connect (model, "items-changed",
                                        G_CALLBACK (items_changed), self);
  items_changed (model, 0, 0, 0, self);
}


/**
 * phosh_notification_frame_bind_notification:
 * @self: the #PhoshNotificationFrame
 * @notification: a #PhoshNotification
 *
 * Helper function for frames that only need to contain a single notification
 *
 * Wraps phosh_notification_frame_bind_model() by placing @notification in
 * a #PhoshNotificationSource
 */
void
phosh_notification_frame_bind_notification (PhoshNotificationFrame *self,
                                            PhoshNotification      *notification)
{
  g_autoptr (PhoshNotificationSource) store = NULL;

  g_return_if_fail (PHOSH_IS_NOTIFICATION_FRAME (self));
  g_return_if_fail (PHOSH_IS_NOTIFICATION (notification));

  store = phosh_notification_source_new ("dummy");

  phosh_notification_source_add (store, notification);

  phosh_notification_frame_bind_model (self, G_LIST_MODEL (store));
}
