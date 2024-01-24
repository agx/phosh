/*
 * Copyright (C) 2024 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "background-manager.h"


struct _PhoshBackgroundManager {
  GObject parent;
};


G_DEFINE_TYPE (PhoshBackgroundManager, phosh_background_manager, G_TYPE_OBJECT);


static void
phosh_background_manager_class_init (PhoshBackgroundManagerClass *klass)
{
}


static void
phosh_background_manager_init (PhoshBackgroundManager *self)
{
}


PhoshBackgroundManager *
phosh_background_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_BACKGROUND_MANAGER, NULL);
}


PhoshBackgroundData *
phosh_background_manager_get_data (PhoshBackgroundManager *self, PhoshBackground *background)
{
  g_autoptr (PhoshBackgroundData) bg_data = g_new0 (PhoshBackgroundData, 1);

  g_return_val_if_fail (PHOSH_IS_BACKGROUND_MANAGER (self), NULL);
  g_return_val_if_fail (PHOSH_IS_BACKGROUND (background), NULL);

  return g_steal_pointer (&bg_data);
}
