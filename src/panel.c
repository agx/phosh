/*
 * Copyright (C) 2017 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 *
 * Based on maynard's panel which is
 *
 * Copyright (C) 2014 Collabora Ltd. *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#include "config.h"

#include "panel.h"

enum {
  FAVORITE_LAUNCHED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct PhoshPanelPrivate {
  gint _dummy;
};

G_DEFINE_TYPE(PhoshPanel, phosh_panel, GTK_TYPE_WINDOW)

static void
phosh_panel_init (PhoshPanel *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      PHOSH_PANEL_TYPE,
      PhoshPanelPrivate);
}


/* FIXME: Temporarily add a term button until we have the menu system */
static void
term_clicked (GtkButton *button, gpointer panel)
{
  GError *error = NULL;

  g_spawn_command_line_async ("weston-terminal", &error);
  if (error)
    g_warning ("Could not launch terminal");
}


static void
phosh_panel_constructed (GObject *object)
{
  PhoshPanel *self = PHOSH_PANEL (object);
  GtkWidget *main_box;
  GtkWidget *button;

  G_OBJECT_CLASS (phosh_panel_parent_class)->constructed (object);

  /* window properties */
  gtk_window_set_title (GTK_WINDOW (self), "phosh panel");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_widget_realize(GTK_WIDGET (self));

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "phosh-panel");

  /* main vbox */
  main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (self), main_box);

  /* FIXME: temporary terminal button */
  button = gtk_button_new_from_icon_name ("terminal", GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_end (GTK_BOX (main_box), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (term_clicked), self);
}


static void
phosh_panel_class_init (PhoshPanelClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = phosh_panel_constructed;

  signals[FAVORITE_LAUNCHED] = g_signal_new ("favorite-launched",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  g_type_class_add_private (object_class, sizeof (PhoshPanelPrivate));
}

GtkWidget *
phosh_panel_new (void)
{
  return g_object_new (PHOSH_PANEL_TYPE,
      NULL);
}
