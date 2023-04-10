/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-notification-frame"

#include "phosh-config.h"
#include "notification-content.h"
#include "notification-frame.h"
#include "notification-source.h"
#include "swipe-away-bin.h"
#include "util.h"
#include "timestamp-label.h"

#include <math.h>

/**
 * PhoshNotificationFrame:
 *
 * A frame containing one or more notifications
 */


enum {
  PROP_0,
  PROP_SHOW_BODY,
  PROP_ACTION_FILTER_KEYS,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];


struct _PhoshNotificationFrame {
  GtkEventBox parent;

  GListModel *model;
  gulong      model_watch;

  GBinding *bind_name;
  GBinding *bind_icon;
  GBinding *bind_timestamp;

  GtkWidget *box;
  GtkWidget *lbl_app_name;
  GtkWidget *img_icon;
  GtkWidget *list_notifs;
  GtkWidget *updated;

  gboolean   show_body;
  GStrv      action_filter_keys;

  /* needed so that the gestures aren't immediately destroyed */
  GtkGesture *header_click_gesture;
  GtkGesture *list_click_gesture;

  int start_x;
  int start_y;
  GtkListBoxRow *active_row;
};
typedef struct _PhoshNotificationFrame PhoshNotificationFrame;


G_DEFINE_TYPE (PhoshNotificationFrame, phosh_notification_frame, GTK_TYPE_EVENT_BOX)


#define DRAG_THRESHOLD_DISTANCE 16


enum {
  SIGNAL_EMPTY,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


static void
phosh_notification_frame_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  PhoshNotificationFrame *self = PHOSH_NOTIFICATION_FRAME (object);

  switch (property_id) {
  case PROP_SHOW_BODY:
    self->show_body = g_value_get_boolean (value);
    break;
  case PROP_ACTION_FILTER_KEYS:
    self->action_filter_keys = g_value_dup_boxed (value);
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
    case PROP_SHOW_BODY:
      g_value_set_boolean (value, self->show_body);
      break;
    case PROP_ACTION_FILTER_KEYS:
      g_value_set_boxed (value, self->action_filter_keys);
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

  /* Don't clear bindings, they're already unref'd before here */

  g_clear_signal_handler (&self->model_watch, self->model);

  g_clear_object (&self->model);
  g_clear_pointer (&self->action_filter_keys, g_strfreev);

  G_OBJECT_CLASS (phosh_notification_frame_parent_class)->finalize (object);
}


static gboolean
motion_notify (PhoshNotificationFrame *self,
               GdkEventMotion         *event)
{
  if (self->start_x >= 0 && self->start_y >= 0) {
    int current_x, current_y;
    double dx, dy;

    gtk_widget_translate_coordinates (GTK_WIDGET (self->box),
                                      gtk_widget_get_toplevel (GTK_WIDGET (self)),
                                      event->x, event->y,
                                      &current_x, &current_y);

    dx = current_x - self->start_x;
    dy = current_y - self->start_y;

    if (sqrt (dx * dx + dy * dy) > DRAG_THRESHOLD_DISTANCE) {
      gtk_gesture_set_state (self->header_click_gesture, GTK_EVENT_SEQUENCE_DENIED);
      gtk_gesture_set_state (self->list_click_gesture, GTK_EVENT_SEQUENCE_DENIED);
    }
  }

  return GDK_EVENT_PROPAGATE;
}


static void
pressed (PhoshNotificationFrame *self,
         int                     n_press,
         double                  x,
         double                  y,
         GtkGesture             *gesture,
         GtkGesture             *other_gesture)
{
  GdkEventSequence *sequence =
    gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture));

  if (n_press != 1) {
    gtk_gesture_set_sequence_state (gesture, sequence, GTK_EVENT_SEQUENCE_DENIED);

    return;
  }

  gtk_widget_translate_coordinates (self->box,
                                    gtk_widget_get_toplevel (GTK_WIDGET (self)),
                                    x, y,
                                    &self->start_x, &self->start_y);

  /* When the title row is clicked we proxy it to the first item */
  self->active_row =
    gtk_list_box_get_row_at_y (GTK_LIST_BOX (self->list_notifs),
                               gesture == self->header_click_gesture ? 0 : y);

  gtk_gesture_set_sequence_state (other_gesture, sequence, GTK_EVENT_SEQUENCE_DENIED);
}

static void
header_pressed (PhoshNotificationFrame *self,
                int                     n_press,
                double                  x,
                double                  y)
{
  pressed (self, n_press, x, y,
           self->header_click_gesture,
           self->list_click_gesture);
}


static void
list_pressed (PhoshNotificationFrame *self,
              int                     n_press,
              double                  x,
              double                  y)
{
  pressed (self, n_press, x, y,
           self->list_click_gesture,
           self->header_click_gesture);
}


