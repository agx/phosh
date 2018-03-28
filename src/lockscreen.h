/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#ifndef PHOSH_LOCKSCREEN_H
#define PHOSH_LOCKSCREEN_H

#include <gtk/gtk.h>

#define PHOSH_LOCKSCREEN_TYPE                 (phosh_lockscreen_get_type ())

G_DECLARE_FINAL_TYPE (PhoshLockscreen, phosh_lockscreen, PHOSH, LOCKSCREEN, GtkWindow)

GtkWidget * phosh_lockscreen_new (void);

#endif /* PHOSH_LOCKSCREEN_H */
