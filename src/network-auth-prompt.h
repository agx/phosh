/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <gtk/gtk.h>
#include <NetworkManager.h>
#include "system-modal-dialog.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_NETWORK_AUTH_PROMPT (phosh_network_auth_prompt_get_type())

G_DECLARE_FINAL_TYPE (PhoshNetworkAuthPrompt, phosh_network_auth_prompt, PHOSH, NETWORK_AUTH_PROMPT, PhoshSystemModalDialog);

GtkWidget *phosh_network_auth_prompt_new         (ShellNetworkAgent *agent);
void       phosh_network_auth_prompt_set_request (PhoshNetworkAuthPrompt        *self,
                                                  char                          *request_id,
                                                  NMConnection                  *connection,
                                                  char                          *setting_name,
                                                  char                         **hints,
                                                  NMSecretAgentGetSecretsFlags   flags);
G_END_DECLS
