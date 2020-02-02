/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-notification-banner"

#include "config.h"
#include "notification-banner.h"
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
  GtkWidget *lbl_summary;
  GtkWidget *lbl_body;
  GtkWidget *img_icon;
  GtkWidget *img_image;
  GtkWidget *box_actions;

  struct {
    gdouble progress;
    gint64  last_frame;
  } animation;
};
typedef struct _PhoshNotificationBanner PhoshNotificationBanner;


G_DEFINE_TYPE (PhoshNotificationBanner, phosh_notification_banner, PHOSH_TYPE_LAYER_SURFACE)


static gboolean
set_image (GBinding     *binding,
           const GValue *from_value,
           GValue       *to_value,
           gpointer      user_data)
{
  PhoshNotificationBanner *self = user_data;
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
  PhoshNotificationBanner *self = user_data;
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
  PhoshNotificationBanner *self = user_data;
  const char* body = g_value_get_string (from_value);
  gboolean visible;

  visible = (body != NULL && g_strcmp0 (body, ""));
  gtk_widget_set_visible (self->lbl_body, visible);

  g_value_set_string (to_value, body);

  return TRUE;
}


static void
set_actions (PhoshNotification       *notification,
             GParamSpec              *pspec,
             PhoshNotificationBanner *self)
{
  GStrv actions = phosh_notification_get_actions (notification);

  g_return_if_fail (PHOSH_IS_NOTIFICATION_BANNER (self));

  gtk_container_foreach (GTK_CONTAINER (self->box_actions),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

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
phosh_notification_banner_set_notification (PhoshNotificationBanner *self,
                                            PhoshNotification       *notification)
{
  g_set_object (&self->notification, notification);

  g_object_bind_property (self->notification, "app-name",
                          self->lbl_app_name, "label",
                          G_BINDING_SYNC_CREATE);

  // If unset PhoshNotification provides a fallback
  g_object_bind_property (self->notification, "app-icon",
                          self->img_icon,     "gicon",
                          G_BINDING_SYNC_CREATE);

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
activate_notification (PhoshNotificationBanner *self, GdkEventButton *event)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION_BANNER (self), FALSE);

  phosh_notification_activate (self->notification,
                               PHOSH_NOTIFICATION_DEFAULT_ACTION);

  return FALSE;
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
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationBanner, lbl_summary);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationBanner, lbl_body);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationBanner, img_icon);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationBanner, img_image);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotificationBanner, box_actions);

  gtk_widget_class_bind_template_callback (widget_class, activate_notification);

  gtk_widget_class_set_css_name (widget_class, "phosh-notification");
}


static void
action_activate (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       data)
{
  PhoshNotificationBanner *self = data;
  const char *target;

  g_return_if_fail (PHOSH_IS_NOTIFICATION_BANNER (self));

  target = g_variant_get_string (parameter, NULL);

  phosh_notification_activate (self->notification, target);
}


static GActionEntry entries[] = {
  { "activate", action_activate, "s", NULL, NULL },
};


static void
phosh_notification_banner_init (PhoshNotificationBanner *self)
{
  g_autoptr (GActionMap) map = NULL;

  self->animation.progress = 0.0;

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


PhoshNotificationBanner *
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
