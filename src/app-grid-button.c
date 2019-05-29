/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#define G_LOG_DOMAIN "phosh-app-grid-button"

#include "app-grid-button.h"

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
static GParamSpec *pspecs[LAST_PROP] = { NULL, };

enum {
  APP_LAUNCHED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

static void
phosh_app_grid_button_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PhoshAppGridButton *self = PHOSH_APP_GRID_BUTTON (object);
  PhoshAppGridButtonPrivate *priv = phosh_app_grid_button_get_instance_private (self);
  GIcon *icon;
  const gchar* name;

  switch (property_id) {
    case PROP_APP_INFO:
      g_clear_object (&priv->info);
      priv->info = g_value_dup_object (value);
      name = g_app_info_get_display_name (G_APP_INFO (priv->info));
      gtk_label_set_label (GTK_LABEL (priv->label), name);
      icon = g_app_info_get_icon (priv->info);
      gtk_image_set_from_gicon (GTK_IMAGE (priv->icon), icon, GTK_ICON_SIZE_DIALOG);
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
      g_value_set_object (value, priv->info);
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

  g_debug ("Launching %s", g_app_info_get_id (priv->info));

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

  pspecs[PROP_APP_INFO] =
    g_param_spec_object ("app-info", "App", "App Info",
                         G_TYPE_APP_INFO,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  pspecs[PROP_IS_FAVORITE] =
    g_param_spec_boolean ("is-favorite", "Favorite", "Is a favorite app",
                          FALSE,
                          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

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
