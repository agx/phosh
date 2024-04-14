/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-notification-content"

#include "phosh-config.h"
#include "notification-content.h"

#include <gio/gdesktopappinfo.h>

/**
 * PhoshNotificationContent:
 *
 * Content of a notification
 */

enum {
  PROP_0,
  PROP_NOTIFICATION,
  PROP_SHOW_BODY,
  PROP_ACTION_FILTER_KEYS,
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

  gboolean   show_body;
  GStrv      action_filter_keys;
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

  visible = body != NULL && g_strcmp0 (body, "") && self->show_body;
  gtk_widget_set_visible (self->lbl_body, visible);

  g_value_set_string (to_value, body);

  return TRUE;
}


static GStrv
get_action_filter_keys (PhoshNotification *notification, const char * const *action_filter_keys)
{
  GAppInfo *info = phosh_notification_get_app_info (notification);
  g_autoptr (GStrvBuilder) filter_builder = NULL;

  if (action_filter_keys == NULL || action_filter_keys[0] == NULL)
    return NULL;

  if (info == NULL)
    return NULL;

  filter_builder = g_strv_builder_new ();
  for (int i = 0; i < g_strv_length ((GStrv)action_filter_keys); i++) {
    g_auto (GStrv) f = g_desktop_app_info_get_string_list (G_DESKTOP_APP_INFO (info),
                                                           action_filter_keys[i],
                                                           NULL);
    if (f)
      g_strv_builder_addv (filter_builder, (const char **)f);
  }

  return g_strv_builder_end (filter_builder);
}


static gboolean
action_matches_filter (const char *action, const char *const *filters)
{
  /* Empty filter cannot match */
  if (filters == NULL || filters[0] == NULL)
    return TRUE;

  for (int i = 0; i < g_strv_length ((GStrv)filters); i++) {
    if (g_str_has_prefix (action, filters[i]))
      return TRUE;
  }
  return FALSE;
}



static void
set_actions (PhoshNotificationContent *self,  PhoshNotification *notification)

{
  GStrv actions;
  g_auto (GStrv) filters = NULL;

  gtk_container_foreach (GTK_CONTAINER (self->box_actions),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  if (notification == NULL)
    return;

  actions = phosh_notification_get_actions (notification);
  if (actions == NULL) {
    return;
  }

  filters = get_action_filter_keys (notification, (const char *const *)self->action_filter_keys);

  for (int i = 0; actions[i] != NULL; i += 2) {
    GtkWidget *btn;
    GtkWidget *lbl;

    /* The default action is already triggered by the notification body */
    if (g_strcmp0 (actions[i], "default") == 0) {
      continue;
    }

    if (actions[i + 1] == NULL) {
      g_warning ("Expected action label at %i, got NULL", i);
      break;
    }

    /* Only Filter actions if a filter is set, otherwise it's "use all actions" */
    if (filters != NULL && filters[0] != NULL) {
      if (action_matches_filter (actions[i], (const char * const *)filters))
        g_object_set (self, "show-body", TRUE, NULL);
      else
        continue;
    }

    lbl = g_object_new (GTK_TYPE_LABEL,
                        "label", actions[i + 1],
                        "xalign", 0.0,
                        "halign", GTK_ALIGN_CENTER,
                        "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
                        "visible", TRUE,
                        NULL);

    btn = g_object_new (GTK_TYPE_BUTTON,
                        "action-name", "noti.activate",
                        "action-target", g_variant_new_string (actions[i]),
                        "expand", TRUE,
                        "visible", TRUE,
                        NULL);
    gtk_container_add (GTK_CONTAINER (btn), lbl);

    gtk_container_add (GTK_CONTAINER (self->box_actions), btn);
  }
}


static void
on_actions_changed (PhoshNotification        *notification,
                    GParamSpec               *pspec,
                    PhoshNotificationContent *self)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION_CONTENT (self));

  set_actions (self, notification);
}



static void
phosh_notification_content_set_notification (PhoshNotificationContent *self,
                                             PhoshNotification        *notification)
{
  g_set_object (&self->notification, notification);

  /* Use the "transform" function to show/hide when set/unset */
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

  g_signal_connect_object (self->notification, "notify::actions",
                           G_CALLBACK (on_actions_changed), self, 0);
  set_actions (self, self->notification);

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
    case PROP_SHOW_BODY:
      self->show_body = g_value_get_boolean (value);
      break;
    case PROP_ACTION_FILTER_KEYS:
      self->action_filter_keys = g_value_dup_boxed (value);
      set_actions (self, self->notification);
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
phosh_notification_content_finalize (GObject *object)
{
  PhoshNotificationContent *self = PHOSH_NOTIFICATION_CONTENT (object);

  g_clear_object (&self->notification);
  g_clear_pointer (&self->action_filter_keys, g_strfreev);

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

  /**
   * PhoshNotificationContent:show-body:
   *
   * Whether the body of the notification is shown
   */
  props[PROP_SHOW_BODY] =
    g_param_spec_boolean ("show-body",
                          "",
                          "",
                          TRUE,
                          G_PARAM_CONSTRUCT | G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS);

  /*
   * PhoshNotificationContent::action-filter-keys
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
  { .name = "activate", .activate = action_activate, .parameter_type = "s" },
};


static void
phosh_notification_content_init (PhoshNotificationContent *self)
{
  g_autoptr (GActionMap) map = NULL;

  self->show_body = TRUE;

  map = G_ACTION_MAP (g_simple_action_group_new ());
  g_action_map_add_action_entries (map,
                                   entries,
                                   G_N_ELEMENTS (entries),
                                   self);
  gtk_widget_insert_action_group (GTK_WIDGET (self),
                                  "noti",
                                  G_ACTION_GROUP (map));

  gtk_widget_init_template (GTK_WIDGET (self));


  g_object_bind_property (self,
                          "show-body",
                          self->lbl_body,
                          "visible",
                          G_BINDING_DEFAULT);

  g_object_bind_property (self,
                          "show-body",
                          self->box_actions,
                          "visible",
                          G_BINDING_DEFAULT);
}


GtkWidget *
phosh_notification_content_new (PhoshNotification  *notification,
                                gboolean            show_body,
                                const char * const *action_filter_keys)
{
  return g_object_new (PHOSH_TYPE_NOTIFICATION_CONTENT,
                       "action-filter-keys", action_filter_keys,
                       "notification", notification,
                       "show-body", show_body,
                       NULL);
}

/**
 * phosh_notification_content_get_notification
 * @self: The notification content
 *
 * Get the notification.
 *
 * Returns:(transfer none): The notification
 */
PhoshNotification *
phosh_notification_content_get_notification (PhoshNotificationContent *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION_CONTENT (self), NULL);

  return self->notification;
}
