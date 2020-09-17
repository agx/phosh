/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_OSK_MANAGER (phosh_osk_manager_get_type())

G_DECLARE_FINAL_TYPE (PhoshOskManager, phosh_osk_manager, PHOSH, OSK_MANAGER, GObject)

PhoshOskManager * phosh_osk_manager_new           (void);
gboolean          phosh_osk_manager_get_available (PhoshOskManager *self);
void              phosh_osk_manager_set_visible   (PhoshOskManager *self, gboolean visible);
gboolean          phosh_osk_manager_get_visible   (PhoshOskManager *self);

G_END_DECLS
