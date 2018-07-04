/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-lockshield"

#include "config.h"

#include "lockshield.h"

/**
 * SECTION:phosh-lockshield
 * @short_description: Lock shield for non primary screens
 * @Title: PhoshLockshield
 *
 * The #PhoshLockshield is displayed on lock screens which are not the
 * primary one.
 */
typedef struct
{
  GtkWidget *background;
} PhoshLockshieldPrivate;


struct _PhoshLockshield
{
  PhoshLayerSurfaceClass parent;
};

G_DEFINE_TYPE_WITH_PRIVATE(PhoshLockshield, phosh_lockshield, PHOSH_TYPE_LAYER_SURFACE) 


static gboolean
draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  PhoshLockshield *self = PHOSH_LOCKSHIELD (widget);
  PhoshLockshieldPrivate *priv = phosh_lockshield_get_instance_private (self);
  GtkStyleContext *context;
  gint width, height;

  width = gtk_widget_get_allocated_width (priv->background);
  height = gtk_widget_get_allocated_height (priv->background);

  context = gtk_widget_get_style_context (priv->background);
  gtk_render_background (context, cr, 0, 0, width, height);
  return FALSE;
}


static void
phosh_lockshield_constructed (GObject *object)
{
  PhoshLockshield *self = PHOSH_LOCKSHIELD (object);
  PhoshLockshieldPrivate *priv = phosh_lockshield_get_instance_private (self);

  G_OBJECT_CLASS (phosh_lockshield_parent_class)->constructed (object);

  priv->background = gtk_drawing_area_new ();

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "phosh-lockshield");

  g_signal_connect (GTK_WIDGET (self),
                    "draw",
                    G_CALLBACK (draw_cb),
                    NULL);
}


static void
phosh_lockshield_class_init (PhoshLockshieldClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = phosh_lockshield_constructed;
}


static void
phosh_lockshield_init (PhoshLockshield *self)
{
}


GtkWidget *
phosh_lockshield_new (gpointer layer_shell,
                      gpointer wl_output)
{
  return g_object_new (PHOSH_TYPE_LOCKSHIELD,
                       "layer-shell", layer_shell,
                       "wl-output", wl_output,
                       "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", -1,
                       "namespace", "lockshield",
                       NULL);
}
