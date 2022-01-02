/*
 * Copyright (C) 2019-2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#define G_LOG_DOMAIN "phosh-network-auth-manager"

#include "config.h"

#include "contrib/shell-network-agent.h"
#include "network-auth-manager.h"
#include "network-auth-prompt.h"
#include "shell.h"
#include "util.h"

#include <NetworkManager.h>

/**
 * SECTION:network-auth-manager
 * @short_description: Handles the interaction between networkmanager and the auth prompts
 * @Title: PhoshNetworkAuthManager
 *
 * Wi-Fi and other credentials are handled with #ShellNetworkAgent which implements
 * #NMSecretAgentOld to interface with NetworkManager. When a credential for a connection are requested,
 * a new #PhoshNetworkAuthPrompt is created, which asks the user various
 * credentials depending on the connection type and details (e.g. access point security method).
 */

struct _PhoshNetworkAuthManager {
  GObject                 parent;

  NMClient               *nmclient;
  GCancellable           *cancel;

  NMDeviceWifi           *dev;
  ShellNetworkAgent      *network_agent;
  PhoshNetworkAuthPrompt *network_prompt;
};
G_DEFINE_TYPE (PhoshNetworkAuthManager, phosh_network_auth_manager, G_TYPE_OBJECT);


static void
network_prompt_done_cb (PhoshNetworkAuthManager *self)
{
  g_return_if_fail (PHOSH_IS_NETWORK_AUTH_MANAGER (self));

  if (self->network_prompt)
    gtk_widget_hide (GTK_WIDGET (self->network_prompt));

  g_clear_pointer ((GtkWidget **)&self->network_prompt, gtk_widget_destroy);
}

static void
network_agent_setup_prompt (PhoshNetworkAuthManager *self)
{
  GtkWidget *network_prompt;

  g_return_if_fail (PHOSH_IS_NETWORK_AUTH_MANAGER (self));

  if (self->network_prompt)
    return;

  network_prompt = phosh_network_auth_prompt_new (self->network_agent);
  self->network_prompt = PHOSH_NETWORK_AUTH_PROMPT (network_prompt);

  g_signal_connect_object (self->network_prompt, "done",
                           G_CALLBACK (network_prompt_done_cb),
                           self, G_CONNECT_SWAPPED);

  /* Show widget when not locked and keep that in sync */
  g_object_bind_property (phosh_shell_get_default (), "locked",
                          self->network_prompt, "visible",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);
}


static void
secret_request_new_cb (PhoshNetworkAuthManager     *self,
                       char                        *request_id,
                       NMConnection                *connection,
                       char                        *setting_name,
                       char                       **hints,
                       NMSecretAgentGetSecretsFlags flags,
                       ShellNetworkAgent           *agent)
{
  g_return_if_fail (PHOSH_IS_NETWORK_AUTH_MANAGER (self));

  g_debug ("Request %s: wants secrets for %s connection", request_id,
           nm_connection_get_connection_type (connection));

  if (!nm_connection_is_type (connection, NM_SETTING_WIRELESS_SETTING_NAME)) {
    g_warning ("%s secret handling currently not supported", nm_connection_get_connection_type (connection));
    shell_network_agent_respond (self->network_agent, request_id, SHELL_NETWORK_AGENT_USER_CANCELED);
    return;
  }

  g_return_if_fail (!self->network_prompt);

  network_agent_setup_prompt (self);
  phosh_network_auth_prompt_set_request (self->network_prompt,
                                         request_id, connection, setting_name,
                                         hints, flags);

}


static void
secret_request_cancelled_cb (PhoshNetworkAuthManager *self,
                             char                    *request_id,
                             ShellNetworkAgent       *agent)
{
  g_return_if_fail (PHOSH_IS_NETWORK_AUTH_MANAGER (self));
  g_return_if_fail (SHELL_IS_NETWORK_AGENT (agent));

  network_prompt_done_cb (self);
}