static void
released (PhoshNotificationFrame *self,
          int                     n_press,
          double                  x,
          double                  y,
          GtkGesture             *gesture)
{
  GtkListBoxRow *pressed_row = self->active_row;
  GtkListBoxRow *active_row;
  PhoshNotificationContent *content;
  PhoshNotification *notification;

  /* When the title row is clicked we proxy it to the first item */
  active_row =
    gtk_list_box_get_row_at_y (GTK_LIST_BOX (self->list_notifs),
                               gesture == self->header_click_gesture ? 0 : y);

  self->active_row = NULL;
  self->start_x = -1;
  self->start_y = -1;

  if (pressed_row != active_row) {
    gtk_gesture_set_state (gesture, GTK_EVENT_SEQUENCE_DENIED);

    return;
  }

  content = PHOSH_NOTIFICATION_CONTENT (active_row);
  notification = phosh_notification_content_get_notification (content);

  phosh_notification_activate (notification,
                               PHOSH_NOTIFICATION_DEFAULT_ACTION);
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
removed (PhoshNotificationFrame *self)
{
  guint i, n;

  n = g_list_model_get_n_items (self->model);
  for (i = 0; i < n; i++) {
    g_autoptr (PhoshNotification) notification = NULL;

    notification = g_list_model_get_item (self->model, 0);

    phosh_notification_close (notification, PHOSH_NOTIFICATION_REASON_CLOSED);
  }
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
   * PhoshNotificationFrame:show-body:
   *
   * Whether notificaions in this frame should show the notification body
   */
  props[PROP_SHOW_BODY] =
    g_param_spec_boolean ("show-body",
                          "",
                          "",
                          TRUE,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS);


  /**
   * PhoshNotificationFrame:action-filter-keys:
   *
   * The keys will be used to look up filter values in the
   * applications desktop file. Actions starting with those values
   * will be used on the lock screen.
   */
  props[PROP_ACTION_FILTER_KEYS] =
    g_param_spec_boxed ("action-filter-keys", "", "",
                        G_TYPE_STRV,
                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
                        G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  signals[SIGNAL_EMPTY] = g_signal_new ("empty",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        0,
                                        NULL,
                                        NULL,
                                        NULL,
                                        G_TYPE_NONE,
                                        0);
  g_type_ensure (PHOSH_TYPE_TIMESTAMP_LABEL);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/notification-frame.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationFrame, box);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationFrame, lbl_app_name);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationFrame, img_icon);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationFrame, list_notifs);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationFrame, updated);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationFrame, header_click_gesture);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationFrame, list_click_gesture);

  gtk_widget_class_bind_template_callback (widget_class, motion_notify);
  gtk_widget_class_bind_template_callback (widget_class, header_pressed);
  gtk_widget_class_bind_template_callback (widget_class, list_pressed);
  gtk_widget_class_bind_template_callback (widget_class, released);
  gtk_widget_class_bind_template_callback (widget_class, notification_activated);
  gtk_widget_class_bind_template_callback (widget_class, removed);

  gtk_widget_class_set_css_name (widget_class, "phosh-notification-frame");
}


static void
phosh_notification_frame_init (PhoshNotificationFrame *self)
{
  self->show_body = TRUE;
  self->start_x = -1;
  self->start_y = -1;

  g_type_ensure (PHOSH_TYPE_SWIPE_AWAY_BIN);

  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_notification_frame_new (gboolean show_body, const char * const *action_filter_keys)
{
  return g_object_new (PHOSH_TYPE_NOTIFICATION_FRAME,
                       "show-body", show_body,
                       "action-filter-keys", action_filter_keys,
                       NULL);
}


static GtkWidget *
create_row (gpointer item, gpointer data)
{
  PhoshNotification *notification = item;
  PhoshNotificationFrame *self = PHOSH_NOTIFICATION_FRAME (data);

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION_FRAME (self), NULL);

  if (self->action_filter_keys) {
    for (int i = 0; i < g_strv_length (self->action_filter_keys); i++)
      g_debug ("%s: %s", __func__, self->action_filter_keys[i]);
  }
  return phosh_notification_content_new (notification, self->show_body,
                                         (const char * const *)self->action_filter_keys);
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

  /* Disconnect from the last notification (if any) */
  g_clear_object (&self->bind_name);
  g_clear_object (&self->bind_icon);
  g_clear_object (&self->bind_timestamp);

  /* Get the latest notification in the model */
  notification = g_list_model_get_item (self->model, 0);

  if (notification == NULL) {
    /* No first notification means no notifications aka we're empty
     * and should be removed from $thing we're in (banner or list)
     */
    g_signal_emit (self, signals[SIGNAL_EMPTY], 0);

    return;
  }

  /* Bind to the new one */
  self->bind_name = g_object_bind_property (notification,       "app-name",
                                            self->lbl_app_name, "label",
                                            G_BINDING_SYNC_CREATE);

  self->bind_icon = g_object_bind_property (notification,   "app-icon",
                                            self->img_icon, "gicon",
                                            G_BINDING_SYNC_CREATE);

  self->bind_timestamp = g_object_bind_property (notification,   "timestamp",
                                                 self->updated,  "timestamp",
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


const char * const *
phosh_notification_frame_get_action_filter_keys (PhoshNotificationFrame *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION_FRAME (self), NULL);

  return (const char *const *)self->action_filter_keys;
}
