/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-widget-box"

#include "phosh-config.h"

#include "plugin-loader.h"
#include "widget-box.h"

#include <handy.h>
#include <glib/gi18n-lib.h>

/**
 * PhoshWidgetBox:
 *
 * A box of widgets for the lock screen
 *
 * The widget box is displayed on the lock screen
 * and displays a list of loadable widgets.
 */

enum {
  PROP_0,
  PROP_PLUGIN_DIRS,
  PROP_PLUGINS,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshWidgetBox {
  GtkBox                parent;

  GtkWidget            *carousel;
  PhoshPluginLoader    *plugin_loader;

  GStrv                 plugin_dirs;
  GStrv                 plugins;
};
G_DEFINE_TYPE (PhoshWidgetBox, phosh_widget_box, GTK_TYPE_BOX)


static GtkWidget *
missing_plugin_widget_new (const char *plugin)
{
  GtkWidget *widget = hdy_status_page_new ();
  g_autofree char *msg = NULL;

  hdy_status_page_set_title (HDY_STATUS_PAGE (widget), _("Plugin not found"));

  hdy_status_page_set_icon_name (HDY_STATUS_PAGE (widget), "dialog-error-symbolic");
  msg = g_strdup_printf (_("The plugin '%s' could not be loaded."), plugin);
  hdy_status_page_set_description (HDY_STATUS_PAGE (widget), msg);

  return widget;
}


static void
phosh_widget_box_load_widgets (PhoshWidgetBox *self)
{
  g_autoptr (GList) children = NULL;

  if (self->plugin_loader == NULL)
    return;

  children = gtk_container_get_children (GTK_CONTAINER (self->carousel));
  for (GList *elem = children; elem; elem = elem->next)
    gtk_container_remove (GTK_CONTAINER (self->carousel), GTK_WIDGET (elem->data));

  for (int i = 0; i < g_strv_length (self->plugins); i++) {
    GtkWidget *widget = phosh_plugin_loader_load_plugin (self->plugin_loader, self->plugins[i]);

    if (widget == NULL) {
      g_warning ("Plugin '%s' not found", self->plugins[i]);
      widget = missing_plugin_widget_new (self->plugins[i]);
    }

    gtk_widget_show (widget);
    gtk_widget_set_hexpand (widget, TRUE);
    hdy_carousel_insert (HDY_CAROUSEL (self->carousel), widget, -1);
  }
}


static void
phosh_widget_box_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  PhoshWidgetBox *self = PHOSH_WIDGET_BOX (object);

  switch (property_id) {
  case PROP_PLUGIN_DIRS:
    g_strfreev (self->plugin_dirs);
    self->plugin_dirs = g_value_dup_boxed (value);
    break;
  case PROP_PLUGINS:
    phosh_widget_box_set_plugins (self, g_value_get_boxed (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_widget_box_get_property (GObject    *object,
                      guint       property_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  PhoshWidgetBox *self = PHOSH_WIDGET_BOX (object);

  switch (property_id) {
  case PROP_PLUGIN_DIRS:
    g_value_set_boxed (value, self->plugin_dirs);
    break;
  case PROP_PLUGINS:
    g_value_set_boxed (value, self->plugins);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_widget_box_constructed (GObject *object)
{
  PhoshWidgetBox *self = PHOSH_WIDGET_BOX(object);
  const char *plugin_dirs[] = { PHOSH_PLUGINS_DIR, NULL };

  G_OBJECT_CLASS (phosh_widget_box_parent_class)->constructed (object);

  if (self->plugin_dirs == NULL)
    self->plugin_dirs = g_strdupv ((GStrv)plugin_dirs);

  self->plugin_loader = phosh_plugin_loader_new (self->plugin_dirs,
                                                 PHOSH_EXTENSION_POINT_LOCKSCREEN_WIDGET);

}


static void
phosh_widget_box_finalize (GObject *object)
{
  PhoshWidgetBox *self = PHOSH_WIDGET_BOX(object);

  g_clear_object (&self->plugin_loader);
  g_clear_pointer (&self->plugins, g_strfreev);
  g_clear_pointer (&self->plugin_dirs, g_strfreev);

  G_OBJECT_CLASS (phosh_widget_box_parent_class)->finalize (object);
}


static void
phosh_widget_box_class_init (PhoshWidgetBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_widget_box_get_property;
  object_class->set_property = phosh_widget_box_set_property;
  object_class->constructed = phosh_widget_box_constructed;
  object_class->finalize = phosh_widget_box_finalize;

  props[PROP_PLUGIN_DIRS] =
    g_param_spec_boxed ("plugin-dirs", "", "",
                        G_TYPE_STRV,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_PLUGINS] =
    g_param_spec_boxed ("plugins", "", "",
                        G_TYPE_STRV,
                        G_PARAM_READWRITE);
  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/widget-box.ui");

  gtk_widget_class_bind_template_child (widget_class, PhoshWidgetBox, carousel);

  gtk_widget_class_set_css_name (widget_class, "phosh-widget-box");
}


static void
phosh_widget_box_init (PhoshWidgetBox *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


PhoshWidgetBox *
phosh_widget_box_new (GStrv plugin_dirs)
{
  return PHOSH_WIDGET_BOX (g_object_new (PHOSH_TYPE_WIDGET_BOX,
                                         "plugin-dirs", plugin_dirs,
                                         NULL));
}


void
phosh_widget_box_set_plugins (PhoshWidgetBox *self, GStrv plugins)
{
  g_return_if_fail (PHOSH_IS_WIDGET_BOX (self));

  g_clear_pointer (&self->plugins, g_strfreev);
  self->plugins = g_strdupv (plugins);

  phosh_widget_box_load_widgets (self);
}


gboolean
phosh_widget_box_has_plugins (PhoshWidgetBox *self)
{
  g_return_val_if_fail (PHOSH_IS_WIDGET_BOX (self), FALSE);

  return g_strv_length (self->plugins) != 0;
}
