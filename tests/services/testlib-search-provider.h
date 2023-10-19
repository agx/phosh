/*
 * Copyright (C) 2025 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gio/gio.h>
#include <glib.h>
#include <glib-object.h>
#include "phosh-search-provider-mock-generated.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_SEARCH_PROVIDER_MOCK (phosh_search_provider_mock_get_type())
G_DECLARE_FINAL_TYPE (PhoshSearchProviderMock, phosh_search_provider_mock, PHOSH, SEARCH_PROVIDER_MOCK, GObject)

PhoshSearchProviderMock *phosh_search_provider_mock_new (void);

gboolean phosh_search_provider_mock_dbus_register (PhoshSearchProviderMock  *provider,
                                                   GDBusConnection          *connection,
                                                   const char               *object_path,
                                                   GError                  **error);
void     phosh_search_provider_mock_dbus_unregister (PhoshSearchProviderMock  *provider,
                                                     GDBusConnection          *connection,
                                                     const char               *object_path);

G_END_DECLS
