/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Arun Mani J <arunmani@peartree.to>
 */

#define G_LOG_DOMAIN "phosh-wifi-status-page"

#include "wifi-status-page.h"

/**
 * PhoshWifiStatusPage:
 *
 * A widget to display list of Wi-Fi access points when the corresponding PhoshQuickSetting is
 * activated.
 */

struct _PhoshWifiStatusPage {
  PhoshStatusPage   parent_instance;
  GtkSwitch        *wifi_switch;
  GtkStack         *stack;
  GtkListBox       *networks;
  PhoshWifiManager *wifi;
};

G_DEFINE_TYPE (PhoshWifiStatusPage, phosh_wifi_status_page, PHOSH_TYPE_STATUS_PAGE);

static void
set_visible_page (PhoshWifiStatusPage *self, GParamSpec *pspec, PhoshWifiManager *wifi)
{
  gboolean wifi_absent = !phosh_wifi_manager_get_present (self->wifi);
  gboolean wifi_disabled = !phosh_wifi_manager_get_enabled (self->wifi);
  gboolean hotspot_enabled = phosh_wifi_manager_is_hotspot_master (self->wifi);
  char *page;

  if (wifi_absent)
    page = "wifi_absent";
  else if (hotspot_enabled)
    page = "hotspot_enabled";
  else if (wifi_disabled)
    page = "wifi_disabled";
  else
    page = "list_box";

  gtk_stack_set_visible_child_name (self->stack, page);
  gtk_widget_set_visible (GTK_WIDGET (self->wifi_switch), !wifi_absent && !hotspot_enabled);
  gtk_switch_set_active (self->wifi_switch, !wifi_disabled);
}

static void
enable_wifi (PhoshWifiStatusPage *self, GtkWidget *widget)
{
  phosh_wifi_manager_set_enabled (self->wifi, TRUE);
}

static void
on_switch_active_changed (PhoshWifiStatusPage *self, GParamSpec *pspec, GtkSwitch *_switch)
{
  gboolean enabled = gtk_switch_get_active (self->wifi_switch);

  phosh_wifi_manager_set_enabled (self->wifi, enabled);
}

static void
on_network_activated_cb (PhoshWifiStatusPage *self, GtkWidget *row)
{
  gint index;
  PhoshWifiNetwork *network;

  g_return_if_fail (GTK_IS_LIST_BOX_ROW (row));

  index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (row));
  network = g_list_model_get_item (G_LIST_MODEL (phosh_wifi_manager_get_networks (self->wifi)),
                                   index);
  phosh_wifi_manager_connect_network (self->wifi, network);
}

static void
scan_for_networks (PhoshWifiStatusPage *self, GtkWidget *row)
{
  phosh_wifi_manager_request_scan (self->wifi);
}

static GtkWidget *
create_network_row (PhoshWifiNetwork *network)
{
  GtkWidget *row;

  row = phosh_wifi_network_row_new (network);
  return row;
}

static void
phosh_wifi_status_page_dispose (GObject *object)
{
  PhoshWifiStatusPage *self = PHOSH_WIFI_STATUS_PAGE (object);

  if (self->wifi) {
    g_signal_handlers_disconnect_by_data (self->wifi, self);
    g_clear_object (&self->wifi);
  }

  G_OBJECT_CLASS (phosh_wifi_status_page_parent_class)->dispose (object);
}

static void
phosh_wifi_status_page_class_init (PhoshWifiStatusPageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = phosh_wifi_status_page_dispose;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/wifi-status-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PhoshWifiStatusPage, wifi_switch);
  gtk_widget_class_bind_template_child (widget_class, PhoshWifiStatusPage, stack);
  gtk_widget_class_bind_template_child (widget_class, PhoshWifiStatusPage, networks);

  gtk_widget_class_bind_template_callback (widget_class, enable_wifi);
  gtk_widget_class_bind_template_callback (widget_class, on_network_activated_cb);
  gtk_widget_class_bind_template_callback (widget_class, scan_for_networks);
}

static void
phosh_wifi_status_page_init (PhoshWifiStatusPage *self)
{
  PhoshShell *shell;

  gtk_widget_init_template (GTK_WIDGET (self));

  shell = phosh_shell_get_default ();
  self->wifi = g_object_ref (phosh_shell_get_wifi_manager (shell));

  if (self->wifi == NULL) {
    g_warning ("Failed to get Wi-Fi manager");
    return;
  }

  g_signal_connect_object (self->wifi_switch,
                           "notify::active",
                           G_CALLBACK (on_switch_active_changed),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->wifi,
                           "notify::present",
                           G_CALLBACK (set_visible_page),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->wifi,
                           "notify::enabled",
                           G_CALLBACK (set_visible_page),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->wifi,
                           "notify::is-hotspot-master",
                           G_CALLBACK (set_visible_page),
                           self,
                           G_CONNECT_SWAPPED);

  gtk_list_box_bind_model (self->networks,
                           G_LIST_MODEL (phosh_wifi_manager_get_networks (self->wifi)),
                           (GtkListBoxCreateWidgetFunc) create_network_row,
                           NULL,
                           NULL);
}

GtkWidget *
phosh_wifi_status_page_new (void)
{
  return g_object_new (PHOSH_TYPE_WIFI_STATUS_PAGE, NULL);
}
