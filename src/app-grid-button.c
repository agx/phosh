/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "phosh-app-grid-button"

#include "config.h"
#include "app-grid-button.h"
#include "phosh-enums.h"
#include "favorite-list-model.h"

#include "toplevel-manager.h"
#include "shell.h"
#include "util.h"

typedef struct _PhoshAppGridButtonPrivate PhoshAppGridButtonPrivate;
struct _PhoshAppGridButtonPrivate {
  GAppInfo *info;
  gboolean is_favorite;
  PhoshAppGridButtonMode mode;

  gulong favorite_changed_watcher;

  GtkWidget  *icon;
  GtkWidget  *label;
  GtkWidget  *popover;
  GtkGesture *gesture;

  GMenu *menu;
  GMenu *actions;

  GActionMap *action_map;
};

G_DEFINE_TYPE_WITH_PRIVATE (PhoshAppGridButton, phosh_app_grid_button, GTK_TYPE_FLOW_BOX_CHILD)

enum {
  PROP_0,
  PROP_APP_INFO,
  PROP_IS_FAVORITE,
  PROP_MODE,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];

enum {
  APP_LAUNCHED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];

static void
phosh_app_grid_button_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PhoshAppGridButton *self = PHOSH_APP_GRID_BUTTON (object);

  switch (property_id) {
    case PROP_APP_INFO:
      phosh_app_grid_button_set_app_info (self, g_value_get_object (value));
      break;
    case PROP_MODE:
      phosh_app_grid_button_set_mode (self, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_app_grid_button_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  PhoshAppGridButton *self = PHOSH_APP_GRID_BUTTON (object);

  switch (property_id) {
    case PROP_APP_INFO:
      g_value_set_object (value, phosh_app_grid_button_get_app_info (self));
      break;
    case PROP_IS_FAVORITE:
      g_value_set_boolean (value, phosh_app_grid_button_is_favorite (self));
      break;
    case PROP_MODE:
      g_value_set_enum (value, phosh_app_grid_button_get_mode (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_app_grid_button_dispose (GObject *object)
{
  PhoshAppGridButton *self = PHOSH_APP_GRID_BUTTON (object);
  PhoshAppGridButtonPrivate *priv = phosh_app_grid_button_get_instance_private (self);

  g_clear_object (&priv->gesture);

  G_OBJECT_CLASS (phosh_app_grid_button_parent_class)->dispose (object);
}


static void
phosh_app_grid_button_finalize (GObject *object)
{
  PhoshAppGridButton *self = PHOSH_APP_GRID_BUTTON (object);
  PhoshAppGridButtonPrivate *priv = phosh_app_grid_button_get_instance_private (self);

  g_clear_object (&priv->info);
  g_clear_object (&priv->menu);
  g_clear_object (&priv->actions);
  g_clear_object (&priv->action_map);

  phosh_clear_handler (&priv->favorite_changed_watcher,
                       phosh_favorite_list_model_get_default ());

  G_OBJECT_CLASS (phosh_app_grid_button_parent_class)->finalize (object);
}


static void
context_menu (GtkWidget *widget,
              GdkEvent  *event)
{
  PhoshAppGridButton *self = PHOSH_APP_GRID_BUTTON (widget);
  PhoshAppGridButtonPrivate *priv = phosh_app_grid_button_get_instance_private (self);

  gtk_popover_popup (GTK_POPOVER (priv->popover));
}


static gboolean
phosh_app_grid_button_popup_menu (GtkWidget *self)
{
  context_menu (self, NULL);
  return TRUE;
}


static gboolean
phosh_app_grid_button_button_press_event (GtkWidget      *self,
                                          GdkEventButton *event)
{
  if (gdk_event_triggers_context_menu ((GdkEvent *) event)) {
    context_menu (self, (GdkEvent *) event);

    return TRUE;
  }

  return GTK_WIDGET_CLASS (phosh_app_grid_button_parent_class)->button_press_event (self, event);
}


static void
activate_cb (PhoshAppGridButton *self)
{
  PhoshAppGridButtonPrivate *priv = phosh_app_grid_button_get_instance_private (self);
  g_autoptr (GdkAppLaunchContext) context = NULL;
  g_autoptr (GError) error = NULL;
  PhoshToplevelManager *toplevel_manager = phosh_shell_get_toplevel_manager (phosh_shell_get_default ());
  g_autofree char *app_id = g_strdup (g_app_info_get_id (G_APP_INFO (priv->info)));

  g_debug ("Launching %s", app_id);

  // strip ".desktop" suffix
  if (app_id && g_str_has_suffix (app_id, ".desktop")) {
    *(app_id + strlen (app_id) - strlen (".desktop")) = '\0';
  }

  for (guint i=0; i < phosh_toplevel_manager_get_num_toplevels (toplevel_manager); i++) {
    PhoshToplevel *toplevel = phosh_toplevel_manager_get_toplevel (toplevel_manager, i);
    const char *window_id = phosh_toplevel_get_app_id (toplevel);
    g_autofree char *fixed_id = phosh_fix_app_id (window_id);

    if (g_strcmp0 (app_id, window_id) == 0 || g_strcmp0 (app_id, fixed_id) == 0) {
      // activate the first matching window for now, since we don't have toplevels sorted by last-focus yet
      phosh_toplevel_activate (toplevel, phosh_wayland_get_wl_seat (phosh_wayland_get_default ()));
      g_signal_emit (self, signals[APP_LAUNCHED], 0, priv->info);
      return;
    }
  }

  context = gdk_display_get_app_launch_context (gtk_widget_get_display (GTK_WIDGET (self)));

  g_app_info_launch (G_APP_INFO (priv->info), NULL,
                     G_APP_LAUNCH_CONTEXT (context), &error);

  if (error) {
    g_critical ("Failed to launch app %s: %s",
                g_app_info_get_id (priv->info),
                error->message);

    return;
  }

  g_signal_emit (self, signals[APP_LAUNCHED], 0, priv->info);
}


static void
phosh_app_grid_button_class_init (PhoshAppGridButtonClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = phosh_app_grid_button_set_property;
  object_class->get_property = phosh_app_grid_button_get_property;
  object_class->dispose = phosh_app_grid_button_dispose;
  object_class->finalize = phosh_app_grid_button_finalize;

  widget_class->popup_menu = phosh_app_grid_button_popup_menu;
  widget_class->button_press_event = phosh_app_grid_button_button_press_event;

  props[PROP_APP_INFO] =
    g_param_spec_object ("app-info", "App", "App Info",
                         G_TYPE_APP_INFO,
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_READWRITE |
                         G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PhoshAppGridButton:is-favorite:
   *
   * %TRUE when the application is currently favorited
   *
   * Stability: Private
   */
  props[PROP_IS_FAVORITE] =
    g_param_spec_boolean ("is-favorite", "Favorite", "Is a favorite app",
                          FALSE,
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_READABLE |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PhoshAppGridButton:mode:
   *
   * The #PhoshAppGridButtonMode of the button
   *
   * In %PHOSH_APP_GRID_BUTTON_FAVORITES the label is
   * hidden
   *
   * Stability: Private
   */
  props[PROP_MODE] =
    g_param_spec_enum ("mode", "Mode", "Button mode",
                       PHOSH_TYPE_APP_GRID_BUTTON_MODE,
                       PHOSH_APP_GRID_BUTTON_LAUNCHER,
                       G_PARAM_STATIC_STRINGS |
                       G_PARAM_READWRITE |
                       G_PARAM_EXPLICIT_NOTIFY);


  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/phosh/ui/app-grid-button.ui");

  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGridButton, icon);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGridButton, label);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGridButton, popover);

  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGridButton, menu);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGridButton, actions);

  gtk_widget_class_bind_template_callback (widget_class, activate_cb);

  signals[APP_LAUNCHED] = g_signal_new ("app-launched",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        0, NULL, NULL, NULL,
                                        G_TYPE_NONE, 1, G_TYPE_APP_INFO);

  gtk_widget_class_set_css_name (widget_class, "phosh-app-grid-button");
}


static void
action_activated (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       data)
{
  PhoshAppGridButton *self = PHOSH_APP_GRID_BUTTON (data);
  PhoshAppGridButtonPrivate *priv = phosh_app_grid_button_get_instance_private (self);
  g_autoptr (GdkAppLaunchContext) context = NULL;
  g_autoptr (GError) error = NULL;
  const char *action_name;

  action_name = g_variant_get_string (parameter, NULL);

  g_debug ("Launching %s->%s", g_app_info_get_id (priv->info), action_name);

  g_return_if_fail (action_name != NULL);
  g_return_if_fail (G_IS_DESKTOP_APP_INFO (priv->info));

  context = gdk_display_get_app_launch_context (gtk_widget_get_display (GTK_WIDGET (self)));

  g_desktop_app_info_launch_action (G_DESKTOP_APP_INFO (priv->info),
                                    action_name,
                                    G_APP_LAUNCH_CONTEXT (context));

  g_signal_emit (self, signals[APP_LAUNCHED], 0, priv->info);
}


static void
favorite_remove_activated (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       data)
{
  PhoshAppGridButton *self = PHOSH_APP_GRID_BUTTON (data);
  PhoshAppGridButtonPrivate *priv = phosh_app_grid_button_get_instance_private (self);

  phosh_favorite_list_model_remove_app (NULL, priv->info);
}


static void
favorite_add_activated (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       data)
{
  PhoshAppGridButton *self = PHOSH_APP_GRID_BUTTON (data);
  PhoshAppGridButtonPrivate *priv = phosh_app_grid_button_get_instance_private (self);

  phosh_favorite_list_model_add_app (NULL, priv->info);
}


static void
long_pressed (GtkGestureLongPress *gesture,
              double               x,
              double               y,
              GtkWidget           *self)
{
  context_menu (self, NULL);
}


static GActionEntry entries[] =
{
  { "action", action_activated, "s", NULL, NULL },
  { "favorite-remove", favorite_remove_activated, NULL, NULL, NULL },
  { "favorite-add", favorite_add_activated, NULL, NULL, NULL },
};


static void
phosh_app_grid_button_init (PhoshAppGridButton *self)
{
  PhoshAppGridButtonPrivate *priv = phosh_app_grid_button_get_instance_private (self);
  GAction *act;

  priv->is_favorite = FALSE;
  priv->mode = PHOSH_APP_GRID_BUTTON_LAUNCHER;
  priv->favorite_changed_watcher = 0;

  priv->action_map = G_ACTION_MAP (g_simple_action_group_new ());
  g_action_map_add_action_entries (priv->action_map,
                                   entries,
                                   G_N_ELEMENTS (entries),
                                   self);
  gtk_widget_insert_action_group (GTK_WIDGET (self),
                                  "app-btn",
                                  G_ACTION_GROUP (priv->action_map));

  act = g_action_map_lookup_action (priv->action_map, "favorite-add");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (act), TRUE);
  act = g_action_map_lookup_action (priv->action_map, "favorite-remove");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (act), FALSE);

  gtk_widget_init_template (GTK_WIDGET (self));

  priv->gesture = gtk_gesture_long_press_new (GTK_WIDGET (self));
  gtk_gesture_single_set_touch_only (GTK_GESTURE_SINGLE (priv->gesture), TRUE);
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (priv->gesture),
                                              GTK_PHASE_CAPTURE);
  g_signal_connect (priv->gesture, "pressed", G_CALLBACK (long_pressed), self);

  gtk_popover_bind_model (GTK_POPOVER (priv->popover),
                          G_MENU_MODEL (priv->menu),
                          "app-btn");
}


