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

/**
 * SECTION:phosh-notification-frame
 * @short_description: A frame containing one or more notifications
 * @Title: PhoshNotificationFrame
 */

enum {
  PROP_0,
  PROP_NOTIFICATION,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];


struct _PhoshNotificationFrame {
  GtkBox parent;

  PhoshNotification *notification;

  GtkWidget *lbl_app_name;
  GtkWidget *img_icon;
  GtkWidget *list_notifs;
};
typedef struct _PhoshNotificationFrame PhoshNotificationFrame;


G_DEFINE_TYPE (PhoshNotificationFrame, phosh_notification_frame, GTK_TYPE_BOX)


static void
phosh_notification_frame_set_notification (PhoshNotificationFrame *self,
                                            PhoshNotification     *notification)
{
  GtkWidget *content;

  g_set_object (&self->notification, notification);

  g_object_bind_property (self->notification, "app-name",
                          self->lbl_app_name, "label",
                          G_BINDING_SYNC_CREATE);

  // If unset PhoshNotification provides a fallback
  g_object_bind_property (self->notification, "app-icon",
                          self->img_icon,     "gicon",
                          G_BINDING_SYNC_CREATE);

  content = phosh_notification_content_new (self->notification);
  gtk_container_add (GTK_CONTAINER (self->list_notifs), content);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_NOTIFICATION]);
}


static void
phosh_notification_frame_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  PhoshNotificationFrame *self = PHOSH_NOTIFICATION_FRAME (object);

  switch (property_id) {
    case PROP_NOTIFICATION:
      phosh_notification_frame_set_notification (self,
                                                 g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_notification_frame_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  PhoshNotificationFrame *self = PHOSH_NOTIFICATION_FRAME (object);

  switch (property_id) {
    case PROP_NOTIFICATION:
      g_value_set_object (value, self->notification);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_notification_frame_finalize (GObject *object)
{
  PhoshNotificationFrame *self = PHOSH_NOTIFICATION_FRAME (object);

  g_clear_object (&self->notification);

  G_OBJECT_CLASS (phosh_notification_frame_parent_class)->finalize (object);
}


static gboolean
header_activated (PhoshNotificationFrame *self, GdkEventButton *event)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION_FRAME (self), FALSE);

  phosh_notification_activate (self->notification,
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

  gtk_container_remove (GTK_CONTAINER (list), GTK_WIDGET (row));

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
  object_class->set_property = phosh_notification_frame_set_property;
  object_class->get_property = phosh_notification_frame_get_property;

  /**
   * PhoshNotificationFrame:notification:
   * @self: the #PhoshNotificationFrame
   *
   * The #PhoshNotification shown in @self
   */
  props[PROP_NOTIFICATION] =
    g_param_spec_object ("notification",
                         "Notification",
                         "Notification in the frame",
                         PHOSH_TYPE_NOTIFICATION,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

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
phosh_notification_frame_new (PhoshNotification *notification)
{
  return g_object_new (PHOSH_TYPE_NOTIFICATION_FRAME,
                       "notification", notification,
                       NULL);
}


PhoshNotification *
phosh_notification_frame_get_notification (PhoshNotificationFrame *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION_FRAME (self), NULL);

  return self->notification;
}
