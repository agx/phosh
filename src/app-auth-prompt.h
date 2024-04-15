/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include <glib-object.h>
#include "system-modal-dialog.h"

G_BEGIN_DECLS

#define PHOSH_APP_AUTH_PROMPT_CHOICES_FORMAT "a(ssa(ss)s)"

#define PHOSH_TYPE_APP_AUTH_PROMPT (phosh_app_auth_prompt_get_type ())

G_DECLARE_FINAL_TYPE (PhoshAppAuthPrompt, phosh_app_auth_prompt, PHOSH, APP_AUTH_PROMPT, PhoshSystemModalDialog)

GtkWidget *phosh_app_auth_prompt_new (GIcon *icon,
                                      const char *title,
                                      const char *subtitle,
                                      const char *body,
                                      const char *grant_label,
                                      const char *deny_label,
                                      gboolean offer_remember,
                                      GVariant *choices);
gboolean phosh_app_auth_prompt_get_grant_access (GtkWidget *self);
GVariant* phosh_app_auth_prompt_get_selected_choices (GtkWidget *self);
gboolean phosh_app_auth_prompt_get_remember (GtkWidget *self);

G_END_DECLS
