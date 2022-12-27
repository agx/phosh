/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-system-modal"

#include "phosh-config.h"

#include "shell.h"
#include "system-modal.h"

/**
 * PhoshSystemModal:
 *
 * A modal system component
 *
 * The #PhoshSystemModal is used as a base class for other
 * system components such as dialogs like #PhoshSystemPrompt or
 * the OSD display.
 */

enum {
  PROP_0,
  PROP_MONITOR,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct {
  PhoshMonitor               *monitor;

} PhoshSystemModalPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshSystemModal, phosh_system_modal, PHOSH_TYPE_LAYER_SURFACE);

/*
 * Keep track of opened modals
 * Only reset PHOSH_STATE_MODAL_SYSTEM_PROMPT when there are no more modals
 * see phosh_system_modal_map/unmap
 */
static int modal_count = 0;


static void
phosh_system_modal_map (GtkWidget *widget)
{
  g_return_if_fail (PHOSH_IS_SYSTEM_MODAL (widget));

  modal_count++;
  phosh_shell_set_state (phosh_shell_get_default (), PHOSH_STATE_MODAL_SYSTEM_PROMPT, TRUE);

  GTK_WIDGET_CLASS (phosh_system_modal_parent_class)->map (widget);
}


static void
phosh_system_modal_unmap (GtkWidget *widget)
{
  g_return_if_fail (PHOSH_IS_SYSTEM_MODAL (widget));

  modal_count--;

  if (modal_count == 0)
    phosh_shell_set_state (phosh_shell_get_default (), PHOSH_STATE_MODAL_SYSTEM_PROMPT, FALSE);
  else if (modal_count < 0)
    g_warning ("The modal counter is negative %d. This should never happen",
               modal_count);

  GTK_WIDGET_CLASS (phosh_system_modal_parent_class)->unmap (widget);
}

static void
phosh_system_modal_set_property (GObject      *obj,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  PhoshSystemModal *self = PHOSH_SYSTEM_MODAL (obj);
  PhoshSystemModalPrivate *priv = phosh_system_modal_get_instance_private (self);

  switch (prop_id) {
  case PROP_MONITOR:
    g_set_object (&priv->monitor, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    break;
  }
}


static void
phosh_system_modal_get_property (GObject    *obj,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  PhoshSystemModal *self = PHOSH_SYSTEM_MODAL (obj);
  PhoshSystemModalPrivate *priv = phosh_system_modal_get_instance_private (self);

  switch (prop_id) {
  case PROP_MONITOR:
    g_value_set_object (value, priv->monitor);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    break;
  }
}


static void
phosh_system_modal_dispose (GObject *obj)
{
  PhoshSystemModal *self = PHOSH_SYSTEM_MODAL (obj);
  PhoshSystemModalPrivate *priv = phosh_system_modal_get_instance_private (self);

  g_clear_object (&priv->monitor);

  G_OBJECT_CLASS (phosh_system_modal_parent_class)->dispose (obj);
}


static void
phosh_system_modal_constructed (GObject *object)
{
  PhoshSystemModal *self = PHOSH_SYSTEM_MODAL (object);
  PhoshSystemModalPrivate *priv = phosh_system_modal_get_instance_private (self);
  PhoshWayland *wl = phosh_wayland_get_default ();

  if (priv->monitor == NULL)
    priv->monitor = g_object_ref (phosh_shell_get_primary_monitor (phosh_shell_get_default ()));

  g_object_set (PHOSH_LAYER_SURFACE (self),
                "layer-shell", phosh_wayland_get_zwlr_layer_shell_v1 (wl),
                "wl-output", phosh_monitor_get_wl_output (priv->monitor),
                "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                "layer", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
                "kbd-interactivity", TRUE,
                "exclusive-zone", -1,
                "namespace", "phosh system-modal",
                NULL);

  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (self)),
                               "phosh-system-modal");

  G_OBJECT_CLASS (phosh_system_modal_parent_class)->constructed (object);
}


static void
phosh_system_modal_class_init (PhoshSystemModalClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->map = phosh_system_modal_map;
  widget_class->unmap = phosh_system_modal_unmap;

  object_class->get_property = phosh_system_modal_get_property;
  object_class->set_property = phosh_system_modal_set_property;
  object_class->constructed = phosh_system_modal_constructed;
  object_class->dispose = phosh_system_modal_dispose;

  props[PROP_MONITOR] = g_param_spec_object ("monitor",
                                             "Monitor",
                                             "Monitor to put modal on",
                                             PHOSH_TYPE_MONITOR,
                                             G_PARAM_CONSTRUCT_ONLY |
                                             G_PARAM_READWRITE |
                                             G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_system_modal_init (PhoshSystemModal *self)
{
}

/**
 * phosh_system_modal_new:
 * @monitor: The #PhoshMonitor to put the modal surface on. If %NULL the primary monitor is used.
 *
 * Create a new system-modal surface.
 */
GtkWidget *
phosh_system_modal_new (PhoshMonitor *monitor)
{
  g_return_val_if_fail (PHOSH_IS_MONITOR (monitor) || monitor == NULL, NULL);

  return g_object_new (PHOSH_TYPE_SYSTEM_MODAL, "monitor", monitor, NULL);
}
