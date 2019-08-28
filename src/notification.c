/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-notification"

#include "config.h"
#include "notification.h"
#include "shell.h"

/**
 * SECTION:phosh-notification
 * @short_description: A notification
 * @Title: PhoshNotification
 */

enum {
  PROP_0,
  PROP_APP_NAME,
  PROP_SUMMARY,
  PROP_BODY,
  PROP_APP_ICON,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];

enum {
  SIGNAL_DISMISSED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];

typedef struct _PhoshNotification
{
  PhoshLayerSurface parent;

  GtkWidget *lbl_app_name;
  GtkWidget *lbl_summary;
  GtkWidget *lbl_body;
  GtkWidget *img_icon;
  gchar *app_icon;
} PhoshNotification;

G_DEFINE_TYPE(PhoshNotification, phosh_notification, PHOSH_TYPE_LAYER_SURFACE)


static void
phosh_notification_set_property (GObject *object,
                             guint property_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
  PhoshNotification *self = PHOSH_NOTIFICATION (object);

  switch (property_id) {
  case PROP_APP_NAME:
    phosh_notification_set_app_name (self, g_value_get_string (value));
    break;
  case PROP_SUMMARY:
    phosh_notification_set_summary (self, g_value_get_string (value));
    break;
  case PROP_BODY:
    gtk_label_set_label (GTK_LABEL (self->lbl_body), g_value_get_string (value));
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_BODY]);
    break;
  case PROP_APP_ICON:
    phosh_notification_set_app_icon (self, g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_notification_get_property (GObject *object,
                             guint property_id,
                             GValue *value,
                             GParamSpec *pspec)
{
  PhoshNotification *self = PHOSH_NOTIFICATION (object);

  switch (property_id) {
  case PROP_APP_NAME:
    g_value_set_string (value, gtk_label_get_label (GTK_LABEL (self->lbl_app_name)));
    break;
  case PROP_SUMMARY:
    g_value_set_string (value, gtk_label_get_label (GTK_LABEL (self->lbl_summary)));
    break;
  case PROP_BODY:
    g_value_set_string (value, gtk_label_get_label (GTK_LABEL (self->lbl_body)));
    break;
  case PROP_APP_ICON:
    g_value_set_string (value, self->app_icon);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static gboolean
evbox_button_press_event_cb (PhoshNotification *self, GdkEventButton *event)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), FALSE);

  g_signal_emit(self, signals[SIGNAL_DISMISSED], 0);
  return FALSE;
}


static void
phosh_notification_class_init (PhoshNotificationClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = phosh_notification_set_property;
  object_class->get_property = phosh_notification_get_property;

  props[PROP_APP_NAME] =
    g_param_spec_string (
      "app-name",
      "App Name",
      "The applications's name",
      "",
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_SUMMARY] =
    g_param_spec_string (
      "summary",
      "Summary",
      "The notification's summary",
      "",
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_BODY] =
    g_param_spec_string (
      "body",
      "Body",
      "The notification's body",
      "",
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_APP_ICON] =
    g_param_spec_string (
      "app-icon",
      "App Icon",
      "Application icon",
      "",
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  /**
   * PhoshNotifiation::dismissed:
   *
   * Notification is dismissed by the user and can be closed
   */
  signals[SIGNAL_DISMISSED] = g_signal_new ("dismissed",
                                            G_TYPE_FROM_CLASS (klass),
                                            G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                            NULL, G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/notification.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshNotification, lbl_app_name);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotification, lbl_summary);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotification, lbl_body);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotification, img_icon);
  gtk_widget_class_bind_template_callback (widget_class, evbox_button_press_event_cb);
}


static void
phosh_notification_init (PhoshNotification *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


PhoshNotification *
phosh_notification_new (const char *app_name,const char *summary,
                        const char *body, const char *app_icon)
{
  PhoshWayland *wl = phosh_wayland_get_default ();

  return g_object_new (PHOSH_TYPE_NOTIFICATION,
                       "app_name", app_name,
                       "summary", summary,
                       "body", body,
                       "app_icon", app_icon,
                       /* layer surface */
                       "layer-shell", phosh_wayland_get_zwlr_layer_shell_v1(wl),
                       "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
                       "height", 100,
                       "width", 200,
                       "margin-top", 20,
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", 0,
                       "namespace", "phosh notification",
                       NULL);
}

void
phosh_notification_set_app_icon (PhoshNotification *self, const gchar *app_icon)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  g_free (self->app_icon);
  self->app_icon = g_strdup (app_icon);

  if (app_icon && g_strcmp0 (app_icon, "")) {
    gtk_image_set_from_icon_name (GTK_IMAGE (self->img_icon), app_icon, GTK_ICON_SIZE_DND);
  } else {
    gtk_widget_hide (self->img_icon);
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_APP_ICON]);
}

void
phosh_notification_set_summary (PhoshNotification *self, const gchar *summary)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  gtk_label_set_label (GTK_LABEL (self->lbl_summary), summary);

  if (summary && g_strcmp0 (summary, ""))
    gtk_widget_show (self->lbl_summary);
  else
    gtk_widget_hide (self->lbl_summary);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SUMMARY]);
}

void
phosh_notification_set_app_name (PhoshNotification *self, const gchar *app_name)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  gtk_label_set_label (GTK_LABEL (self->lbl_app_name), app_name);

  if (app_name && g_strcmp0 (app_name, ""))
    gtk_widget_show (self->lbl_app_name);
  else
    gtk_widget_hide (self->lbl_app_name);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_APP_NAME]);
}
