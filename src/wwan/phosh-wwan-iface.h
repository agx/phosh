/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */
#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_WWAN (phosh_wwan_get_type())
G_DECLARE_INTERFACE (PhoshWWan, phosh_wwan, PHOSH, WWAN, GObject)

struct _PhoshWWanInterface
{
  GTypeInterface parent_iface;

  guint         (*get_signal_quality) (PhoshWWan *self);
  const char*   (*get_access_tec)     (PhoshWWan *self);
  gboolean      (*is_unlocked)        (PhoshWWan *self);
  gboolean      (*has_sim)            (PhoshWWan *self);
  gboolean      (*is_present)         (PhoshWWan *self);
  const char*   (*get_operator)       (PhoshWWan *self);
};

guint         phosh_wwan_get_signal_quality (PhoshWWan* self);
const char*   phosh_wwan_get_access_tec     (PhoshWWan* self);
gboolean      phosh_wwan_is_unlocked        (PhoshWWan* self);
gboolean      phosh_wwan_has_sim            (PhoshWWan* self);
gboolean      phosh_wwan_is_present         (PhoshWWan* self);
const gchar  *phosh_wwan_get_operator       (PhoshWWan *self);
