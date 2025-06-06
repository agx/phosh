/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Arun Mani J <arunmani@peartree.to>
 */

#define G_LOG_DOMAIN "phosh-wifi-status-page"

#include "shell-priv.h"
#include "status-page-placeholder.h"
#include "wifi-network-row.h"
#include "wifi-status-page.h"

/**
 * PhoshWifiStatusPage:
 *
 * A widget to display list of Wi-Fi access points when the corresponding PhoshQuickSetting is
 * activated.
 */

struct _PhoshWifiStatusPage {
  PhoshStatusPage             parent_instance;

  GtkStack                   *wifi_scan;
  GtkSpinner                 *wifi_scan_spinner;
  GtkStack                   *stack;
  GtkListBox                 *networks;
  PhoshStatusPagePlaceholder *empty_state;
  GtkButton                  *empty_state_btn;

  PhoshWifiManager           *wifi;
  char                       *connecting_network;
};

G_DEFINE_TYPE (PhoshWifiStatusPage, phosh_wifi_status_page, PHOSH_TYPE_STATUS_PAGE);

static void
set_visible_page (PhoshWifiStatusPage *self, GParamSpec *pspec, PhoshWifiManager *wifi)
{
  gboolean wifi_absent = !phosh_wifi_manager_get_present (self->wifi);
  gboolean wifi_disabled = !phosh_wifi_manager_get_enabled (self->wifi);
  gboolean hotspot_enabled = phosh_wifi_manager_is_hotspot_master (self->wifi);
  GListModel *devices = G_LIST_MODEL (phosh_wifi_manager_get_networks (self->wifi));
  gboolean empty_state = TRUE;
  const char *icon_name;
  const char *title;
  const char *button_label;

  if (wifi_absent) {
    icon_name = "network-wireless-hardware-disabled-symbolic";
    title = _("No Wi-Fi Device Found");
    button_label = NULL;
  } else if (wifi_disabled) {
    icon_name = "network-wireless-disabled-symbolic";
    title = _("Wi-Fi Disabled");
    button_label = _("Enable Wi-Fi");
  } else if (hotspot_enabled) {
    icon_name = "network-wireless-hotspot-symbolic";
    title = _("Wi-Fi Hotspot Active");
    button_label = _("Turn Off");
  } else if (g_list_model_get_item (devices, 0) == NULL) {
    icon_name = "network-wireless-no-route-symbolic";
    title = _("No Wi-Fi Hotspots");
    button_label = NULL;
  } else {
    empty_state = FALSE;
  }

  if (empty_state) {
    phosh_status_page_placeholder_set_icon_name (self->empty_state, icon_name);
    phosh_status_page_placeholder_set_title (self->empty_state, title);
    gtk_button_set_label (self->empty_state_btn, button_label);
    gtk_widget_set_visible (GTK_WIDGET (self->empty_state_btn), button_label != NULL);
    gtk_stack_set_visible_child_name (self->stack, "empty-state");
  } else {
    gtk_stack_set_visible_child_name (self->stack, "list_box");
  }

  gtk_widget_set_visible (GTK_WIDGET (self->wifi_scan), !wifi_disabled && !hotspot_enabled);
}


static void
on_ssid_changed (PhoshWifiStatusPage *self, GParamSpec *pspec, PhoshWifiManager *manager)
{
  const char *ssid;

  g_assert (PHOSH_IS_WIFI_STATUS_PAGE (self));
  g_assert (PHOSH_IS_WIFI_MANAGER (manager));

  ssid = phosh_wifi_manager_get_ssid (manager);
  if (ssid && self->connecting_network && g_str_equal (ssid, self->connecting_network)) {
    g_signal_emit_by_name (self, "done", NULL);
    g_clear_pointer (&self->connecting_network, g_free);
  }
}


static void
on_scanning_changed (PhoshWifiStatusPage *self, GParamSpec *pspec, PhoshWifiManager *manager)
{
  gboolean scanning;
  const char *page;

  g_assert (PHOSH_IS_WIFI_STATUS_PAGE (self));
  g_assert (PHOSH_IS_WIFI_MANAGER (manager));

  scanning = phosh_wifi_manager_get_scanning (manager);
  if (scanning)
    gtk_spinner_start (self->wifi_scan_spinner);
  else
    gtk_spinner_stop (self->wifi_scan_spinner);

  page = scanning ? "spinner" : "button";
  gtk_stack_set_visible_child_name (self->wifi_scan, page);
}


