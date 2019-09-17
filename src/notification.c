/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-notification"

#include "config.h"
#include "notification.h"
#include "shell.h"

#define HANDY_USE_UNSTABLE_API
#include <handy.h>

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
  PROP_APP_INFO,
  PROP_IMAGE,
  PROP_ACTIONS,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];

enum {
  SIGNAL_ACTIONED,
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
  GtkWidget *img_image;
  GtkWidget *box_actions;

  GIcon     *icon;
  GIcon     *image;
  GAppInfo  *info;
  GStrv      actions;

  struct {
    gdouble progress;
    gint64  last_frame;
  } animation;
} PhoshNotification;

G_DEFINE_TYPE (PhoshNotification, phosh_notification, PHOSH_TYPE_LAYER_SURFACE)


static void
phosh_notification_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
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
    phosh_notification_set_body (self, g_value_get_string (value));
    break;
  case PROP_APP_ICON:
    phosh_notification_set_app_icon (self, g_value_get_object (value));
    break;
  case PROP_APP_INFO:
    phosh_notification_set_app_info (self, g_value_get_object (value));
    break;
  case PROP_IMAGE:
    phosh_notification_set_image (self, g_value_get_object (value));
    break;
  case PROP_ACTIONS:
    phosh_notification_set_actions (self, g_value_get_boxed (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_notification_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
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
    g_value_set_object (value, self->icon);
    break;
  case PROP_APP_INFO:
    g_value_set_object (value, self->info);
    break;
  case PROP_IMAGE:
    g_value_set_object (value, self->image);
    break;
  case PROP_ACTIONS:
    g_value_set_boxed (value, self->actions);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static gboolean
activate_notification (PhoshNotification *self, GdkEventButton *event)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), FALSE);

  g_signal_emit (self, signals[SIGNAL_ACTIONED], 0, "default");

  return FALSE;
}


static void
phosh_notification_finalize (GObject *object)
{
  PhoshNotification *self = PHOSH_NOTIFICATION (object);

  g_clear_object (&self->icon);
  g_clear_object (&self->image);
  g_clear_object (&self->info);
  g_clear_pointer (&self->actions, g_strfreev);

  G_OBJECT_CLASS (phosh_notification_parent_class)->finalize (object);
}


static void
phosh_notification_slide (PhoshNotification *self)
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
  PhoshNotification *self = PHOSH_NOTIFICATION (widget);

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

  phosh_notification_slide (self);

  return finished ? G_SOURCE_REMOVE : G_SOURCE_CONTINUE;
}


static void
phosh_notification_show (GtkWidget *widget)
{
  PhoshNotification *self = PHOSH_NOTIFICATION (widget);
  gboolean enable_animations;

  enable_animations = hdy_get_enable_animations (GTK_WIDGET (self));

  self->animation.last_frame = -1;
  self->animation.progress = enable_animations ? 0.0 : 1.0;
  gtk_widget_add_tick_callback (GTK_WIDGET (self), animate_down_cb, NULL, NULL);

  GTK_WIDGET_CLASS (phosh_notification_parent_class)->show (widget);
}


static void
phosh_notification_class_init (PhoshNotificationClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_notification_finalize;
  object_class->set_property = phosh_notification_set_property;
  object_class->get_property = phosh_notification_get_property;

  widget_class->show = phosh_notification_show;

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
    g_param_spec_object (
      "app-icon",
      "App Icon",
      "Application icon",
      G_TYPE_ICON,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PhoshNotification:app-info:
   * 
   * When not %NULL this overrides the values of #PhoshNotification:app-name
   * and #PhoshNotification:app-icon with those of the #GAppInfo
   */
  props[PROP_APP_INFO] =
    g_param_spec_object (
      "app-info",
      "App Info",
      "Application info",
      G_TYPE_APP_INFO,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_IMAGE] =
    g_param_spec_object (
      "image",
      "Image",
      "Notification image",
      G_TYPE_ICON,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_ACTIONS] =
    g_param_spec_boxed (
      "actions",
      "Actions",
      "Notification actions",
      G_TYPE_STRV,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  /**
   * PhoshNotifiation::actioned:
   *
   * When the user activates one of the provided actions (inc default)
   */
  signals[SIGNAL_ACTIONED] = g_signal_new ("actioned",
                                           G_TYPE_FROM_CLASS (klass),
                                           G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                           NULL,
                                           G_TYPE_NONE,
                                           1,
                                           G_TYPE_STRING);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/notification.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshNotification, lbl_app_name);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotification, lbl_summary);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotification, lbl_body);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotification, img_icon);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotification, img_image);
  gtk_widget_class_bind_template_child (widget_class, PhoshNotification, box_actions);

  gtk_widget_class_bind_template_callback (widget_class, activate_notification);

  gtk_widget_class_set_css_name (widget_class, "phosh-notification");
}

