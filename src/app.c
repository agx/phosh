/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-app"

#include "config.h"
#include "app.h"
#include "phosh.h"

#include <gio/gdesktopappinfo.h>

/**
 * SECTION:phosh-app
 * @short_description: An app in the faovorites overview
 * @Title: PhoshApp
 *
 * The #PhoshApp is used to select a running application
 * in the favorites overview.
 */

#define APP_ICON_SIZE GTK_ICON_SIZE_DIALOG

enum {
  PHOSH_APP_PROP_0,
  PHOSH_APP_PROP_APP_ID,
  PHOSH_APP_PROP_TITLE,
  PHOSH_APP_PROP_LAST_PROP,
};
static GParamSpec *props[PHOSH_APP_PROP_LAST_PROP];

typedef struct
{
  GtkWidget *image;
  GtkWidget *box;

  char *app_id;
  char *title;
  GDesktopAppInfo *info;
} PhoshAppPrivate;


struct _PhoshApp
{
  GtkButton parent;
};

G_DEFINE_TYPE_WITH_PRIVATE(PhoshApp, phosh_app, GTK_TYPE_BUTTON)


static void
phosh_app_set_property (GObject *object,
                          guint property_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
  PhoshApp *self = PHOSH_APP (object);
  PhoshAppPrivate *priv = phosh_app_get_instance_private(self);

  switch (property_id) {
  case PHOSH_APP_PROP_APP_ID:
    g_free (priv->app_id);
    priv->app_id = g_value_dup_string (value);
    g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_APP_PROP_APP_ID]);
    break;
  case PHOSH_APP_PROP_TITLE:
    g_free (priv->title);
    priv->title = g_value_dup_string (value);
    g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_APP_PROP_TITLE]);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_app_get_property (GObject *object,
                          guint property_id,
                          GValue *value,
                          GParamSpec *pspec)
{
  PhoshApp *self = PHOSH_APP (object);
  PhoshAppPrivate *priv = phosh_app_get_instance_private(self);

  switch (property_id) {
  case PHOSH_APP_PROP_APP_ID:
    g_value_set_string (value, priv->app_id);
    break;
  case PHOSH_APP_PROP_TITLE:
    g_value_set_string (value, priv->title);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


/* In GTK+4 we can just call gtk_image_new_from_gicon since it got rid of the
   restrictive size element */
static GtkWidget *
get_image_from_gicon (PhoshApp *self, int scale)
{
  PhoshAppPrivate *priv = phosh_app_get_instance_private (self);
  GtkIconTheme *theme = gtk_icon_theme_get_for_screen (gdk_screen_get_default ());
  GIcon *icon;
  GError *err = NULL;
  g_autoptr(GdkPixbuf) pixbuf = NULL;
  g_autoptr(GtkIconInfo) info = NULL;

  icon = g_app_info_get_icon (G_APP_INFO (priv->info));
  info = gtk_icon_theme_lookup_by_gicon_for_scale(theme, icon, 128, scale, 0);
  if (!info) {
    g_warning ("Failed to lookup icon for %s", priv->app_id);
    return NULL;
  }
  pixbuf = gtk_icon_info_load_icon (info, &err);
  if (!pixbuf) {
    g_warning ("Failed to load icon for %s: %s", priv->app_id, err->message);
    return NULL;
  }
  return gtk_image_new_from_pixbuf (pixbuf);
}


static GtkWidget *
get_missing_image (int scale)
{
  GtkIconTheme *theme = gtk_icon_theme_get_for_screen (gdk_screen_get_default ());
  GError *err = NULL;
  g_autoptr(GdkPixbuf) pixbuf = NULL;
  g_autoptr(GtkIconInfo) info = NULL;

  info = gtk_icon_theme_lookup_icon_for_scale(theme, "image-missing", 128, scale, 0);
  pixbuf = gtk_icon_info_load_icon (info, &err);
  return gtk_image_new_from_pixbuf (pixbuf);
}


static void
phosh_app_constructed (GObject *object)
{
  PhoshApp *self = PHOSH_APP (object);
  PhoshAppPrivate *priv = phosh_app_get_instance_private (self);
  GtkWidget *lbl_name = NULL, *lbl_title = NULL;
  g_autofree gchar *desktop_id = NULL;
  g_autofree gchar *name = NULL;
  gint scale = 1;
  PhoshMonitor *monitor = phosh_shell_get_primary_monitor (phosh_shell_get_default());

  if (monitor)
    scale = monitor->scale;

  desktop_id = g_strdup_printf ("%s.desktop", priv->app_id);
  g_return_if_fail (desktop_id);
  priv->info = g_desktop_app_info_new (desktop_id);
  if (priv->info) {
    priv->image = get_image_from_gicon (self, scale);
    name = g_desktop_app_info_get_locale_string (priv->info, "Name");
    lbl_name = gtk_label_new (name ? name : priv->app_id);
  } else {
    lbl_name = gtk_label_new (priv->app_id);
  }
  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(lbl_name)),
                               "phosh-app-name");

  if (!priv->info || !priv->image) {
    priv->image = get_missing_image (scale);
  }

  if (priv->title) {
      lbl_title = gtk_label_new (priv->title);
      gtk_label_set_max_width_chars (GTK_LABEL (lbl_title), 30);
      gtk_label_set_ellipsize (GTK_LABEL (lbl_title), PANGO_ELLIPSIZE_MIDDLE);
      gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(lbl_title)),
                                   "phosh-app-title");
  }

  priv->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (priv->box), priv->image, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (priv->box), lbl_name, FALSE, FALSE, 0);
  if (lbl_title) {
    gtk_box_pack_start (GTK_BOX (priv->box), lbl_title, FALSE, FALSE, 0);
  }
  gtk_container_add (GTK_CONTAINER (self), priv->box);

  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(self)),
                               "phosh-app-btn");

  G_OBJECT_CLASS (phosh_app_parent_class)->constructed (object);
}


