/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include <glib/gi18n.h>

#include "menu.h"
#include "phosh-mobile-shell-client-protocol.h"

/**
 * SECTION:phosh-menu
 * @short_description: A menu of the phosh wayland shell
 * @Title: PhoshMenu
 *
 * The #PhoshMenu widget is a shell menu attached to a panel
 * Don't let the wayland details leak to child classes.
 */


typedef struct
{
  struct wl_surface *surface;
  struct phosh_mobile_shell *mshell;
  gboolean shown;
  gchar *name;
  int position;
} PhoshMenuPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshMenu, phosh_menu, GTK_TYPE_WINDOW)


enum {
  PHOSH_MENU_PROP_0 = 0,
  PHOSH_MENU_PROP_SHOWN,
  PHOSH_MENU_PROP_NAME,
  PHOSH_MENU_PROP_SHELL,
  PHOSH_MENU_PROP_POSITION,
  PHOSH_MENU_PROP_LAST_PROP,
};
static GParamSpec *props[PHOSH_MENU_PROP_LAST_PROP] = { NULL, };

enum {
  TOGGLED = 1,
  LAST_SIGNAL,
};
static guint signals [LAST_SIGNAL];


static void
phosh_menu_set_property (GObject *object,
                         guint property_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
  PhoshMenu *self = PHOSH_MENU (object);
  PhoshMenuPrivate *priv = phosh_menu_get_instance_private(self);

  switch (property_id) {
  case PHOSH_MENU_PROP_SHOWN:
    priv->shown = g_value_get_boolean (value);
    break;

  case PHOSH_MENU_PROP_NAME:
    g_free (priv->name);
    priv->name = g_value_dup_string (value);
    break;

  case PHOSH_MENU_PROP_SHELL:
    priv->mshell = g_value_get_pointer (value);
    break;

  case PHOSH_MENU_PROP_POSITION:
    priv->position = g_value_get_int (value);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_menu_get_property (GObject *object,
                         guint property_id,
                         GValue *value,
                         GParamSpec *pspec)
{
  PhoshMenu *self = PHOSH_MENU (object);
  PhoshMenuPrivate *priv = phosh_menu_get_instance_private(self);

  switch (property_id) {
  case PHOSH_MENU_PROP_SHOWN:
    g_value_set_boolean (value, priv->shown);
    break;

  case PHOSH_MENU_PROP_NAME:
    g_value_set_string (value, priv->name);
    break;

  case PHOSH_MENU_PROP_SHELL:
    g_value_set_pointer (value, priv->mshell);

  case PHOSH_MENU_PROP_POSITION:
    g_value_set_int (value, priv->position);

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_menu_constructed (GObject *object)
{
  PhoshMenu *self = PHOSH_MENU (object);
  PhoshMenuPrivate *priv = phosh_menu_get_instance_private (self);
  GdkWindow *gdk_window;
  g_autofree gchar *name;;

  G_OBJECT_CLASS (phosh_menu_parent_class)->constructed (object);

  name = g_strdup_printf("phosh-menu-%s", priv->name);

  /* window properties */
  gtk_window_set_title (GTK_WINDOW (self), name);
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_widget_realize(GTK_WIDGET (self));

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "phosh-menu");

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      name);

  gdk_window = gtk_widget_get_window (GTK_WIDGET (self));
  gdk_wayland_window_set_use_custom_surface (gdk_window);
  priv->surface = gdk_wayland_window_get_wl_surface (gdk_window);

  g_return_if_fail (priv->surface != NULL);

  phosh_mobile_shell_set_panel_menu (priv->mshell, priv->surface);
  phosh_mobile_shell_set_menu_position(priv->mshell,
                                       priv->surface,
                                       priv->position);
}


static void
phosh_menu_finalize (GObject *object)
{
  PhoshMenuPrivate *priv = phosh_menu_get_instance_private (PHOSH_MENU(object));
  GObjectClass *parent_class = G_OBJECT_CLASS (phosh_menu_parent_class);

  g_free (priv->name);

  if (parent_class->finalize != NULL)
    parent_class->finalize (object);
}


static void
phosh_menu_class_init (PhoshMenuClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_menu_constructed;
  object_class->finalize = phosh_menu_finalize;

  object_class->set_property = phosh_menu_set_property;
  object_class->get_property = phosh_menu_get_property;

  /**
   * PhoshMenu::toggled:
   * @self: The #PhoshMenu instance.
   *
   * This signal is emitted when the menu state changes.
   * That is if it's shown or hidden on the screen.
   */
  signals [TOGGLED] = g_signal_new ("toggled",
        G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
        NULL, G_TYPE_NONE, 0);

  props[PHOSH_MENU_PROP_SHOWN] = g_param_spec_boolean ("shown",
                                                       "menu shown",
                                                       "Whether the menu is shown on screen",
                                                       FALSE,
                                                       G_PARAM_READABLE);
  props[PHOSH_MENU_PROP_NAME] = g_param_spec_string ("name",
                                                     "menu name",
                                                     "the menus name",
                                                     "unnamed",
                                                     G_PARAM_CONSTRUCT_ONLY |
                                                     G_PARAM_READWRITE);
  props[PHOSH_MENU_PROP_SHELL] = g_param_spec_pointer ("shell",
                                                       "mobile shell",
                                                       "the mobile shell",
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_READWRITE);
  props[PHOSH_MENU_PROP_POSITION] = g_param_spec_int ("position",
                                                      "menu position",
                                                      "menu position on top bar",
                                                      0,
                                                      INT_MAX,
                                                      PHOSH_MOBILE_SHELL_MENU_POSITION_LEFT,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, PHOSH_MENU_PROP_LAST_PROP, props);
}


/**
 * phosh_menu_new:
 *
 * Create a new #PhoshMenu widget.
 *
 * Returns: the newly created #PhoshMenu widget
 *
 */
GtkWidget *
phosh_menu_new (const char* name, int position,
                const gpointer *shell)
{
  return g_object_new (PHOSH_TYPE_MENU,
                       "name", name,
                       "shell", shell,
                       "position", position,
                       NULL);
}


static void
phosh_menu_init (PhoshMenu *self)
{
  PhoshMenuPrivate *priv = phosh_menu_get_instance_private(self);

  priv->shown = FALSE;
  priv->name = NULL;
  priv->mshell = NULL;
}


gboolean
phosh_menu_is_shown (PhoshMenu *self)
{
  PhoshMenuPrivate *priv = phosh_menu_get_instance_private(self);

  return priv->shown;
}


void
phosh_menu_hide (PhoshMenu *self)
{
  PhoshMenuPrivate *priv = phosh_menu_get_instance_private(self);

  g_return_if_fail (priv->mshell);
  g_return_if_fail (priv->surface);

  if (priv->shown)
    phosh_mobile_shell_hide_panel_menu(priv->mshell,
                                       priv->surface);
  priv->shown = FALSE;
}


void
phosh_menu_show (PhoshMenu *self)
{
  PhoshMenuPrivate *priv = phosh_menu_get_instance_private(self);

  g_return_if_fail (priv->mshell);
  g_return_if_fail (priv->surface);

  if (!priv->shown)
    phosh_mobile_shell_show_panel_menu(priv->mshell,
                                       priv->surface);
  priv->shown = TRUE;
}


gboolean
phosh_menu_toggle (PhoshMenu *self)
{
  PhoshMenuPrivate *priv = phosh_menu_get_instance_private(self);

  priv->shown ? phosh_menu_hide (self) : phosh_menu_show (self);
  return priv->shown;
}
