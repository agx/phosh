/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-osk-button"

#include "osk-button.h"
#include "osk-manager.h"
#include "shell.h"

#include <gio/gio.h>
#include <gtk/gtk.h>

/**
 * PhoshOskButton:
 *
 * A button that toggles the OSK
 *
 * The #PhoshOskButton is responsible for toggling the on screen keyboard
 */
struct _PhoshOskButton
{
  GtkToggleButton parent;

  PhoshOskManager *osk;
  gboolean setting_visibility;
};

G_DEFINE_TYPE (PhoshOskButton, phosh_osk_button, GTK_TYPE_TOGGLE_BUTTON)


static void
toggled_cb (PhoshOskButton *self, gpointer data)
{
  gboolean visible, active;

  self->setting_visibility = TRUE;
  visible = phosh_osk_manager_get_visible (self->osk);

  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self));
  if (visible != active)
    phosh_osk_manager_set_visible (self->osk, active);

  self->setting_visibility = FALSE;
}


static void
on_osk_availability_changed (PhoshOskButton *self, GParamSpec *pspec, PhoshOskManager *osk)
{
  gboolean available;

  g_return_if_fail (PHOSH_IS_OSK_BUTTON (self));
  g_return_if_fail (PHOSH_IS_OSK_MANAGER (osk));
  g_return_if_fail (self->osk == osk);

  available = phosh_osk_manager_get_available (osk);
  gtk_widget_set_sensitive (GTK_WIDGET (self), available);
}


static void
on_osk_visibility_changed (PhoshOskButton *self, GParamSpec *pspec, PhoshOskManager *osk)
{
  gboolean visible;

  g_return_if_fail (PHOSH_IS_OSK_BUTTON (self));
  g_return_if_fail (PHOSH_IS_OSK_MANAGER (osk));
  g_return_if_fail (self->osk == osk);

  visible = phosh_osk_manager_get_visible (osk);
  if (!self->setting_visibility)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self), visible);
}


static void
phosh_osk_button_constructed (GObject *object)
{
  PhoshOskButton *self = PHOSH_OSK_BUTTON (object);
  PhoshShell *shell;
  GtkWidget *image;

  G_OBJECT_CLASS (phosh_osk_button_parent_class)->constructed (object);

  shell = phosh_shell_get_default ();
  self->osk = g_object_ref(phosh_shell_get_osk_manager (shell));

  g_signal_connect_object (self->osk,
                           "notify::visible",
                           G_CALLBACK (on_osk_visibility_changed),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->osk,
                           "notify::available",
                           G_CALLBACK (on_osk_availability_changed),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect (self,
                    "toggled",
                    G_CALLBACK (toggled_cb),
                    NULL);

  image = gtk_image_new_from_icon_name ("input-keyboard-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (self), image);
  gtk_button_set_always_show_image (GTK_BUTTON (self), TRUE);

  on_osk_availability_changed (self, NULL, self->osk);
  on_osk_visibility_changed (self, NULL, self->osk);
}


static void
phosh_osk_button_dispose (GObject *object)
{
  PhoshOskButton *self = PHOSH_OSK_BUTTON (object);

  g_clear_object (&self->osk);
  G_OBJECT_CLASS (phosh_osk_button_parent_class)->dispose (object);
}


static void
phosh_osk_button_class_init (PhoshOskButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_osk_button_constructed;
  object_class->dispose = phosh_osk_button_dispose;
}


static void
phosh_osk_button_init (PhoshOskButton *self)
{
}


GtkWidget *
phosh_osk_button_new (void)
{
  return g_object_new (PHOSH_TYPE_OSK_BUTTON, NULL);
}
