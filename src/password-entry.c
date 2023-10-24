/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Rudra Pratap Singh <rudra@peartree.to>
 */

#include "password-entry.h"

#define PASSWORD_EYE_OPEN  "eye-open-negative-filled-symbolic"
#define PASSWORD_EYE_CLOSE "eye-not-looking-symbolic"

struct _PhoshPasswordEntry {
  GtkEntry parent_instance;

  gboolean visibility;
};

G_DEFINE_TYPE (PhoshPasswordEntry, phosh_password_entry, GTK_TYPE_ENTRY)


static void
on_icon_press (PhoshPasswordEntry *self, gpointer user_data)
{
  const char *icon_name;

  self->visibility = !self->visibility;
  icon_name = self->visibility ? PASSWORD_EYE_CLOSE : PASSWORD_EYE_OPEN;

  gtk_entry_set_visibility (GTK_ENTRY (self), self->visibility);
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (self),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     icon_name);
}


static void
phosh_password_entry_class_init (PhoshPasswordEntryClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/password-entry.ui");
  gtk_widget_class_bind_template_callback (widget_class, on_icon_press);
}


static void
phosh_password_entry_init (PhoshPasswordEntry *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


PhoshPasswordEntry *
phosh_password_entry_new (void)
{
  return PHOSH_PASSWORD_ENTRY (g_object_new (PHOSH_TYPE_PASSWORD_ENTRY, NULL));
}