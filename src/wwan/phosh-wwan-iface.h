/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_WWAN (phosh_wwan_get_type())
G_DECLARE_INTERFACE (PhoshWWan, phosh_wwan, PHOSH, WWAN, GObject)

/**
 * PhoshWWanInterface
 * @parent_iface: The parent interface
 * @get_signal_quality: Get the current signal quality
 * @get_access_tec: Get the current access technology (2G, 3G, ...)
 * @is_unlocked: whether the SIM in the modem is locked
 * @has_sim: Whether there's a sim in the modem
 * @is_present: whether a modem is present at all
 * @is_enabled: whether a modem is enabled
 * @get_operator: Get the current network operator name
 *
 * A #PhoshWWanInterface handles modem interaction such as getting
 * network information and signal strength.
 */

struct _PhoshWWanInterface
{
  GTypeInterface parent_iface;

  guint         (*get_signal_quality) (PhoshWWan *self);
  const char*   (*get_access_tec)     (PhoshWWan *self);
  gboolean      (*is_unlocked)        (PhoshWWan *self);
  gboolean      (*has_sim)            (PhoshWWan *self);
  gboolean      (*is_present)         (PhoshWWan *self);
  gboolean      (*is_enabled)         (PhoshWWan *self);
  const char*   (*get_operator)       (PhoshWWan *self);
};

guint         phosh_wwan_get_signal_quality (PhoshWWan* self);
const char*   phosh_wwan_get_access_tec     (PhoshWWan* self);
gboolean      phosh_wwan_is_unlocked        (PhoshWWan* self);
gboolean      phosh_wwan_has_sim            (PhoshWWan* self);
gboolean      phosh_wwan_is_present         (PhoshWWan* self);
gboolean      phosh_wwan_is_enabled         (PhoshWWan *self);
void          phosh_wwan_set_enabled        (PhoshWWan *self, gboolean enabled);
const char   *phosh_wwan_get_operator       (PhoshWWan *self);