static void
secret_agent_register_cb (GObject      *object,
                          GAsyncResult *result,
                          gpointer      user_data)
{
  PhoshNetworkAuthManager *self = user_data;
  NMSecretAgentOld *agent = NM_SECRET_AGENT_OLD (object);

  g_autoptr (GError) error = NULL;

  if (!nm_secret_agent_old_register_finish (agent, result, &error)) {
    g_message ("Error registering network agent: %s", error->message);
    return;
  }

  g_return_if_fail (PHOSH_IS_NETWORK_AUTH_MANAGER (self));

  g_signal_connect_object (self->network_agent, "new-request",
                           G_CALLBACK (secret_request_new_cb),
                           self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->network_agent, "cancel-request",
                           G_CALLBACK (secret_request_cancelled_cb),
                           self, G_CONNECT_SWAPPED);
}


static void
on_network_agent_ready (GObject      *source_object,
                        GAsyncResult *res,
                        gpointer      user_data)
{
  g_autoptr (GError) err = NULL;
  PhoshNetworkAuthManager *self;
  GObject *nw_agent;

  nw_agent = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object), res, &err);
  if (!nw_agent) {
    g_warning ("Failed to init network agent: %s", err->message);
    return;
  }

  self = PHOSH_NETWORK_AUTH_MANAGER (user_data);
  g_return_if_fail (PHOSH_IS_NETWORK_AUTH_MANAGER (self));
  self->network_agent = SHELL_NETWORK_AGENT (nw_agent);
  nm_secret_agent_old_register_async (NM_SECRET_AGENT_OLD (self->network_agent), NULL,
                                      secret_agent_register_cb, self);
}


static void
setup_network_agent (PhoshNetworkAuthManager *self)
{
  g_return_if_fail (PHOSH_IS_NETWORK_AUTH_MANAGER (self));

  g_async_initable_new_async (SHELL_TYPE_NETWORK_AGENT,
                              G_PRIORITY_DEFAULT,
                              self->cancel,
                              on_network_agent_ready,
                              self,
                              "identifier", "sm.puri.phosh.NetworkAgent",
                              "auto-register", FALSE,
                              NULL);
}


static void
on_nm_client_ready (GObject *obj, GAsyncResult *res, gpointer data)
{
  g_autoptr (GError) err = NULL;
  PhoshNetworkAuthManager *self;
  NMClient *client;

  client = nm_client_new_finish (res, &err);
  if (client == NULL) {
    g_message ("Failed to init NM: %s", err->message);
    return;
  }

  self = PHOSH_NETWORK_AUTH_MANAGER (data);
  self->nmclient = client;

  setup_network_agent (self);

  g_debug ("Network-auth-manager initialized");
}


static void
phosh_network_auth_manager_constructed (GObject *object)
{
  PhoshNetworkAuthManager *self = PHOSH_NETWORK_AUTH_MANAGER (object);

  self->cancel = g_cancellable_new ();
  nm_client_new_async (self->cancel, on_nm_client_ready, self);

  G_OBJECT_CLASS (phosh_network_auth_manager_parent_class)->constructed (object);
}


static void
phosh_network_auth_manager_dispose (GObject *object)
{
  PhoshNetworkAuthManager *self = PHOSH_NETWORK_AUTH_MANAGER (object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  g_clear_object (&self->network_agent);
  if (self->nmclient) {
    g_signal_handlers_disconnect_by_data (self->nmclient, self);
    g_clear_object (&self->nmclient);
  }

  G_OBJECT_CLASS (phosh_network_auth_manager_parent_class)->dispose (object);
}


static void
phosh_network_auth_manager_class_init (PhoshNetworkAuthManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_network_auth_manager_constructed;
  object_class->dispose = phosh_network_auth_manager_dispose;
}


static void
phosh_network_auth_manager_init (PhoshNetworkAuthManager *self)
{
}


PhoshNetworkAuthManager *
phosh_network_auth_manager_new (void)
{
  return PHOSH_NETWORK_AUTH_MANAGER (g_object_new (PHOSH_TYPE_NETWORK_AUTH_MANAGER, NULL));
}