GtkWidget *
phosh_app_grid_button_new (GAppInfo *info)
{
  return g_object_new (PHOSH_TYPE_APP_GRID_BUTTON,
                       "app-info", info,
                       NULL);
}


GtkWidget *
phosh_app_grid_button_new_favorite (GAppInfo *info)
{
  return g_object_new (PHOSH_TYPE_APP_GRID_BUTTON,
                       "app-info", info,
                       "mode", PHOSH_APP_GRID_BUTTON_FAVORITES,
                       NULL);
}


static void
favorites_changed (GListModel         *list,
                   guint               position,
                   guint               removed,
                   guint               added,
                   PhoshAppGridButton *self)
{
  PhoshAppGridButtonPrivate *priv;
  gboolean favorite = FALSE;
  GAction *act;

  g_return_if_fail (PHOSH_IS_APP_GRID_BUTTON (self));
  g_return_if_fail (PHOSH_IS_FAVORITE_LIST_MODEL (list));

  priv = phosh_app_grid_button_get_instance_private (self);

  favorite = phosh_favorite_list_model_app_is_favorite (PHOSH_FAVORITE_LIST_MODEL (list),
                                                           priv->info);

  if (priv->is_favorite == favorite) {
    return;
  }

  act = g_action_map_lookup_action (priv->action_map, "favorite-add");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (act), !favorite);
  act = g_action_map_lookup_action (priv->action_map, "favorite-remove");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (act), favorite);

  priv->is_favorite = favorite;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_IS_FAVORITE]);
}


