/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-system-modal"

#include "config.h"

#include "shell.h"
#include "system-modal.h"

/**
 * SECTION:system-modal
 * @short_description: A modal system modal
 * @Title: PhoshSystemModal
 *
 * The #PhoshSystemModal is used as a base class for other
 * system modal dialogs like #PhoshSystemPrompt.
 */

enum {
  PROP_0,
  PROP_MONITOR,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct {
  PhoshMonitor               *monitor;
  struct zwlr_layer_shell_v1 *layer_shell;

} PhoshSystemModalPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshSystemModal, phosh_system_modal, PHOSH_TYPE_LAYER_SURFACE);


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

  G_OBJECT_CLASS (phosh_system_modal_parent_class)->constructed (object);
}


static void
phosh_system_modal_class_init (PhoshSystemModalClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->get_property = phosh_system_modal_get_property;
  object_class->set_property = phosh_system_modal_set_property;
  object_class->constructed = phosh_system_modal_constructed;
  object_class->dispose = phosh_system_modal_dispose;

  props[PROP_MONITOR] = g_param_spec_object ("monitor",
                                             "Monitor",
                                             "Monitor to put dialog on",
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
 * @monitor: The #PhoshMonitor to put the dialog on. If %NULL the primary monitor is used.
 *
 * Create a new system-modal dialog.
 */
GtkWidget *
phosh_system_modal_new (PhoshMonitor *monitor)
{
  g_return_val_if_fail (PHOSH_IS_MONITOR (monitor) || monitor == NULL, NULL);

  return g_object_new (PHOSH_TYPE_SYSTEM_MODAL, "monitor", monitor, NULL);
}
