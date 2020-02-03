/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-notification-content"

#include "config.h"
#include "notification-content.h"


/**
 * SECTION:phosh-notification-content
 * @short_description: Content of a notification
 * @Title: PhoshNotificationContent
 */

enum {
  PROP_0,
  PROP_NOTIFICATION,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];


struct _PhoshNotificationContent {
  GtkListBoxRow parent;

  PhoshNotification *notification;

  GtkWidget *lbl_summary;
  GtkWidget *lbl_body;
  GtkWidget *img_image;
  GtkWidget *box_actions;
};
typedef struct _PhoshNotificationContent PhoshNotificationContent;


G_DEFINE_TYPE (PhoshNotificationContent, phosh_notification_content, GTK_TYPE_LIST_BOX_ROW)


static gboolean
set_image (GBinding     *binding,
           const GValue *from_value,
           GValue       *to_value,
           gpointer      user_data)
{
  PhoshNotificationContent *self = user_data;
  GIcon *image = g_value_get_object (from_value);

  gtk_widget_set_visible (self->img_image, image != NULL);

  g_value_set_object (to_value, image);

  return TRUE;
}


static gboolean
set_summary (GBinding     *binding,
             const GValue *from_value,
             GValue       *to_value,
             gpointer      user_data)
{
  PhoshNotificationContent *self = user_data;
  const char* summary = g_value_get_string (from_value);

  if (summary != NULL && g_strcmp0 (summary, "")) {
    gtk_widget_show (self->lbl_summary);
  } else {
    gtk_widget_hide (self->lbl_summary);
  }

  g_value_set_string (to_value, summary);

  return TRUE;
}


static gboolean
set_body (GBinding     *binding,
          const GValue *from_value,
          GValue       *to_value,
          gpointer      user_data)
{
  PhoshNotificationContent *self = user_data;
  const char* body = g_value_get_string (from_value);
  gboolean visible;

  visible = body != NULL && g_strcmp0 (body, "");
  gtk_widget_set_visible (self->lbl_body, visible);

  g_value_set_string (to_value, body);

  return TRUE;
}


static void
set_actions (PhoshNotification        *notification,
             GParamSpec               *pspec,
             PhoshNotificationContent *self)
{
  GStrv actions = phosh_notification_get_actions (notification);

  g_return_if_fail (PHOSH_IS_NOTIFICATION_CONTENT (self));

  gtk_container_foreach (GTK_CONTAINER (self->box_actions),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  if (actions == NULL) {
    return;
  }

  for (int i = 0; actions[i] != NULL; i += 2) {
    GtkWidget *btn;
    GtkWidget *lbl;

    // The default action is already trigged by the notification body
    if (g_strcmp0 (actions[i], "default") == 0) {
      continue;
    }

    if (actions[i + 1] == NULL) {
      g_warning ("Expected action label at %i, got NULL", i);

      break;
    }

    lbl = g_object_new (GTK_TYPE_LABEL,
                        "label", actions[i + 1],
                        "xalign", 0.0,
                        "visible", TRUE,
                        NULL);

    btn = g_object_new (GTK_TYPE_BUTTON,
                        "action-name", "noti.activate",
                        "action-target", g_variant_new_string (actions[i]),
                        "visible", TRUE,
                        NULL);
    gtk_container_add (GTK_CONTAINER (btn), lbl);

    gtk_container_add (GTK_CONTAINER (self->box_actions), btn);
  }
}


static void
phosh_notification_content_set_notification (PhoshNotificationContent *self,
                                            PhoshNotification       *notification)
{
  g_set_object (&self->notification, notification);

  // Use the "transform" function to show/hide when set/unset
  g_object_bind_property_full (self->notification, "image",
                               self->img_image,    "gicon",
                               G_BINDING_SYNC_CREATE,
                               set_image,
                               NULL,
                               self,
                               NULL);

  g_object_bind_property_full (self->notification, "summary",
                               self->lbl_summary,  "label",
                               G_BINDING_SYNC_CREATE,
                               set_summary,
                               NULL,
                               self,
                               NULL);

  g_object_bind_property_full (self->notification, "body",
                               self->lbl_body,     "label",
                               G_BINDING_SYNC_CREATE,
                               set_body,
                               NULL,
                               self,
                               NULL);

  g_signal_connect (self->notification, "notify::actions",
                    G_CALLBACK (set_actions), self);
  set_actions (self->notification, NULL, self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_NOTIFICATION]);
}


static void
phosh_notification_content_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  PhoshNotificationContent *self = PHOSH_NOTIFICATION_CONTENT (object);

  switch (property_id) {
    case PROP_NOTIFICATION:
      phosh_notification_content_set_notification (self,
                                                  g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_notification_content_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  PhoshNotificationContent *self = PHOSH_NOTIFICATION_CONTENT (object);

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
phosh_notification_content_finalize (GObject *object)
{
  PhoshNotificationContent *self = PHOSH_NOTIFICATION_CONTENT (object);

  g_clear_object (&self->notification);

  G_OBJECT_CLASS (phosh_notification_content_parent_class)->finalize (object);
}


static void
phosh_notification_content_class_init (PhoshNotificationContentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_notification_content_finalize;
  object_class->set_property = phosh_notification_content_set_property;
  object_class->get_property = phosh_notification_content_get_property;

  /**
   * PhoshNotificationContent:notification:
   * @self: the #PhoshNotificationContent
   *
   * The #PhoshNotification shown in @self
   */
  props[PROP_NOTIFICATION] =
    g_param_spec_object ("notification",
                         "Notification",
                         "Notification being shown",
                         PHOSH_TYPE_NOTIFICATION,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/notification-content.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationContent, lbl_summary);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationContent, lbl_body);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationContent, img_image);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationContent, box_actions);

  gtk_widget_class_set_css_name (widget_class, "phosh-notification-content");
}


static void
action_activate (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       data)
{
  PhoshNotificationContent *self = data;
  const char *target;

  g_return_if_fail (PHOSH_IS_NOTIFICATION_CONTENT (self));

  target = g_variant_get_string (parameter, NULL);

  phosh_notification_activate (self->notification, target);
}


static GActionEntry entries[] = {
  { "activate", action_activate, "s", NULL, NULL },
};


static void
phosh_notification_content_init (PhoshNotificationContent *self)
{
  g_autoptr (GActionMap) map = NULL;

  map = G_ACTION_MAP (g_simple_action_group_new ());
  g_action_map_add_action_entries (map,
                                   entries,
                                   G_N_ELEMENTS (entries),
                                   self);
  gtk_widget_insert_action_group (GTK_WIDGET (self),
                                  "noti",
                                  G_ACTION_GROUP (map));

  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_notification_content_new (PhoshNotification *notification)
{
  return g_object_new (PHOSH_TYPE_NOTIFICATION_CONTENT,
                       "notification", notification,
                       NULL);
}


PhoshNotification *
phosh_notification_content_get_notification (PhoshNotificationContent *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION_CONTENT (self), NULL);

  return self->notification;
}
