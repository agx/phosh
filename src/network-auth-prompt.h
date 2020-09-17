/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <gtk/gtk.h>
#include <NetworkManager.h>
#include "layersurface.h"

#define PHOSH_TYPE_NETWORK_AUTH_PROMPT (phosh_network_auth_prompt_get_type())

G_DECLARE_FINAL_TYPE (PhoshNetworkAuthPrompt, phosh_network_auth_prompt, PHOSH, NETWORK_AUTH_PROMPT, PhoshLayerSurface);

GtkWidget *phosh_network_auth_prompt_new         (ShellNetworkAgent *agent,
                                                  NMClient          *nm_client,
                                                  gpointer           layer_shell,
                                                  gpointer           wl_output);
void       phosh_network_auth_prompt_set_request (PhoshNetworkAuthPrompt        *self,
                                                  gchar                         *request_id,
                                                  NMConnection                  *connection,
                                                  gchar                         *setting_name,
                                                  gchar                        **hints,
                                                  NMSecretAgentGetSecretsFlags   flags);
