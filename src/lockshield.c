/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-lockshield"

#include "phosh-config.h"

#include "lockshield.h"

/**
 * PhoshLockshield:
 *
 * Lock shield for non primary screens
 *
 * The #PhoshLockshield is displayed on lock screens
 * which are not the primary one.
 */
struct _PhoshLockshield
{
  PhoshLayerSurface parent;
};

G_DEFINE_TYPE(PhoshLockshield, phosh_lockshield, PHOSH_TYPE_LAYER_SURFACE)


static void
phosh_lockshield_constructed (GObject *object)
{
  PhoshLockshield *self = PHOSH_LOCKSHIELD (object);

  G_OBJECT_CLASS (phosh_lockshield_parent_class)->constructed (object);

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "phosh-lockshield");
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
phosh_lockshield_new (struct zwlr_layer_shell_v1 *layer_shell,
                      PhoshMonitor               *monitor)
{
  return g_object_new (PHOSH_TYPE_LOCKSHIELD,
                       "layer-shell", layer_shell,
                       "wl-output", monitor->wl_output,
                       "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", -1,
                       "namespace", "phosh lockshield",
                       NULL);
}
