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

/**
 * PhoshNMSecret:
 * @name: The secrets name
 * @key: The key that identifies it to the nm plugin
 * @value: The value (secret)
 * @is_pw: Whether this is a secret
 *
 * Used for secret transfer between #PhoshNetworkAuthManager and #PhoshNetworkAuthPrompt
 */
typedef struct phosh_nm_secret {
  char     *name;
  char     *key;
  char     *value;
  gboolean  is_pw;
} PhoshNMSecret;

#define PHOSH_TYPE_NETWORK_AUTH_PROMPT (phosh_network_auth_prompt_get_type())

G_DECLARE_FINAL_TYPE (PhoshNetworkAuthPrompt, phosh_network_auth_prompt, PHOSH, NETWORK_AUTH_PROMPT, PhoshSystemModalDialog);

GtkWidget *phosh_network_auth_prompt_new         (ShellNetworkAgent *agent);
gboolean   phosh_network_auth_prompt_set_request (PhoshNetworkAuthPrompt        *self,
                                                  char                          *request_id,
                                                  NMConnection                  *connection,
                                                  char                          *setting_name,
                                                  char                         **hints,
                                                  NMSecretAgentGetSecretsFlags   flags,
                                                  const char                    *title,
                                                  const char                    *message,
                                                  GPtrArray                     *secrets);
G_END_DECLS