static void
action_activate (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       data)
{
  const char *target;

  g_return_if_fail (PHOSH_IS_NOTIFICATION (data));

  target = g_variant_get_string (parameter, NULL);

  g_signal_emit (data, signals[SIGNAL_ACTIONED], 0, target);
}

static GActionEntry entries[] =
{
  { "activate", action_activate, "s", NULL, NULL },
};

static void
phosh_notification_init (PhoshNotification *self)
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


PhoshNotification *
phosh_notification_new (const char *app_name,
                        GAppInfo   *info,
                        const char *summary,
                        const char *body,
                        GIcon      *icon,
                        GIcon      *image,
                        GStrv       actions)
{
  PhoshWayland *wl = phosh_wayland_get_default ();
  int width = 360;
  phosh_shell_get_usable_area (phosh_shell_get_default (),
                               NULL, NULL, &width, NULL);

  return g_object_new (PHOSH_TYPE_NOTIFICATION,
                       "app_name", app_name,
                       "summary", summary,
                       "body", body,
                       "app-icon", icon,
                       "app-info", info,
                       "image", image,
                       "actions", actions,
                       /* layer surface */
                       "margin-top", -300,
                       "layer-shell", phosh_wayland_get_zwlr_layer_shell_v1(wl),
                       "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
                       "height", 50,
                       "width", MIN (width, 450),
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", 0,
                       "namespace", "phosh notification",
                       NULL);
}

void
phosh_notification_set_app_icon (PhoshNotification *self, GIcon *icon)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  g_clear_object (&self->icon);
  if (icon != NULL) {
    self->icon = g_object_ref (icon);

    gtk_image_set_from_gicon (GTK_IMAGE (self->img_icon),
                              icon,
                              GTK_ICON_SIZE_SMALL_TOOLBAR);
  } else {
    gtk_image_set_from_icon_name (GTK_IMAGE (self->img_icon),
                                  "application-x-executable",
                                  GTK_ICON_SIZE_SMALL_TOOLBAR);
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_APP_ICON]);
}

void
phosh_notification_set_app_info (PhoshNotification *self,
                                 GAppInfo          *info)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  g_clear_object (&self->info);

  if (info != NULL) {
    GIcon *icon;
    const char *name;

    self->info = g_object_ref (info);

    icon = g_app_info_get_icon (info);
    name = g_app_info_get_display_name (info);

    phosh_notification_set_app_icon (self, icon);
    phosh_notification_set_app_name (self, name);
  }
}

void
phosh_notification_set_image (PhoshNotification *self, GIcon *image)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  g_clear_object (&self->image);

  if (image != NULL) {
    self->image = g_object_ref (image);

    gtk_image_set_from_gicon (GTK_IMAGE (self->img_image),
                              image,
                              GTK_ICON_SIZE_DND);
    gtk_widget_show (self->img_image);
  } else {
    gtk_widget_hide (self->img_image);
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_IMAGE]);
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
phosh_notification_set_body (PhoshNotification *self, const gchar *body)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  gtk_label_set_markup (GTK_LABEL (self->lbl_body), body);

  if (body && g_strcmp0 (body, ""))
    gtk_widget_show (self->lbl_body);
  else
    gtk_widget_hide (self->lbl_body);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_BODY]);
}

void
phosh_notification_set_app_name (PhoshNotification *self, const gchar *app_name)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  if (app_name &&
      strlen (app_name) > 0 &&
      g_strcmp0 (app_name, "notify-send") != 0) {
    gtk_label_set_label (GTK_LABEL (self->lbl_app_name), app_name);
  } else {
    gtk_label_set_label (GTK_LABEL (self->lbl_app_name), _("Notification"));
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_APP_NAME]);
}

void
phosh_notification_set_actions (PhoshNotification *self,
                                GStrv              actions)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  g_clear_pointer (&self->actions, g_strfreev);

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

  self->actions = g_boxed_copy (G_TYPE_STRV, actions);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ACTIONS]);
}