static void
phosh_app_dispose (GObject *object)
{
  PhoshApp *self = PHOSH_APP (object);
  PhoshAppPrivate *priv = phosh_app_get_instance_private (self);

  g_clear_object (&priv->info);

  G_OBJECT_CLASS (phosh_app_parent_class)->dispose (object);
}


static void
phosh_app_finalize (GObject *object)
{
  PhoshApp *self = PHOSH_APP (object);
  PhoshAppPrivate *priv = phosh_app_get_instance_private (self);

  g_free (priv->app_id);
  g_free (priv->title);

  G_OBJECT_CLASS (phosh_app_parent_class)->finalize (object);
}



static void
phosh_app_class_init (PhoshAppClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = phosh_app_constructed;
  object_class->dispose = phosh_app_dispose;
  object_class->finalize = phosh_app_finalize;

  object_class->set_property = phosh_app_set_property;
  object_class->get_property = phosh_app_get_property;

  props[PHOSH_APP_PROP_APP_ID] =
    g_param_spec_string (
      "app-id",
      "app-id",
      "The application id",
      "",
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PHOSH_APP_PROP_TITLE] =
    g_param_spec_string (
      "title",
      "title",
      "The window's title",
      "",
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PHOSH_APP_PROP_LAST_PROP, props);
}


static void
phosh_app_init (PhoshApp *self)
{
}


GtkWidget *
phosh_app_new (const char *app_id, const char *title)
{
  return g_object_new (PHOSH_TYPE_APP,
                       "app-id", app_id,
                       "title", title,
                       NULL);
}


const char *
phosh_app_get_app_id (PhoshApp *self)
{
  PhoshAppPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_APP (self), NULL);
  priv = phosh_app_get_instance_private (self);

  return priv->app_id;
}


const char *
phosh_app_get_title (PhoshApp *self)
{
  PhoshAppPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_APP (self), NULL);
  priv = phosh_app_get_instance_private (self);

  return priv->title;
}
