/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "config.h"

#include "lockscreen.h"

enum {
  LOCKSCREEN_UNLOCK,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct PhoshLockscreenPrivate {
  gint _dummy;
};

G_DEFINE_TYPE(PhoshLockscreen, phosh_lockscreen, GTK_TYPE_WINDOW)

static void
phosh_lockscreen_init (PhoshLockscreen *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      PHOSH_LOCKSCREEN_TYPE,
      PhoshLockscreenPrivate);
}


/* FIXME: Temporarily add a button until we interface with pam */
static void
btn_clicked (PhoshLockscreen *lockscreen, GtkButton *btn)
{
  g_signal_emit(lockscreen, signals[LOCKSCREEN_UNLOCK], 0);
}


static void
phosh_lockscreen_constructed (GObject *object)
{
  PhoshLockscreen *self = PHOSH_LOCKSCREEN (object);
  GtkWidget *grid;
  GtkWidget *button;
  GtkWidget *label;

  GdkDisplay *display = gdk_display_get_default ();
  /* There's no primary monitor on nested wayland so just use the
     first one for now */
  GdkMonitor *monitor = gdk_display_get_monitor (display, 0);
  GdkRectangle geom;

  G_OBJECT_CLASS (phosh_lockscreen_parent_class)->constructed (object);

  g_return_if_fail(monitor);
  gdk_monitor_get_geometry (monitor, &geom);

  /* window properties */
  gtk_window_set_title (GTK_WINDOW (self), "phosh lockscreen");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_window_resize (GTK_WINDOW (self), geom.width, geom.height);
  gtk_widget_realize(GTK_WIDGET (self));

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "phosh-lockscreen");

  grid = gtk_widget_new (GTK_TYPE_GRID, "halign", GTK_ALIGN_CENTER, "valign", GTK_ALIGN_CENTER, NULL);
  gtk_container_add (GTK_CONTAINER (self), grid);

  /* FIXME: just a button for now */
  label = gtk_label_new("Click to unlock desktop");
  button = gtk_button_new_from_icon_name ("face-surprise-symbolic", GTK_ICON_SIZE_DIALOG);
  gtk_grid_attach (GTK_GRID (grid), label, 2, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), button, 2, 3, 1, 1);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (btn_clicked), self);
}


static void
phosh_lockscreen_class_init (PhoshLockscreenClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = phosh_lockscreen_constructed;

  signals[LOCKSCREEN_UNLOCK] = g_signal_new ("lockscreen-unlock",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  g_type_class_add_private (object_class, sizeof (PhoshLockscreenPrivate));
}

GtkWidget *
phosh_lockscreen_new (void)
{
  return g_object_new (PHOSH_LOCKSCREEN_TYPE, NULL);
}
