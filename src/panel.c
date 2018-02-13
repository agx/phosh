/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Somewhat based on maynard's panel which is
 * Copyright (C) 2014 Collabora Ltd. *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#include "config.h"

#include "panel.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-wall-clock.h>

#define FAVORITES_LABEL_TEXT "Librem5 dev board"

enum {
  FAVORITES_ACTIVATED,
  SETTINGS_ACTIVATED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct PhoshPanelPrivate {
  GtkWidget *btn_favorites;
  GtkWidget *btn_settings;

  GnomeWallClock *wall_clock;
};

G_DEFINE_TYPE_WITH_PRIVATE (PhoshPanel, phosh_panel, GTK_TYPE_WINDOW)


static void
favorites_clicked_cb (PhoshPanel *self, GtkButton *btn)
{
  g_return_if_fail (PHOSH_IS_PANEL (self));
  g_return_if_fail (GTK_IS_BUTTON (btn));
  g_signal_emit(self, signals[FAVORITES_ACTIVATED], 0);
}


static void
settings_clicked_cb (PhoshPanel *self, GtkButton *btn)
{
  g_return_if_fail (PHOSH_IS_PANEL (self));
  g_return_if_fail (GTK_IS_BUTTON (btn));
  g_signal_emit(self, signals[SETTINGS_ACTIVATED], 0);
}


static void
wall_clock_notify_cb (GnomeWallClock *wall_clock,
    GParamSpec *pspec,
    PhoshPanel *self)
{
  GDateTime *datetime;
  PhoshPanelPrivate *priv = phosh_panel_get_instance_private (self);
  g_autofree gchar *str;

  datetime = g_date_time_new_now_local ();

  str = g_date_time_format (datetime, "%H:%M");
  gtk_button_set_label (GTK_BUTTON (priv->btn_settings), str);

  g_date_time_unref (datetime);
}


static void
phosh_panel_constructed (GObject *object)
{
  PhoshPanel *self = PHOSH_PANEL (object);
  PhoshPanelPrivate *priv = phosh_panel_get_instance_private (self);

  G_OBJECT_CLASS (phosh_panel_parent_class)->constructed (object);

  gtk_button_set_label (GTK_BUTTON (priv->btn_favorites), FAVORITES_LABEL_TEXT);
  priv->wall_clock = g_object_new (GNOME_TYPE_WALL_CLOCK, NULL);
  g_signal_connect (priv->wall_clock,
		    "notify::clock",
		    G_CALLBACK (wall_clock_notify_cb),
		    self);

  g_signal_connect_object (priv->btn_favorites,
                           "clicked",
                           G_CALLBACK (favorites_clicked_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (priv->btn_settings,
                           "clicked",
                           G_CALLBACK (settings_clicked_cb),
                           self,
                           G_CONNECT_SWAPPED);

  /* window properties */
  gtk_window_set_title (GTK_WINDOW (self), "phosh panel");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_widget_realize(GTK_WIDGET (self));

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "phosh-panel");

  /* Button properites */
  gtk_style_context_remove_class (gtk_widget_get_style_context (priv->btn_favorites),
				  "button");
  gtk_style_context_remove_class (gtk_widget_get_style_context (priv->btn_favorites),
				  "image-button");
  gtk_style_context_remove_class (gtk_widget_get_style_context (priv->btn_settings),
				  "button");
  gtk_style_context_remove_class (gtk_widget_get_style_context (priv->btn_settings),
				  "image-button");

  wall_clock_notify_cb (priv->wall_clock, NULL, self);
}


static void
phosh_panel_dispose (GObject *object)
{
  PhoshPanel *self = PHOSH_PANEL (object);
  PhoshPanelPrivate *priv = phosh_panel_get_instance_private (self);

  g_clear_object (&priv->wall_clock);

  G_OBJECT_CLASS (phosh_panel_parent_class)->dispose (object);
}


static void
phosh_panel_class_init (PhoshPanelClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_panel_constructed;
  object_class->dispose = phosh_panel_dispose;

  signals[FAVORITES_ACTIVATED] = g_signal_new ("favorites-activated",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  signals[SETTINGS_ACTIVATED] = g_signal_new ("settings-activated",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class,
					       "/sm/puri/phosh/ui/top-panel.ui");
  gtk_widget_class_bind_template_child_private (widget_class, PhoshPanel, btn_favorites);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshPanel, btn_settings);
}


static void
phosh_panel_init (PhoshPanel *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_panel_new (void)
{
  return g_object_new (PHOSH_PANEL_TYPE,
      NULL);
}