void
phosh_app_grid_button_set_app_info (PhoshAppGridButton *self,
                                    GAppInfo *info)
{
  PhoshFavoriteListModel *list = NULL;
  PhoshAppGridButtonPrivate *priv;
  GIcon *icon;
  const char *name;

  g_return_if_fail (PHOSH_IS_APP_GRID_BUTTON (self));
  g_return_if_fail (G_IS_APP_INFO (info) || info == NULL);

  priv = phosh_app_grid_button_get_instance_private (self);

  if (priv->info == info)
      return;

  g_clear_object (&priv->info);

  g_menu_remove_all (priv->actions);

  list = phosh_favorite_list_model_get_default ();

  phosh_clear_handler (&priv->favorite_changed_watcher, list);

  if (info) {
    priv->info = g_object_ref (info);

    priv->favorite_changed_watcher = g_signal_connect (list,
                                                        "items-changed",
                                                        G_CALLBACK (favorites_changed),
                                                        self);
    favorites_changed (G_LIST_MODEL (list), 0, 0, 0, self);

    name = g_app_info_get_name (G_APP_INFO (priv->info));
    gtk_label_set_label (GTK_LABEL (priv->label), name);

    icon = g_app_info_get_icon (priv->info);
    if (G_UNLIKELY (icon == NULL)) {
      gtk_image_set_from_icon_name (GTK_IMAGE (priv->icon),
                                    PHOSH_APP_UNKNOWN_ICON,
                                    GTK_ICON_SIZE_DIALOG);
    } else {
      if (G_IS_THEMED_ICON (icon)) {
        g_themed_icon_append_name (G_THEMED_ICON (icon),
                                   PHOSH_APP_UNKNOWN_ICON);
      }
      gtk_image_set_from_gicon (GTK_IMAGE (priv->icon),
                                icon,
                                GTK_ICON_SIZE_DIALOG);
    }

    gtk_widget_set_sensitive (GTK_WIDGET (self), TRUE);

    if (G_IS_DESKTOP_APP_INFO (priv->info)) {
      const char *const *actions = NULL;
      int i = 0;

      actions = g_desktop_app_info_list_actions (G_DESKTOP_APP_INFO (priv->info));

      /*
       * So the dummy GAppInfo for the tests is (for reasons known only to gio)
       * actually a GDesktopAppInfo rather than something like GDummyAppInfo,
       * this means that guarding this block with G_IS_DESKTOP_APP_INFO
       * doesn't actually help much. This seems to surprise even gio as instead
       * of always returning at least an empty array (as the API promises) it
       * returns NULL
       *
       * tl;dr: we do (actions && actions[i]) instead of (actions[i]) otherwise
       *        the tests explode because of a condition that can only exist in
       *        the tests
       */

      while (actions && actions[i]) {
        g_autofree char *detailed_action = NULL;
        g_autofree char *label = NULL;

        detailed_action = g_strdup_printf ("action::%s", actions[i]);

        label = g_desktop_app_info_get_action_name (G_DESKTOP_APP_INFO (priv->info),
                                                    actions[i]);

        g_menu_append (priv->actions, label, detailed_action);

        i++;
      }
    }
  } else {
    gtk_label_set_label (GTK_LABEL (priv->label), _("Application"));
    gtk_image_set_from_icon_name (GTK_IMAGE (priv->icon),
                                  PHOSH_APP_UNKNOWN_ICON,
                                  GTK_ICON_SIZE_DIALOG);

    gtk_widget_set_sensitive (GTK_WIDGET (self), FALSE);
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_APP_INFO]);
}