static void
on_placeholder_clicked (PhoshWifiStatusPage *self, GtkWidget *widget)
{
  gboolean wifi_disabled = !phosh_wifi_manager_get_enabled (self->wifi);
  gboolean hotspot_enabled = phosh_wifi_manager_is_hotspot_master (self->wifi);

  if (wifi_disabled)
    phosh_wifi_manager_set_enabled (self->wifi, TRUE);
  else if (hotspot_enabled)
    phosh_wifi_manager_set_hotspot_master (self->wifi, FALSE);
}


static void
on_wifi_scan_clicked (PhoshWifiStatusPage *self)
{
  phosh_wifi_manager_request_scan (self->wifi);
}


static void
on_network_activated (PhoshWifiStatusPage *self, GtkWidget *row)
{
  gint index;
  PhoshWifiNetwork *network;

  g_return_if_fail (GTK_IS_LIST_BOX_ROW (row));

  index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (row));
  network = g_list_model_get_item (G_LIST_MODEL (phosh_wifi_manager_get_networks (self->wifi)),
                                   index);

  /* Remember the network we're connecting to so we can close the
   * status page if connecting works */
  g_free (self->connecting_network);
  self->connecting_network = g_strdup (phosh_wifi_network_get_ssid (network));

  phosh_wifi_manager_connect_network (self->wifi, network);
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

  g_clear_pointer (&self->connecting_network, g_free);

  G_OBJECT_CLASS (phosh_wifi_status_page_parent_class)->dispose (object);
}


static void
phosh_wifi_status_page_class_init (PhoshWifiStatusPageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = phosh_wifi_status_page_dispose;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/ui/wifi-status-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PhoshWifiStatusPage, wifi_scan);
  gtk_widget_class_bind_template_child (widget_class, PhoshWifiStatusPage, wifi_scan_spinner);
  gtk_widget_class_bind_template_child (widget_class, PhoshWifiStatusPage, stack);
  gtk_widget_class_bind_template_child (widget_class, PhoshWifiStatusPage, networks);
  gtk_widget_class_bind_template_child (widget_class, PhoshWifiStatusPage, empty_state);
  gtk_widget_class_bind_template_child (widget_class, PhoshWifiStatusPage, empty_state_btn);

  gtk_widget_class_bind_template_callback (widget_class, on_network_activated);
  gtk_widget_class_bind_template_callback (widget_class, on_placeholder_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_wifi_scan_clicked);
}


static void
phosh_wifi_status_page_init (PhoshWifiStatusPage *self)
{
  PhoshShell *shell;
  GListModel *model;

  gtk_widget_init_template (GTK_WIDGET (self));

  shell = phosh_shell_get_default ();
  self->wifi = g_object_ref (phosh_shell_get_wifi_manager (shell));

  if (self->wifi == NULL) {
    g_warning ("Failed to get Wi-Fi manager");
    return;
  }

  model = G_LIST_MODEL (phosh_wifi_manager_get_networks (self->wifi));

  g_object_connect (self->wifi,
                    "swapped-object-signal::notify::present", set_visible_page, self,
                    "swapped-object-signal::notify::enabled", set_visible_page, self,
                    "swapped-object-signal::notify::is-hotspot-master", set_visible_page, self,
                    "swapped-object-signal::notify::ssid", on_ssid_changed, self,
                    "swapped-object-signal::notify::scanning", on_scanning_changed, self,
                    NULL);

  g_signal_connect_object (model,
                           "items-changed",
                           G_CALLBACK (set_visible_page),
                           self,
                           G_CONNECT_SWAPPED);

  gtk_list_box_bind_model (self->networks,
                           model,
                           (GtkListBoxCreateWidgetFunc) create_network_row,
                           NULL,
                           NULL);
}


GtkWidget *
phosh_wifi_status_page_new (void)
{
  return g_object_new (PHOSH_TYPE_WIFI_STATUS_PAGE, NULL);
}
