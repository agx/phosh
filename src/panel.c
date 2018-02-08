/*
 * Copyright (C) 2017 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 *
 * Somewhat based on maynard's panel which is
 *
 * Copyright (C) 2014 Collabora Ltd. *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#include "config.h"

#include "panel.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-wall-clock.h>

#define TOPLEFT_LABEL_TEXT "Librem5 dev board"

enum {
  FAVORITE_LAUNCHED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct PhoshPanelPrivate {
  GtkWidget *label_topleft;
  GtkWidget *label_clock;
  GtkWidget *btn_terminal;

  GnomeWallClock *wall_clock;
};

G_DEFINE_TYPE_WITH_PRIVATE (PhoshPanel, phosh_panel, GTK_TYPE_WINDOW)



/* FIXME: Temporarily add a term button until we have the menu system */
static void
term_clicked_cb (PhoshPanel *self, GtkButton *btn)
{
  GError *error = NULL;

  g_return_if_fail (PHOSH_IS_PANEL (self));
  g_return_if_fail (GTK_IS_BUTTON (btn));

  g_spawn_command_line_async ("weston-terminal", &error);
  if (error)
    g_warning ("Could not launch terminal");
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
  gtk_label_set_markup (GTK_LABEL (priv->label_clock), str);

  g_date_time_unref (datetime);
}


static void
phosh_panel_constructed (GObject *object)
{
  PhoshPanel *self = PHOSH_PANEL (object);
  PhoshPanelPrivate *priv = phosh_panel_get_instance_private (self);

  G_OBJECT_CLASS (phosh_panel_parent_class)->constructed (object);

  gtk_label_set_text (GTK_LABEL (priv->label_topleft), TOPLEFT_LABEL_TEXT);
  priv->wall_clock = g_object_new (GNOME_TYPE_WALL_CLOCK, NULL);
  g_signal_connect (priv->wall_clock,
		    "notify::clock",
		    G_CALLBACK (wall_clock_notify_cb),
		    self);

  g_signal_connect_object (priv->btn_terminal,
                           "clicked",
                           G_CALLBACK (term_clicked_cb),
                           self,
                           G_CONNECT_SWAPPED);

  /* window properties */
  gtk_window_set_title (GTK_WINDOW (self), "phosh panel");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_widget_realize(GTK_WIDGET (self));

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "phosh-panel");

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

  signals[FAVORITE_LAUNCHED] = g_signal_new ("favorite-launched",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class,
					       "/org/librem5/phosh/ui/top-panel.ui");
  gtk_widget_class_bind_template_child_private (widget_class, PhoshPanel, label_topleft);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshPanel, label_clock);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshPanel, btn_terminal);
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
