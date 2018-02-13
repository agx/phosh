/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3+
 */

#ifndef PHOSH_MENU_H
#define PHOSH_MENU_H

#include <gtk/gtk.h>
#include <gdk/gdkwayland.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_MENU (phosh_menu_get_type())

G_DECLARE_DERIVABLE_TYPE (PhoshMenu, phosh_menu, PHOSH, MENU, GtkWindow)

/**
 * PhoshMenuClass
 * @parent_class: The parent class
 */
struct _PhoshMenuClass
{
  GtkWindowClass parent_class;
};

GtkWidget *          phosh_menu_new            (const char* name,
						int position,
						const gpointer *shell);
gboolean             phosh_menu_is_shown       (PhoshMenu *self);
void                 phosh_menu_show           (PhoshMenu *self);
void                 phosh_menu_hide           (PhoshMenu *self);
gboolean             phosh_menu_toggle         (PhoshMenu *self);

G_END_DECLS

#endif /* PHOSH_MENU_H */
