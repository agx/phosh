/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-notification-banner"

#include "phosh-config.h"
#include "animation.h"
#include "layersurface-priv.h"
#include "notification-banner.h"
#include "notification-frame.h"
#include "shell-priv.h"
#include "util.h"

#include <handy.h>

#define BANNER_MIN_WIDTH 360

/**
 * PhoshNotificationBanner:
 *
 * A floating notification
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
  gulong handler_expired;
  gulong handler_closed;

  gboolean slide_up;
  PhoshAnimation *animation;
};
typedef struct _PhoshNotificationBanner PhoshNotificationBanner;


G_DEFINE_TYPE (PhoshNotificationBanner, phosh_notification_banner, PHOSH_TYPE_LAYER_SURFACE)


static void
clear_handler (PhoshNotificationBanner *self)
{
  g_clear_signal_handler (&self->handler_expired, self->notification);
  g_clear_signal_handler (&self->handler_closed, self->notification);
}


static void
phosh_notification_banner_slide (double value, gpointer user_data)
{
  PhoshNotificationBanner *self = PHOSH_NOTIFICATION_BANNER (user_data);
  int margin, height;

  gtk_window_get_size (GTK_WINDOW (self), NULL, &height);
  margin = -(height * 0.9) * (self->slide_up ? value : (1.0 - value));

  phosh_layer_surface_set_margins (PHOSH_LAYER_SURFACE (self), margin, 0, 0, 0);

  phosh_layer_surface_wl_surface_commit (PHOSH_LAYER_SURFACE (self));
}


static void
phosh_notification_banner_slide_done (gpointer user_data)
{
  PhoshNotificationBanner *self = PHOSH_NOTIFICATION_BANNER (user_data);

  g_clear_pointer (&self->animation, phosh_animation_unref);
  if (self->slide_up)
    gtk_widget_destroy (GTK_WIDGET (self));
}


static void
expired (PhoshNotification       *notification,
         PhoshNotificationBanner *self)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION_BANNER (self));
  g_return_if_fail (PHOSH_IS_NOTIFICATION (notification));

  clear_handler (self);

  if (gtk_widget_get_mapped (GTK_WIDGET (self))) {
    self->slide_up = TRUE;
    self->animation = phosh_animation_new (GTK_WIDGET (self),
                                           0.0,
                                           1.0,
                                           250 * PHOSH_ANIMATION_SLOWDOWN,
                                           PHOSH_ANIMATION_TYPE_EASE_IN_QUINTIC,
                                           phosh_notification_banner_slide,
                                           phosh_notification_banner_slide_done,
                                           self);
    phosh_animation_start (self->animation);
  } else {
    gtk_widget_destroy (GTK_WIDGET (self));
  }
}


static void
closed (PhoshNotification       *notification,
        PhoshNotificationReason  reason,
        PhoshNotificationBanner *self)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION_BANNER (self));
  g_return_if_fail (PHOSH_IS_NOTIFICATION (notification));

  clear_handler (self);

  /* Close the banner */
  gtk_widget_destroy (GTK_WIDGET (self));
}


static void
phosh_notification_banner_set_notification (PhoshNotificationBanner *self,
                                            PhoshNotification       *notification)
{
  GtkWidget *content;

  g_set_object (&self->notification, notification);

  content = phosh_notification_frame_new (TRUE, NULL);
  phosh_notification_frame_bind_notification (PHOSH_NOTIFICATION_FRAME (content),
                                              self->notification);
  gtk_container_add (GTK_CONTAINER (self), content);

  self->handler_expired = g_signal_connect (self->notification, "expired",
                                            G_CALLBACK (expired), self);
  self->handler_closed = g_signal_connect (self->notification, "closed",
                                           G_CALLBACK (closed), self);
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


static void
phosh_notification_banner_finalize (GObject *object)
{
  PhoshNotificationBanner *self = PHOSH_NOTIFICATION_BANNER (object);

  clear_handler (self);

  g_clear_pointer (&self->animation, phosh_animation_unref);
  g_clear_object (&self->notification);

  G_OBJECT_CLASS (phosh_notification_banner_parent_class)->finalize (object);
}


static void
phosh_notification_banner_map (GtkWidget *widget)
{
  PhoshNotificationBanner *self = PHOSH_NOTIFICATION_BANNER (widget);
  int height;

  GTK_WIDGET_CLASS (phosh_notification_banner_parent_class)->map (widget);

  height = gtk_widget_get_allocated_height (widget);
  phosh_layer_surface_set_size (PHOSH_LAYER_SURFACE (widget), -1, height);

  phosh_animation_start (self->animation);
}


static void
phosh_notification_banner_class_init (PhoshNotificationBannerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_notification_banner_finalize;
  object_class->set_property = phosh_notification_banner_set_property;
  object_class->get_property = phosh_notification_banner_get_property;

  widget_class->map = phosh_notification_banner_map;

  /**
   * PhoshNotificationBanner:notification:
   *
   * The #PhoshNotification shown
   */
  props[PROP_NOTIFICATION] =
    g_param_spec_object ("notification", "", "",
                         PHOSH_TYPE_NOTIFICATION,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_css_name (widget_class, "phosh-notification-banner");
}


static void
phosh_notification_banner_init (PhoshNotificationBanner *self)
{
  self->animation = phosh_animation_new (GTK_WIDGET (self),
                                         0.0,
                                         1.0,
                                         250 * PHOSH_ANIMATION_SLOWDOWN,
                                         PHOSH_ANIMATION_TYPE_EASE_OUT_CUBIC,
                                         phosh_notification_banner_slide,
                                         phosh_notification_banner_slide_done,
                                         self);
}


GtkWidget *
phosh_notification_banner_new (PhoshNotification *notification)
{
  PhoshWayland *wl = phosh_wayland_get_default ();
  PhoshMonitor *monitor = phosh_shell_get_primary_monitor (phosh_shell_get_default ());
  int width = BANNER_MIN_WIDTH;

  phosh_shell_get_usable_area (phosh_shell_get_default (), NULL, NULL, &width, NULL);

  return g_object_new (PHOSH_TYPE_NOTIFICATION_BANNER,
                       "notification", notification,
                       "width-request", BANNER_MIN_WIDTH,
                       "valign", GTK_ALIGN_CENTER,
                       /* layer surface */
                       "layer-shell", phosh_wayland_get_zwlr_layer_shell_v1 (wl),
                       "wl-output", monitor ? monitor->wl_output : NULL,
                       "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
                       "width", MIN (width, 450),
                       /* Initial height to not take up the whole screen */
                       "height", 100,
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", 0,
                       "namespace", "phosh notification",
                       NULL);
}

/**
 * phosh_notification_banner_get_notification
 * @self: The banner
 *
 * Get the notification.
 *
 * Returns:(transfer none): The notification
 */
PhoshNotification *
phosh_notification_banner_get_notification (PhoshNotificationBanner *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION_BANNER (self), NULL);

  return self->notification;
}
