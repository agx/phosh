/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Thomas Booker <tw.booker@outlook.com>
 */

#define G_LOG_DOMAIN "phosh-emergency-menu"

#include "phosh-config.h"

#include "emergency-menu.h"
#include "emergency-calls-manager.h"
#include "emergency-contact-row.h"
#include "system-modal-dialog.h"
#include "shell.h"
#include "util.h"

#include <glib/gi18n.h>
#include <handy.h>


/**
 * PhoshEmergencyMenu:
 *
 * A menu that allows the user to dial an emergency service, see
 * emergency info and quickly call emergency contacts.
 *
 * #PhoshEmergencyMenu is a menu that allows the user to dial an
 * emergency service, see emergency info and quickly call emergency
 * contacts.
 *
 * The #PhoshEmergencyMenu is designed to be integrated with the lock
 * screen via an emergency button.
 *
 * The fetching of emergency contact and calling of emergency
 * contact/service is done via a DBus API with the Calls app, it is
 * managed by the #PhoshEmergencyContactManager class.
 *
 * The emergency contacts list bind a GListStore provided by the
 * #PhoshEmergencyContactManager to a `GtkListBox` using
 * `gtk_list_box_bind_model` function.
 */

enum {
  DONE,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct _PhoshEmergencyMenu {
  PhoshSystemModalDialog        parent;

  PhoshEmergencyCallsManager   *manager;

  HdyCarousel                  *emergency_carousel;
  GtkBox                       *emergency_dialpad_box;
  GtkBox                       *emergency_info_box;
  GtkListBox                   *emergency_contacts_list_box;
  GtkLabel                     *emergency_owner_name;
  GtkWidget                    *placeholder;

  GSimpleActionGroup           *actions;
};
G_DEFINE_TYPE (PhoshEmergencyMenu, phosh_emergency_menu, PHOSH_TYPE_SYSTEM_MODAL_DIALOG);


static void
on_go_back_activated (GSimpleAction *action, GVariant *param, gpointer data)
{
  PhoshEmergencyMenu *self = PHOSH_EMERGENCY_MENU (data);
  gboolean close;

  close = g_variant_get_boolean (param);
  if (close) {
    g_signal_emit (self, signals[DONE], 0);
  } else {
    hdy_carousel_scroll_to (self->emergency_carousel, GTK_WIDGET (self->emergency_dialpad_box));
  }
}


static void
on_emergency_menu_done (PhoshEmergencyMenu *self)
{
  g_return_if_fail (PHOSH_IS_EMERGENCY_MENU (self));
  g_signal_emit (self, signals[DONE], 0);
}


static void
on_dial_error (PhoshEmergencyMenu *self, GError *error)
{
  PhoshSystemModalDialog *error_dialog = PHOSH_SYSTEM_MODAL_DIALOG (phosh_system_modal_dialog_new ());
  GtkWidget *error_label = gtk_label_new (NULL);
  GtkWidget *ok_button = gtk_button_new_with_label (_("Ok"));

  phosh_system_modal_dialog_add_button (error_dialog, ok_button, -1);
  phosh_system_modal_dialog_set_title (error_dialog, _("Unable to place emergency call"));

  if (g_dbus_error_strip_remote_error (error))
    gtk_label_set_label (GTK_LABEL (error_label), error->message);
  else
    gtk_label_set_label (GTK_LABEL (error_label), _("Internal error"));
  gtk_label_set_line_wrap (GTK_LABEL (error_label), TRUE);
  phosh_system_modal_dialog_set_content (error_dialog, error_label);

  g_signal_connect_swapped (ok_button, "clicked", G_CALLBACK (gtk_widget_destroy), error_dialog);
  g_signal_connect_swapped (error_dialog,
                            "dialog-canceled",
                            G_CALLBACK (gtk_widget_destroy),
                            error_dialog);

  gtk_widget_show_all (GTK_WIDGET (error_dialog));
}


static GtkWidget *
create_emergency_contact_row (gpointer item, gpointer unused)
{
  PhoshEmergencyContact *contact = PHOSH_EMERGENCY_CONTACT (item);

  return GTK_WIDGET (phosh_emergency_contact_row_new (contact));
}


static void
on_n_items_changed (PhoshEmergencyMenu *self, GParamSpec *pspec, GListStore *store)
{
  gboolean visible;

  g_return_if_fail (PHOSH_IS_EMERGENCY_MENU (self));
  g_return_if_fail (G_IS_LIST_STORE (store));

  /* Work around https://gitlab.gnome.org/GNOME/libhandy/-/issues/468 */
  visible = !g_list_model_get_n_items (G_LIST_MODEL (store));
  gtk_widget_set_visible (self->placeholder, visible);
}



static void
emergency_menu_constructed (GObject *object)
{
  PhoshEmergencyMenu *self = PHOSH_EMERGENCY_MENU (object);
  GListStore *emergency_contacts_list;

  G_OBJECT_CLASS (phosh_emergency_menu_parent_class)->constructed (object);

  self->manager = phosh_shell_get_emergency_calls_manager (phosh_shell_get_default ());

  emergency_contacts_list = phosh_emergency_calls_manager_get_list_store (self->manager);
  gtk_list_box_bind_model (self->emergency_contacts_list_box,
                           G_LIST_MODEL (emergency_contacts_list),
                           create_emergency_contact_row,
                           NULL,
                           NULL);
  g_signal_connect_object (emergency_contacts_list, "notify::n-items",
                           G_CALLBACK (on_n_items_changed),
                           self,
                           G_CONNECT_SWAPPED);
  on_n_items_changed (self, NULL, emergency_contacts_list);

  gtk_label_set_label (self->emergency_owner_name, g_get_real_name ());
  g_signal_connect_swapped (self->manager,
                            "dial-error",
                            G_CALLBACK (on_dial_error),
                            self);
}


static void
emergency_menu_dispose (GObject *object)
{
  PhoshEmergencyMenu *self = PHOSH_EMERGENCY_MENU (object);

  g_clear_object (&self->actions);

  G_OBJECT_CLASS (phosh_emergency_menu_parent_class)->dispose (object);
}

static void
on_dialpad_dialed (PhoshEmergencyMenu *self, const gchar *number)
{
  phosh_emergency_calls_manager_call (self->manager, number);
}

/**
 * on_emergency_contacts_list_box_activated:
 *
 * When the user clicks on an emergency contact call that contact via
 * the phosh_emergency_calls_row_call function.
 */
static void
on_emergency_contacts_list_box_activated (PhoshEmergencyMenu *self, PhoshEmergencyContactRow* row, GtkListBox* list)
{
  phosh_emergency_contact_row_call (row, self->manager);
}

/**
 * emergency_contacts_button_pressed_cb:
 *
 * When the "Emergency Info" button on the dial pad screen is pressed
 * send the user to the info screen.
 */
static void
on_emergency_contacts_button_clicked (PhoshEmergencyMenu *self)
{
  g_debug ("Emergency info button pressed");
  hdy_carousel_scroll_to (self->emergency_carousel, GTK_WIDGET (self->emergency_info_box));
}


static void
phosh_emergency_menu_class_init (PhoshEmergencyMenuClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed  = emergency_menu_constructed;
  object_class->dispose  = emergency_menu_dispose;

  signals[DONE] = g_signal_new ("done",
                                G_TYPE_FROM_CLASS (klass),
                                G_SIGNAL_RUN_LAST,
                                0, NULL, NULL,
                                NULL, G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/phosh/ui/emergency-menu.ui");

  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyMenu, emergency_carousel);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyMenu, emergency_dialpad_box);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyMenu, emergency_info_box);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyMenu, emergency_contacts_list_box);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyMenu, emergency_owner_name);
  gtk_widget_class_bind_template_child (widget_class, PhoshEmergencyMenu, placeholder);
  gtk_widget_class_bind_template_callback (widget_class, on_emergency_contacts_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_emergency_contacts_list_box_activated);
  gtk_widget_class_bind_template_callback (widget_class, on_dialpad_dialed);
  gtk_widget_class_bind_template_callback (widget_class, on_emergency_menu_done);

  gtk_widget_class_set_css_name (widget_class, "phosh-emergency-menu");
}


static GActionEntry entries[] = {
  { .name = "go-back", .parameter_type = "b", .activate = on_go_back_activated, .state = "1" },
};


static void
phosh_emergency_menu_init (PhoshEmergencyMenu *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->actions = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (self->actions),
                                   entries, G_N_ELEMENTS (entries),
                                   self);
  gtk_widget_insert_action_group (GTK_WIDGET (self), "emergency-menu",
                                  G_ACTION_GROUP (self->actions));
}


PhoshEmergencyMenu *
phosh_emergency_menu_new (void)
{
  return g_object_new (PHOSH_TYPE_EMERGENCY_MENU, NULL);
}
