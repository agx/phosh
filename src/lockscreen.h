/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#ifndef PHOSH_LOCKSCREEN_H
#define PHOSH_LOCKSCREEN_H

#include <gtk/gtk.h>

#define PHOSH_LOCKSCREEN_TYPE                 (phosh_lockscreen_get_type ())
#define PHOSH_LOCKSCREEN(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHOSH_LOCKSCREEN_TYPE, PhoshLockscreen))
#define PHOSH_LOCKSCREEN_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), PHOSH_LOCKSCREEN_TYPE, PhoshLockscreenClass))
#define PHOSH_IS_LOCKSCREEN(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHOSH_LOCKSCREEN_TYPE))
#define PHOSH_IS_LOCKSCREEN_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), PHOSH_LOCKSCREEN_TYPE))
#define PHOSH_LOCKSCREEN_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), PHOSH_LOCKSCREEN_TYPE, PhoshLockscreenClass))

typedef struct PhoshLockscreen PhoshLockscreen;
typedef struct PhoshLockscreenClass PhoshLockscreenClass;
typedef struct PhoshLockscreenPrivate PhoshLockscreenPrivate;

struct PhoshLockscreen
{
  GtkWindow parent;

  PhoshLockscreenPrivate *priv;
};

struct PhoshLockscreenClass
{
  GtkWindowClass parent_class;
};

GType phosh_lockscreen_get_type (void) G_GNUC_CONST;

GtkWidget * phosh_lockscreen_new (void);

#endif /* PHOSH_LOCKSCREEN_H */
