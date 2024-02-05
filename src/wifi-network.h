/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>
#include <glib-2.0/glib.h>
#include <gio/gio.h>

#include <NetworkManager.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_WIFI_NETWORK phosh_wifi_network_get_type ()
G_DECLARE_FINAL_TYPE (PhoshWifiNetwork, phosh_wifi_network, PHOSH, WIFI_NETWORK, GObject)

PhoshWifiNetwork *phosh_wifi_network_new_from_access_point (NMAccessPoint *ap, gboolean active);

gboolean       phosh_wifi_network_matches_access_point (PhoshWifiNetwork *self, NMAccessPoint *ap);

void           phosh_wifi_network_add_access_point (PhoshWifiNetwork *self,
                                                    NMAccessPoint    *ap,
                                                    gboolean          active);
gboolean       phosh_wifi_network_remove_access_point (PhoshWifiNetwork *self, NMAccessPoint *ap);

char          *phosh_wifi_network_get_ssid (PhoshWifiNetwork *self);
gboolean       phosh_wifi_network_get_secured (PhoshWifiNetwork *self);
NM80211Mode    phosh_wifi_network_get_mode (PhoshWifiNetwork *self);
guint          phosh_wifi_network_get_strength (PhoshWifiNetwork *self);
gboolean       phosh_wifi_network_get_active (PhoshWifiNetwork *self);
void           phosh_wifi_network_update_active (PhoshWifiNetwork *self,
                                                 NMAccessPoint    *active_ap);
void           phosh_wifi_network_set_is_connecting (PhoshWifiNetwork *self,
                                                     gboolean          is_connecting);
gboolean       phosh_wifi_network_get_is_connecting (PhoshWifiNetwork *self);
NMAccessPoint *phosh_wifi_network_get_best_access_point (PhoshWifiNetwork *self);

G_END_DECLS
