/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#ifndef __PHOSH_PANEL_H__
#define __PHOSH_PANEL_H__

#include <gtk/gtk.h>

#define PHOSH_PANEL_TYPE                 (phosh_panel_get_type ())
#define PHOSH_PANEL(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHOSH_PANEL_TYPE, PhoshPanel))
#define PHOSH_PANEL_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), PHOSH_PANEL_TYPE, PhoshPanelClass))
#define PHOSH_IS_PANEL(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHOSH_PANEL_TYPE))
#define PHOSH_IS_PANEL_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), PHOSH_PANEL_TYPE))
#define PHOSH_PANEL_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), PHOSH_PANEL_TYPE, PhoshPanelClass))

typedef struct PhoshPanel PhoshPanel;
typedef struct PhoshPanelClass PhoshPanelClass;
typedef struct PhoshPanelPrivate PhoshPanelPrivate;

struct PhoshPanel
{
  GtkWindow parent;
};

struct PhoshPanelClass
{
  GtkWindowClass parent_class;
};

#define PHOSH_PANEL_HEIGHT 32

GType phosh_panel_get_type (void) G_GNUC_CONST;

GtkWidget * phosh_panel_new (void);

#endif /* __PHOSH_PANEL_H__ */
