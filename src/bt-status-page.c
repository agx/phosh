/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-bt-status-page"

#include "bt-device-row.h"
#include "bt-status-page.h"
#include "status-page-placeholder.h"

#include "bluetooth-device.h"

#include <handy.h>

/**
 * PhoshBtStatusPage:
 *
 * A Quick setting status page widget to show Bluetooth devices
 */

struct _PhoshBtStatusPage {
  PhoshStatusPage             parent_instance;

  GtkListBox                 *devices_list_box;
  GtkStack                   *stack;
  PhoshStatusPagePlaceholder *empty_state;
  GtkButton                  *enable_button;

  PhoshBtManager             *bt_manager;
};

G_DEFINE_TYPE (PhoshBtStatusPage, phosh_bt_status_page, PHOSH_TYPE_STATUS_PAGE);


static GtkWidget *
create_bt_device_row (BluetoothDevice *device)
{
  return phosh_bt_device_row_new (device);
}


static void
phosh_bt_status_page_dispose (GObject *object)
{
  PhoshBtStatusPage *self = PHOSH_BT_STATUS_PAGE (object);

  g_clear_object (&self->bt_manager);

  G_OBJECT_CLASS (phosh_bt_status_page_parent_class)->dispose (object);
}


static void
phosh_bt_status_page_class_init (PhoshBtStatusPageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = phosh_bt_status_page_dispose;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/bt-status-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PhoshBtStatusPage, empty_state);
  gtk_widget_class_bind_template_child (widget_class, PhoshBtStatusPage, devices_list_box);
  gtk_widget_class_bind_template_child (widget_class, PhoshBtStatusPage, enable_button);
  gtk_widget_class_bind_template_child (widget_class, PhoshBtStatusPage, stack);
}


static void
update_stack_page_cb (PhoshBtStatusPage *self)
{
  const char *page_name, *title;
  gboolean show_enable_button;

  if (phosh_bt_manager_get_enabled (self->bt_manager)) {
    GListModel *devices = phosh_bt_manager_get_connectable_devices (self->bt_manager);
    guint n_devices = g_list_model_get_n_items (devices);

    show_enable_button = FALSE;
    title = _("No connectable Bluetooth Devices found");
    page_name = n_devices ? "devices" : "empty-state";
  } else {
    show_enable_button = TRUE;
    title = _("Bluetooth disabled");
    page_name = "empty-state";
  }

  phosh_status_page_placeholder_set_title (self->empty_state, title);
  gtk_stack_set_visible_child_name (self->stack, page_name);
  gtk_widget_set_visible (GTK_WIDGET (self->enable_button), show_enable_button);
}


static void
phosh_bt_status_page_init (PhoshBtStatusPage *self)
{
  PhoshShell *shell;

  gtk_widget_init_template (GTK_WIDGET (self));

  shell = phosh_shell_get_default ();
  self->bt_manager = g_object_ref (phosh_shell_get_bt_manager (shell));
  g_return_if_fail (PHOSH_IS_BT_MANAGER (self->bt_manager));

  gtk_list_box_bind_model (self->devices_list_box,
                           phosh_bt_manager_get_connectable_devices (self->bt_manager),
                           (GtkListBoxCreateWidgetFunc)create_bt_device_row,
                           NULL,
                           NULL);

  g_signal_connect_object (self->bt_manager, "notify::n-devices",
                           G_CALLBACK (update_stack_page_cb), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->bt_manager, "notify::enabled",
                           G_CALLBACK (update_stack_page_cb), self, G_CONNECT_SWAPPED);
  update_stack_page_cb (self);
}


GtkWidget *
phosh_bt_status_page_new (void)
{
  return g_object_new (PHOSH_TYPE_BT_STATUS_PAGE, NULL);
}
