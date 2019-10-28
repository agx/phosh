/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#define G_LOG_DOMAIN "phosh-app-grid-button"

#include "config.h"
#include "app-grid-button.h"

#include "toplevel-manager.h"
#include "shell.h"
#include "util.h"

typedef struct _PhoshAppGridButtonPrivate PhoshAppGridButtonPrivate;
struct _PhoshAppGridButtonPrivate {
  GAppInfo *info;

  GtkWidget *icon;
  GtkWidget *label;
};

G_DEFINE_TYPE_WITH_PRIVATE (PhoshAppGridButton, phosh_app_grid_button, GTK_TYPE_FLOW_BOX_CHILD)

enum {
  PROP_0,
  PROP_APP_INFO,
  PROP_IS_FAVORITE,
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
  PhoshAppGridButtonPrivate *priv = phosh_app_grid_button_get_instance_private (self);

  switch (property_id) {
    case PROP_APP_INFO:
      phosh_app_grid_button_set_app_info (self, g_value_get_object (value));
      break;
    case PROP_IS_FAVORITE:
      gtk_widget_set_visible (priv->label, !g_value_get_boolean (value));
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
  PhoshAppGridButtonPrivate *priv = phosh_app_grid_button_get_instance_private (self);

  switch (property_id) {
    case PROP_APP_INFO:
      g_value_set_object (value, phosh_app_grid_button_get_app_info (self));
      break;
    case PROP_IS_FAVORITE:
      g_value_set_boolean (value, !gtk_widget_get_visible (priv->label));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
phosh_app_grid_button_finalize (GObject *object)
{
  PhoshAppGridButton *self = PHOSH_APP_GRID_BUTTON (object);
  PhoshAppGridButtonPrivate *priv = phosh_app_grid_button_get_instance_private (self);

  g_clear_object (&priv->info);

  G_OBJECT_CLASS (phosh_app_grid_button_parent_class)->finalize (object);
}

static void
activate_cb (PhoshAppGridButton *self)
{
  PhoshAppGridButtonPrivate *priv = phosh_app_grid_button_get_instance_private (self);
  g_autoptr (GdkAppLaunchContext) context = NULL;
  g_autoptr (GError) error = NULL;
  PhoshToplevelManager *toplevel_manager = phosh_shell_get_toplevel_manager (phosh_shell_get_default ());
  g_autofree gchar *app_id = g_strdup (g_app_info_get_id (G_APP_INFO (priv->info)));

  g_debug ("Launching %s", app_id);

  // strip ".desktop" suffix
  if (app_id && g_str_has_suffix (app_id, ".desktop")) {
    *(app_id + strlen (app_id) - strlen (".desktop")) = '\0';
  }

  for (guint i=0; i < phosh_toplevel_manager_get_num_toplevels (toplevel_manager); i++) {
    PhoshToplevel *toplevel = phosh_toplevel_manager_get_toplevel (toplevel_manager, i);
    const gchar *window_id = phosh_toplevel_get_app_id (toplevel);
    g_autofree gchar *fixed_id = phosh_fix_app_id (window_id);

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
  object_class->finalize = phosh_app_grid_button_finalize;

  props[PROP_APP_INFO] =
    g_param_spec_object ("app-info", "App", "App Info",
                         G_TYPE_APP_INFO,
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_READWRITE |
                         G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_IS_FAVORITE] =
    g_param_spec_boolean ("is-favorite", "Favorite", "Is a favorite app",
                          FALSE,
                          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/phosh/ui/app-grid-button.ui");

  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGridButton, icon);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGridButton, label);

  gtk_widget_class_bind_template_callback (widget_class, activate_cb);

  signals[APP_LAUNCHED] = g_signal_new ("app-launched",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        0, NULL, NULL, NULL,
                                        G_TYPE_NONE, 1, G_TYPE_APP_INFO);

  gtk_widget_class_set_css_name (widget_class, "phosh-app-grid-button");
}

static void
phosh_app_grid_button_init (PhoshAppGridButton *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
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
                       "is-favorite", TRUE,
                       NULL);
}


void
phosh_app_grid_button_set_app_info (PhoshAppGridButton *self,
                                    GAppInfo *info)
{
  PhoshAppGridButtonPrivate *priv;
  GIcon *icon;
  const gchar* name;

  g_return_if_fail (PHOSH_IS_APP_GRID_BUTTON (self));
  g_return_if_fail (G_IS_APP_INFO (info) || info == NULL);

  priv = phosh_app_grid_button_get_instance_private (self);

  if (priv->info == info)
      return;

  g_clear_object (&priv->info);

  if (info) {
    priv->info = g_object_ref (info);
    name = g_app_info_get_name (G_APP_INFO (priv->info));
    gtk_label_set_label (GTK_LABEL (priv->label), name);
    icon = g_app_info_get_icon (priv->info);
    gtk_image_set_from_gicon (GTK_IMAGE (priv->icon),
                              icon,
                              GTK_ICON_SIZE_DIALOG);
    gtk_widget_set_sensitive (GTK_WIDGET (self), TRUE);
  } else {
    gtk_label_set_label (GTK_LABEL (priv->label), _("Application"));
    gtk_image_set_from_icon_name (GTK_IMAGE (priv->icon),
                                  "application-x-executable",
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
