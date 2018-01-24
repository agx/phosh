/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#ifndef __PHOSH_LOCKSCREEN_H__
#define __PHOSH_LOCKSCREEN_H__

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

#define PHOSH_LOCKSCREEN_WIDTH 56
#define PHOSH_LOCKSCREEN_HEIGHT_RATIO 0.73

GType phosh_lockscreen_get_type (void) G_GNUC_CONST;

GtkWidget * phosh_lockscreen_new (void);

#endif /* __PHOSH_LOCKSCREEN_H__ */
