/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#ifndef __PHOSH_H__
#define __PHOSH_H__

#include <gtk/gtk.h>

#define PHOSH_TYPE_SHELL phosh_shell_get_type()

G_DECLARE_FINAL_TYPE (PhoshShell, phosh_shell, PHOSH, SHELL, GObject)

PhoshShell         * phosh                       (void);
void                 phosh_shell_rotate_display  (PhoshShell *self, guint degrees);

#endif /* __PHOSH_H__ */