GAppInfo *
phosh_app_grid_button_get_app_info (PhoshAppGridButton *self)
{
  PhoshAppGridButtonPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_APP_GRID_BUTTON (self), NULL);
  priv = phosh_app_grid_button_get_instance_private (self);

  return priv->info;
}


gboolean
phosh_app_grid_button_is_favorite (PhoshAppGridButton *self)
{
  PhoshAppGridButtonPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_APP_GRID_BUTTON (self), FALSE);
  priv = phosh_app_grid_button_get_instance_private (self);

  return priv->is_favorite;
}


void
phosh_app_grid_button_set_mode (PhoshAppGridButton     *self,
                                PhoshAppGridButtonMode  mode)
{
  PhoshAppGridButtonPrivate *priv;

  g_return_if_fail (PHOSH_IS_APP_GRID_BUTTON (self));
  priv = phosh_app_grid_button_get_instance_private (self);

  if (priv->mode == mode) {
    return;
  }

  switch (mode) {
    case PHOSH_APP_GRID_BUTTON_LAUNCHER:
      gtk_widget_set_visible (priv->label, TRUE);
      break;
    case PHOSH_APP_GRID_BUTTON_FAVORITES:
      gtk_widget_set_visible (priv->label, FALSE);
      break;
    default:
      g_critical ("Invalid mode %i", mode);
      return;
  }

  priv->mode = mode;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_MODE]);
}


PhoshAppGridButtonMode
phosh_app_grid_button_get_mode (PhoshAppGridButton *self)
{
  PhoshAppGridButtonPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_APP_GRID_BUTTON (self), FALSE);
  priv = phosh_app_grid_button_get_instance_private (self);

  return priv->mode;
}
