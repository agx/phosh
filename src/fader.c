/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-fader"

#include "config.h"
#include "fader.h"
#include "shell.h"

#include <handy.h>

/**
 * SECTION:fader
 * @short_description: A fader
 * @Title: PhoshFader
 *
 * A fullsreen surface that fades in
 */

typedef struct _PhoshFader
{
  PhoshLayerSurface parent;

} PhoshFader;
G_DEFINE_TYPE (PhoshFader, phosh_fader, PHOSH_TYPE_LAYER_SURFACE)

static void
phosh_fader_show (GtkWidget *widget)
{
  gboolean enable_animations;
  GtkStyleContext *context;
  
  enable_animations = hdy_get_enable_animations (widget);

  if (enable_animations) {
    context = gtk_widget_get_style_context(widget);
    gtk_style_context_add_class (context, "phosh-fader-fade-in");
  }

  GTK_WIDGET_CLASS (phosh_fader_parent_class)->show (widget);
}
  
static void
phosh_fader_class_init (PhoshFaderClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->show = phosh_fader_show;
}

static void
phosh_fader_init (PhoshFader *self)
{
  GtkStyleContext *context;

  context = gtk_widget_get_style_context(GTK_WIDGET (self));
  gtk_style_context_remove_class(context, "phosh-fader-fade-in");
}


PhoshFader *
phosh_fader_new (gpointer layer_shell, gpointer wl_output)
{
  return g_object_new (PHOSH_TYPE_FADER,
                       "layer-shell", layer_shell,
                       "wl-output", wl_output,
                       "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", -1,
                       "namespace", "phosh fader",
                       NULL);
}
