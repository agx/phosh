/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-notification-banner"

#include "config.h"
#include "notification-banner.h"
#include "notification-content.h"
#include "shell.h"

#define HANDY_USE_UNSTABLE_API
#include <handy.h>

/**
 * SECTION:phosh-notification-banner
 * @short_description: A floating notification
 * @Title: PhoshNotificationBanner
 */

enum {
  PROP_0,
  PROP_NOTIFICATION,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];


struct _PhoshNotificationBanner {
  PhoshLayerSurface parent;

  PhoshNotification *notification;

  GtkWidget *lbl_app_name;
  GtkWidget *img_icon;
  GtkWidget *list_notifs;

  struct {
    gdouble progress;
    gint64  last_frame;
  } animation;
};
typedef struct _PhoshNotificationBanner PhoshNotificationBanner;


G_DEFINE_TYPE (PhoshNotificationBanner, phosh_notification_banner, PHOSH_TYPE_LAYER_SURFACE)


static void
phosh_notification_banner_set_notification (PhoshNotificationBanner *self,
                                            PhoshNotification       *notification)
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
phosh_notification_banner_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  PhoshNotificationBanner *self = PHOSH_NOTIFICATION_BANNER (object);

  switch (property_id) {
    case PROP_NOTIFICATION:
      phosh_notification_banner_set_notification (self,
                                                  g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_notification_banner_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  PhoshNotificationBanner *self = PHOSH_NOTIFICATION_BANNER (object);

  switch (property_id) {
    case PROP_NOTIFICATION:
      g_value_set_object (value, self->notification);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static gboolean
header_activated (PhoshNotificationBanner *self, GdkEventButton *event)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION_BANNER (self), FALSE);

  phosh_notification_activate (self->notification,
                               PHOSH_NOTIFICATION_DEFAULT_ACTION);

  return FALSE;
}


static void
notification_activated (PhoshNotificationBanner *self,
                        GtkListBoxRow           *row,
                        GtkListBox              *list)
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
phosh_notification_banner_finalize (GObject *object)
{
  PhoshNotificationBanner *self = PHOSH_NOTIFICATION_BANNER (object);

  g_clear_object (&self->notification);

  G_OBJECT_CLASS (phosh_notification_banner_parent_class)->finalize (object);
}


static void
phosh_notification_banner_slide (PhoshNotificationBanner *self)
{
  gint margin;
  gint height;
  gdouble progress = hdy_ease_out_cubic (self->animation.progress);

  progress = 1.0 - progress;

  gtk_window_get_size (GTK_WINDOW (self), NULL, &height);
  margin = (height - 300) * progress;

  phosh_layer_surface_set_margins (PHOSH_LAYER_SURFACE (self), margin, 0, 0, 0);

  phosh_layer_surface_wl_surface_commit (PHOSH_LAYER_SURFACE (self));
}


static gboolean
animate_down_cb (GtkWidget     *widget,
                 GdkFrameClock *frame_clock,
                 gpointer       user_data)
{
  gint64 time;
  gboolean finished = FALSE;
  PhoshNotificationBanner *self = PHOSH_NOTIFICATION_BANNER (widget);

  time = gdk_frame_clock_get_frame_time (frame_clock) - self->animation.last_frame;
  if (self->animation.last_frame < 0) {
    time = 0;
  }

  self->animation.progress += 0.06666 * time / 16666.00;
  self->animation.last_frame = gdk_frame_clock_get_frame_time (frame_clock);

  if (self->animation.progress >= 1.0) {
    finished = TRUE;
    self->animation.progress = 1.0;
  }

  phosh_notification_banner_slide (self);

  return finished ? G_SOURCE_REMOVE : G_SOURCE_CONTINUE;
}


static void
phosh_notification_banner_show (GtkWidget *widget)
{
  PhoshNotificationBanner *self = PHOSH_NOTIFICATION_BANNER (widget);
  gboolean enable_animations;

  enable_animations = hdy_get_enable_animations (GTK_WIDGET (self));

  self->animation.last_frame = -1;
  self->animation.progress = enable_animations ? 0.0 : 1.0;
  gtk_widget_add_tick_callback (GTK_WIDGET (self), animate_down_cb, NULL, NULL);

  GTK_WIDGET_CLASS (phosh_notification_banner_parent_class)->show (widget);
}


static void
phosh_notification_banner_class_init (PhoshNotificationBannerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_notification_banner_finalize;
  object_class->set_property = phosh_notification_banner_set_property;
  object_class->get_property = phosh_notification_banner_get_property;

  widget_class->show = phosh_notification_banner_show;

  /**
   * PhoshNotificationBanner:notification:
   * @self: the #PhoshNotificationBanner
   *
   * The #PhoshNotification shown in @self
   */
  props[PROP_NOTIFICATION] =
    g_param_spec_object ("notification",
                         "Notification",
                         "Notification in the banner",
                         PHOSH_TYPE_NOTIFICATION,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/notification-banner.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationBanner, lbl_app_name);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationBanner, img_icon);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationBanner, list_notifs);

  gtk_widget_class_bind_template_callback (widget_class, header_activated);
  gtk_widget_class_bind_template_callback (widget_class, notification_activated);

  gtk_widget_class_set_css_name (widget_class, "phosh-notification-banner");
}


static void
phosh_notification_banner_init (PhoshNotificationBanner *self)
{
  self->animation.progress = 0.0;

  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_notification_banner_new (PhoshNotification *notification)
{
  PhoshWayland *wl = phosh_wayland_get_default ();
  int width = 360;
  phosh_shell_get_usable_area (phosh_shell_get_default (),
                               NULL, NULL, &width, NULL);

  return g_object_new (PHOSH_TYPE_NOTIFICATION_BANNER,
                       "notification", notification,
                       /* layer surface */
                       "margin-top", -300,
                       "layer-shell", phosh_wayland_get_zwlr_layer_shell_v1 (wl),
                       "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
                       "height", 50,
                       "width", MIN (width, 450),
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", 0,
                       "namespace", "phosh notification",
                       NULL);
}


PhoshNotification *
phosh_notification_banner_get_notification (PhoshNotificationBanner *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION_BANNER (self), NULL);

  return self->notification;
}
