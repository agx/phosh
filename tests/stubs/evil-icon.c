/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-evil-icon"

#include "evil-icon.h"


typedef struct _PhoshEvilIcon {
  GObject     parent;
} PhoshEvilIcon;


static void icon_iface_init (GIconIface *iface);

G_DEFINE_TYPE_WITH_CODE (PhoshEvilIcon, phosh_evil_icon, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ICON, icon_iface_init))


static void
phosh_evil_icon_class_init (PhoshEvilIconClass *klass)
{

}


static guint
phosh_evil_icon_hash (GIcon *self)
{
  return 42;
}


static gboolean
phosh_evil_icon_equal (GIcon *self, GIcon *other)
{
  return FALSE;
}


static GVariant *
phosh_evil_icon_serialize (GIcon *self)
{
  return NULL;
}


static void
icon_iface_init (GIconIface *iface)
{
  iface->hash = phosh_evil_icon_hash;
  iface->equal = phosh_evil_icon_equal;
  iface->serialize = phosh_evil_icon_serialize;
}


static void
phosh_evil_icon_init (PhoshEvilIcon *self)
{

}


GIcon *
phosh_evil_icon_new (void)
{
  return g_object_new (PHOSH_TYPE_EVIL_ICON, NULL);
}
