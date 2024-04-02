/*
 * Copyright (C) 2023 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */


#include <gtk/gtk.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_LAUNCHER_BOX (phosh_launcher_box_get_type ())
G_DECLARE_FINAL_TYPE (PhoshLauncherBox, phosh_launcher_box, PHOSH, LAUNCHER_BOX, GtkBox)

G_END_DECLS
