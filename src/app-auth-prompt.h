/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "system-modal.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_APP_AUTH_PROMPT (phosh_app_auth_prompt_get_type ())

G_DECLARE_FINAL_TYPE (PhoshAppAuthPrompt, phosh_app_auth_prompt, PHOSH, APP_AUTH_PROMPT, PhoshSystemModal)

GtkWidget *phosh_app_auth_prompt_new (GIcon *icon,
                                      const char *title,
                                      const char *subtitle,
                                      const char *body,
                                      const char *grant_label,
                                      const char *deny_label,
                                      gboolean offer_remember);
gboolean phosh_app_auth_prompt_get_grant_access (GtkWidget *self);
gboolean phosh_app_auth_prompt_get_remember (GtkWidget *self);

G_END_DECLS
