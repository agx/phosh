/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#ifndef PHOSH_FAVORITES_H
#define PHOSH_FAVORITES_H

#include "menu.h"

#define PHOSH_TYPE_FAVORITES (phosh_favorites_get_type())

G_DECLARE_FINAL_TYPE (PhoshFavorites, phosh_favorites, PHOSH, FAVORITES, PhoshMenu)

GtkWidget * phosh_favorites_new (int position, const gpointer *shell);

#endif /* PHOSH_FAVORITES_H */
