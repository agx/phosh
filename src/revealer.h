/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_REVEALER (phosh_revealer_get_type ())

G_DECLARE_FINAL_TYPE (PhoshRevealer, phosh_revealer, PHOSH, REVEALER, GtkBin)

PhoshRevealer *           phosh_revealer_new (void);
GtkWidget *               phosh_revealer_get_child (PhoshRevealer *self);
void                      phosh_revealer_set_child (PhoshRevealer *self, GtkWidget *child);
gboolean                  phosh_revealer_get_show_child (PhoshRevealer *self);
void                      phosh_revealer_set_show_child (PhoshRevealer *self, gboolean show_child);
guint                     phosh_revealer_get_transition_duration (PhoshRevealer *self);
void                      phosh_revealer_set_transition_duration (PhoshRevealer *self,
                                                                  guint          transition_duration);
GtkRevealerTransitionType phosh_revealer_get_transition_type (PhoshRevealer *self);
void                      phosh_revealer_set_transition_type (PhoshRevealer *self,
                                                              GtkRevealerTransitionType
                                                              transition_type);

G_END_DECLS
