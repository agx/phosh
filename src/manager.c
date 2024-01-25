/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-manager"

#include "phosh-config.h"

#include "manager.h"

/**
 * PhoshManager:
 *
 * Base class for manager objects
 *
 * Common functionality for manager objects.
 */

typedef struct
{
  guint        idle_id;
} PhoshManagerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshManager, phosh_manager, G_TYPE_OBJECT);


static gboolean
on_idle (PhoshManager *self)
{
  PhoshManagerClass *klass = PHOSH_MANAGER_GET_CLASS (self);
  PhoshManagerPrivate *priv = phosh_manager_get_instance_private (self);

  if (klass->idle_init)
    (*klass->idle_init) (self);

  priv->idle_id = 0;
  return G_SOURCE_REMOVE;
}


static void
phosh_manager_dispose (GObject *object)
{
  PhoshManager *self = PHOSH_MANAGER (object);
  PhoshManagerPrivate *priv = phosh_manager_get_instance_private (self);

  g_clear_handle_id (&priv->idle_id, g_source_remove);

  G_OBJECT_CLASS (phosh_manager_parent_class)->dispose (object);
}


static void
phosh_manager_constructed (GObject *object)
{
  PhoshManager *self = PHOSH_MANAGER (object);
  PhoshManagerClass *klass = PHOSH_MANAGER_GET_CLASS (object);
  PhoshManagerPrivate *priv = phosh_manager_get_instance_private (self);

  G_OBJECT_CLASS (phosh_manager_parent_class)->constructed (object);

  if (klass->idle_init) {
    priv->idle_id = g_idle_add ((GSourceFunc) on_idle, self);
    g_source_set_name_by_id (priv->idle_id, "[PhoshManager] idle");
  }
}


static void
phosh_manager_class_init (PhoshManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_manager_constructed;
  object_class->dispose = phosh_manager_dispose;
}

static void
phosh_manager_init (PhoshManager *self)
{
}
