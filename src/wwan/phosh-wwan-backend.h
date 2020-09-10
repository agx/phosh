/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

/**
 * PhoshWWanBackend:
 * @PHOSH_WWAN_BACKEND_MM: Use ModemManager
 * @PHOSH_WWAN_BACKEND_OFONO: Use oFono
 *
 * Which WWAN backend to use.
 */
typedef enum /*< enum,prefix=PHOSH >*/
{
  PHOSH_WWAN_BACKEND_MM,    /*< nick=modemmanager >*/
  PHOSH_WWAN_BACKEND_OFONO, /*< nick=ofono >*/
} PhoshWWanBackend;
