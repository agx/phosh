/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_SHELL phosh_shell_get_type ()

G_DECLARE_DERIVABLE_TYPE (PhoshShell, phosh_shell, PHOSH, SHELL, GObject)

struct _PhoshShellClass {
  GObjectClass parent_class;
  GType        (*get_lockscreen_type) (PhoshShell *self);

  /* Padding for future expansion */
  void (*_phosh_reserved1) (void);
  void (*_phosh_reserved2) (void);
  void (*_phosh_reserved3) (void);
  void (*_phosh_reserved4) (void);
  void (*_phosh_reserved5) (void);
  void (*_phosh_reserved6) (void);
  void (*_phosh_reserved7) (void);
  void (*_phosh_reserved8) (void);
  void (*_phosh_reserved9) (void);
};

PhoshShell *phosh_shell_new                 (void);

void        phosh_shell_set_default         (PhoshShell *self);
PhoshShell *phosh_shell_get_default         (void);

GType       phosh_shell_get_lockscreen_type (PhoshShell *self);
gboolean    phosh_shell_get_locked          (PhoshShell *self);

G_END_DECLS
