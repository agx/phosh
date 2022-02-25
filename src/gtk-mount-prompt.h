/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "system-modal-dialog.h"

#define PHOSH_TYPE_GTK_MOUNT_PROMPT (phosh_gtk_mount_prompt_get_type ())

G_DECLARE_FINAL_TYPE (PhoshGtkMountPrompt, phosh_gtk_mount_prompt, PHOSH, GTK_MOUNT_PROMPT, PhoshSystemModalDialog)

GtkWidget        *phosh_gtk_mount_prompt_new (const char *message,
                                              const char *icon_name,
                                              const char *default_user,
                                              const char *default_domain,
                                              GVariant   *pids,
                                              const char *const *choices,
                                              GAskPasswordFlags ask_flags);
const gchar      *phosh_gtk_mount_prompt_get_password (PhoshGtkMountPrompt *self);
GAskPasswordFlags phosh_gtk_mount_prompt_get_ask_flags (PhoshGtkMountPrompt *self);
gboolean          phosh_gtk_mount_prompt_get_cancelled (PhoshGtkMountPrompt *self);
int               phosh_gtk_mount_prompt_get_choice (PhoshGtkMountPrompt *self);
GStrv             phosh_gtk_mount_prompt_get_choices (PhoshGtkMountPrompt *self);
void              phosh_gtk_mount_prompt_set_pids (PhoshGtkMountPrompt *self, GVariant *pids);
