/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Florian Loers <florianloers@mailbox.org>
 */

#include <gio/gio.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_AUTH_PROMPT_OPTION (phosh_auth_prompt_option_get_type ())

G_DECLARE_FINAL_TYPE (PhoshAuthPromptOption, phosh_auth_prompt_option, PHOSH, AUTH_PROMPT_OPTION, GObject)

const char *phosh_auth_prompt_option_get_id (PhoshAuthPromptOption *self);
const char *phosh_auth_prompt_option_get_label (PhoshAuthPromptOption *self);

G_END_DECLS
