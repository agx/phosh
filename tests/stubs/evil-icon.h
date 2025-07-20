/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include <gio/gio.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_EVIL_ICON (phosh_evil_icon_get_type ())

G_DECLARE_FINAL_TYPE (PhoshEvilIcon, phosh_evil_icon, PHOSH, EVIL_ICON, GObject)


GIcon *phosh_evil_icon_new (void);


G_END_DECLS
