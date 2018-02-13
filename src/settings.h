/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#ifndef __PHOSH_SETTINGS_H__
#define __PHOSH_SETTINGS_H__

#include "menu.h"

#define PHOSH_TYPE_SETTINGS (phosh_settings_get_type())

G_DECLARE_FINAL_TYPE (PhoshSettings, phosh_settings, PHOSH, SETTINGS, PhoshMenu)

GtkWidget * phosh_settings_new (int position, const gpointer* shell);

#endif /* __PHOSH_SETTINGS_H__ */
