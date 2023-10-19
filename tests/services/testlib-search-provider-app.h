/*
 * Copyright (C) 2025 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

#include "testlib-search-provider.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_SEARCH_PROVIDER_MOCK_APP (phosh_search_provider_mock_app_get_type ())
G_DECLARE_FINAL_TYPE (PhoshSearchProviderMockApp,
                      phosh_search_provider_mock_app,
                      PHOSH, SEARCH_PROVIDER_MOCK_APP,
                      GApplication)

G_END_DECLS
