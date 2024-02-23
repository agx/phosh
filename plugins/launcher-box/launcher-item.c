/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "launcher-item.h"

#include <gio/gdesktopappinfo.h>

/**
 * PhoshLauncherItem:
 *
 * A launcher item keeps the app-info and the launcher's indicator state together
 */

enum {
  PROP_0,
  PROP_APP_INFO,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshLauncherItem {
  GObject               parent;

  GDesktopAppInfo      *app_info;
};
G_DEFINE_TYPE (PhoshLauncherItem, phosh_launcher_item, G_TYPE_OBJECT)


static void
phosh_launcher_item_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  PhoshLauncherItem *self = PHOSH_LAUNCHER_ITEM (object);

  switch (property_id) {
  case PROP_APP_INFO:
    g_set_object (&self->app_info, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_launcher_item_get_property (GObject    *object,
                      guint       property_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  PhoshLauncherItem *self = PHOSH_LAUNCHER_ITEM (object);

  switch (property_id) {
  case PROP_APP_INFO:
    g_value_set_object (value, self->app_info);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_launcher_item_dispose (GObject *object)
{
  PhoshLauncherItem *self = PHOSH_LAUNCHER_ITEM (object);

  g_clear_object (&self->app_info);

  G_OBJECT_CLASS (phosh_launcher_item_parent_class)->dispose (object);
}


static void
phosh_launcher_item_class_init (PhoshLauncherItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phosh_launcher_item_get_property;
  object_class->set_property = phosh_launcher_item_set_property;
  object_class->dispose = phosh_launcher_item_dispose;

  props[PROP_APP_INFO] =
    g_param_spec_object ("app-info", "", "",
                         G_TYPE_DESKTOP_APP_INFO,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_launcher_item_init (PhoshLauncherItem *self)
{
}


PhoshLauncherItem *
phosh_launcher_item_new (GDesktopAppInfo *app_info)
{
  return g_object_new (PHOSH_TYPE_LAUNCHER_ITEM, "app-info", app_info, NULL);
}


GDesktopAppInfo *
phosh_launcher_item_get_app_info (PhoshLauncherItem *self)
{
  g_return_val_if_fail (PHOSH_IS_LAUNCHER_ITEM (self), NULL);

  return self->app_info;
}
