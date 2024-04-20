/*
 * Copyright (C) 2019-2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#define G_LOG_DOMAIN "phosh-network-auth-manager"

#include "phosh-config.h"

#include "contrib/shell-network-agent.h"
#include "network-auth-manager.h"
#include "network-auth-prompt.h"
#include "shell.h"
#include "phosh-wayland.h"
#include "util.h"

#include <gmobile.h>

#include <NetworkManager.h>
#include <gio/gunixoutputstream.h>
#include <gio/gunixinputstream.h>

/**
 * PhoshNetworkAuthManager:
 *
 * Handles the interaction between networkmanager and the auth prompts
 *
 * Wi-Fi and other credentials are handled with #ShellNetworkAgent which implements
 * #NMSecretAgentOld to interface with NetworkManager. When a credential for a connection are requested,
 * a new #PhoshNetworkAuthPrompt is created, which asks the user various
 * credentials depending on the connection type and details (e.g. access point security method).
 *
 * For VPN prompts the plugins auth helper is being run and the the list of secrets to request
 * is fed to #PhoshNetworkAuthPrompt.
 *
 * TODO: Support more connection types
 */

struct _PhoshNetworkAuthManager {
  GObject                 parent;

  GCancellable           *cancel;
  GCancellable           *register_cancel;

  NMDeviceWifi           *dev;
  ShellNetworkAgent      *network_agent;
  PhoshNetworkAuthPrompt *network_prompt;
};
G_DEFINE_TYPE (PhoshNetworkAuthManager, phosh_network_auth_manager, G_TYPE_OBJECT);


static void
network_prompt_done_cb (PhoshNetworkAuthManager *self)
{
  g_return_if_fail (PHOSH_IS_NETWORK_AUTH_MANAGER (self));

  g_clear_pointer ((PhoshSystemModalDialog**)&self->network_prompt,
                   phosh_system_modal_dialog_close);
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


static PhoshNMSecret *
phosh_nm_secret_new (const char *name, const char* key, const char *value, gboolean is_pw)
{
  PhoshNMSecret *secret = g_new0 (PhoshNMSecret, 1);

  g_debug ("New VPN secret '%s/%s/%s/%d", name, key, value, is_pw);

  secret->name = g_strdup (name);
  secret->key = g_strdup (key);
  secret->value = g_strdup (value);
  secret->is_pw = is_pw;

  return secret;
}

static void
phosh_nm_secret_free (PhoshNMSecret *secret)
{
  g_free (secret->name);
  g_free (secret->key);
  g_free (secret->value);
}


typedef struct vpn_request {
  PhoshNetworkAuthManager     *self;
  char                        *request_id;
  NMConnection                *connection;
  char                        *setting_name;
  char                       **hints;
  NMSecretAgentGetSecretsFlags flags;
  GCancellable                *cancel;
} VPNRequest;


static void
vpn_request_free (VPNRequest *request)
{
  g_object_unref (request->self);
  g_free (request->request_id);
  g_object_unref (request->connection);
  g_free (request->setting_name);
  g_strfreev (request->hints);
  g_object_unref (request->cancel);
}

static void
vpn_request_error (VPNRequest *request, GError *error)
{
  g_warning ("Failed to get VPN secrets: %s", error->message);
  shell_network_agent_respond (request->self->network_agent, request->request_id,
                               SHELL_NETWORK_AGENT_INTERNAL_ERROR);
}

typedef struct {
  GPid           auth_helper_pid;
  GString       *auth_helper_response;
  VPNRequest    *request;
  GPtrArray     *secrets;
  GCancellable  *cancellable;
  gulong         cancellable_id;
  guint          child_watch_id;
  GInputStream  *input_stream;
  GOutputStream *output_stream;
  char           read_buf[5];
} AuthHelperData;


static void
auth_helper_data_free (AuthHelperData *data)
{
  g_clear_signal_handler (&data->cancellable_id, data->cancellable);
  g_clear_object (&data->cancellable);
  g_clear_handle_id (&data->child_watch_id, g_source_remove);
  g_spawn_close_pid (data->auth_helper_pid);
  g_string_free (data->auth_helper_response, TRUE);
  g_clear_object (&data->input_stream);
  g_clear_object (&data->output_stream);
  g_free (data);
}


static gboolean
str_to_bool (const char *str)
{
  g_autofree char *tmp = NULL;

  if (!str)
    return FALSE;

  tmp = g_ascii_strdown (str, -1);
  if (g_strcmp0 (tmp, "true") == 0 ||
      g_strcmp0 (tmp, "yes") == 0 ||
      g_strcmp0 (tmp, "1") == 0 ||
      g_strcmp0 (tmp, "on") == 0) {
    return TRUE;
  }
  return FALSE;
}


/* @phosh_clear_cancellable_disconnect:
 * @cancellable: a #GCancellable
 * @cancellable_id: An id retrieved when connecting to the @cancellable's `cancelled` signal
 *
 * If @cancellable_id is not 0, clear it and call g_cancellable_disconnect().
 * @cancellable may be %NULL, if there is nothing to disconnect.
 */
static inline gboolean
phosh_clear_g_cancellable_disconnect (GCancellable *cancellable, gulong *cancellable_id)
{
  gulong id;

  g_return_val_if_fail (G_IS_CANCELLABLE (cancellable), FALSE);

  id = *cancellable_id;
  if (cancellable_id && id != 0) {
    *cancellable_id = 0;
    g_cancellable_disconnect (cancellable, id);
    return TRUE;
  }
  return FALSE;
}

#define VPN_UI_GROUP "VPN Plugin UI"

static void
auth_helper_exited (GPid pid, int status, gpointer user_data)
{
  AuthHelperData * data        = user_data;
  VPNRequest * request         = data->request;
  NMSettingVpn * s_vpn         = nm_connection_get_setting_vpn (request->connection);

  g_autoptr (GKeyFile) keyfile  = NULL;
  g_autofree char * title       = NULL;
  g_autofree char * message     = NULL;
  g_auto (GStrv) groups         = NULL;

  g_autoptr (GError)    error   = NULL;
  g_autoptr (GPtrArray) secrets = NULL;

  data->child_watch_id = 0;
  g_debug ("Auth helper exited with: %d", status);

  phosh_clear_g_cancellable_disconnect (data->cancellable, &data->cancellable_id);

  if (status != 0) {
    g_set_error (&error,
                 NM_SECRET_AGENT_ERROR,
                 NM_SECRET_AGENT_ERROR_FAILED,
                 "Auth dialog failed with error code %d",
                 status);
    goto out;
  }

  keyfile = g_key_file_new ();
  if (!g_key_file_load_from_data (keyfile,
                                  data->auth_helper_response->str,
                                  data->auth_helper_response->len,
                                  G_KEY_FILE_NONE,
                                  &error)) {
    goto out;
  }

  groups = g_key_file_get_groups (keyfile, NULL);
  if (g_strcmp0 (groups[0], VPN_UI_GROUP)) {
    g_set_error (&error,
                 NM_SECRET_AGENT_ERROR,
                 NM_SECRET_AGENT_ERROR_FAILED,
                 "Expected [VPN Plugin UI] in auth dialog response");
    goto out;
  }

  title = g_key_file_get_string (keyfile, "VPN Plugin UI", "Title", &error);
  if (!title)
    goto out;

  message = g_key_file_get_string (keyfile, "VPN Plugin UI", "Description", &error);
  if (!message)
    goto out;

  secrets = g_ptr_array_new_with_free_func ((GDestroyNotify) phosh_nm_secret_free);
  for (int i = 1; groups[i]; i++) {
    g_autofree char *pretty_name = NULL;
    g_autofree char *value = NULL;
    PhoshNMSecret *secret;

    if (g_strcmp0 (groups[i], VPN_UI_GROUP) == 0)
      continue;

    value = g_key_file_get_string (keyfile, groups[i], "Value", NULL);
    if (!g_key_file_get_boolean (keyfile, groups[i], "IsSecret", NULL))
      continue;
    if (g_key_file_get_boolean (keyfile, groups[i], "ShouldAsk", NULL)) {
      pretty_name = g_key_file_get_string (keyfile, groups[i], "Label", NULL);
      secret = phosh_nm_secret_new (pretty_name,
                                    groups[i],
                                    nm_setting_vpn_get_service_type (s_vpn),
                                    g_key_file_get_boolean (keyfile, groups[i], "IsSecret", NULL));
      g_ptr_array_add (secrets, secret);
    } else {
      if (gm_str_is_null_or_empty (value))
        continue;

      shell_network_agent_add_vpn_secret (request->self->network_agent,
                                          request->request_id,
                                          groups[i],
                                          value);
    }
  }

out:
  if (error) {
    vpn_request_error (request, error);
  } else {
    if (secrets && secrets->len) {
      gboolean success;
      network_agent_setup_prompt (request->self);
      success = phosh_network_auth_prompt_set_request (request->self->network_prompt,
                                                       request->request_id,
                                                       request->connection,
                                                       request->setting_name,
                                                       request->hints,
                                                       request->flags,
                                                       title,
                                                       message,
                                                       secrets);
      if (!success) {
        /* TODO: queue request and process once prompt is done */
        g_warning ("Dropping request %s since prompt already busy", request->request_id);
        shell_network_agent_respond (request->self->network_agent,
                                     request->request_id,
                                     SHELL_NETWORK_AGENT_USER_CANCELED);
      }
    } else {
      g_debug ("Skipping VPN dialog for %s", request->request_id);
      shell_network_agent_respond (request->self->network_agent,
                                   request->request_id,
                                   SHELL_NETWORK_AGENT_CONFIRMED);
    }
  }
  auth_helper_data_free (data);
}

static void
_request_cancelled (GObject *object, gpointer user_data)
{
  auth_helper_data_free (user_data);
}

static void
auth_helper_read_done (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  GInputStream * auth_helper_out = G_INPUT_STREAM (source_object);
  AuthHelperData *data            = user_data;
  gssize read_size;

  g_autoptr (GError) error = NULL;

  read_size = g_input_stream_read_finish (auth_helper_out, res, &error);
  switch (read_size) {
  case -1:
    if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      vpn_request_error (data->request, error);
    auth_helper_data_free (data);
    break;
  case 0:
    /* Done reading. Let's wait for the auth dialog to exit so that we're able to collect the status.
     * Remember we can be cancelled in between. */
    data->child_watch_id = g_child_watch_add (data->auth_helper_pid, auth_helper_exited, data);
    data->cancellable    = g_object_ref (data->request->cancel);
    data->cancellable_id = g_cancellable_connect (data->cancellable,
                                                  G_CALLBACK (_request_cancelled), data, NULL);
    break;
  default:
    g_string_append_len (data->auth_helper_response, data->read_buf, read_size);
    g_input_stream_read_async (auth_helper_out,
                               data->read_buf,
                               sizeof(data->read_buf),
                               G_PRIORITY_DEFAULT,
                               NULL,
                               auth_helper_read_done,
                               data);
    return;
  }

  g_input_stream_close (auth_helper_out, NULL, NULL);
}

static void
auth_helper_write_done (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  GOutputStream *auth_helper_out = G_OUTPUT_STREAM (source_object);
  char *auth_helper_request_str = user_data;

  g_free (auth_helper_request_str);
  /* We don't care about write errors. If there are any problems, the
   * reader shall notice. */
  g_output_stream_write_finish (auth_helper_out, res, NULL);
  g_output_stream_close (auth_helper_out, NULL, NULL);
}

static void
add_to_string (GString *string, const char *key, const char *value)
{
  g_auto (GStrv)     lines = NULL;
  int i;

  lines = g_strsplit (value, "\n", -1);

  g_string_append (string, key);
  for (i = 0; lines[i]; i++) {
    g_string_append_c (string, '=');
    g_string_append (string, lines[i]);
    g_string_append_c (string, '\n');
  }
}

static void
add_data_item_to_string (const char *key, const char *value, gpointer user_data)
{
  GString *string = user_data;

  add_to_string (string, "DATA_KEY", key);
  add_to_string (string, "DATA_VAL", value);
  g_string_append_c (string, '\n');
}

static void
add_secret_to_string (const char *key, const char *value, gpointer user_data)
{
  GString *string = user_data;

  add_to_string (string, "SECRET_KEY", key);
  add_to_string (string, "SECRET_VAL", value);
  g_string_append_c (string, '\n');
}


static void
on_search_vpn_plugin_ready (GObject      *source_object,
                            GAsyncResult *res,
                            gpointer      user_data)
{
  VPNRequest *request = user_data;
  NMSettingVpn *vpn_setting;
  int auth_helper_stdin_fd, auth_helper_stdout_fd;
  GPid auth_helper_pid;
  const char *str;
  GOutputStream *auth_helper_stdin;
  GInputStream *auth_helper_stdout;

  g_autoptr (GPtrArray) auth_helper_argv = NULL;
  g_autoptr (NMVpnPluginInfo) plugin = NULL;
  g_autoptr (GError) err = NULL;
  const gchar *service_type;

  GString * auth_helper_request;
  g_autofree char * auth_helper_request_str = NULL;
  gsize auth_helper_request_len;
  AuthHelperData * data;

  vpn_setting = nm_connection_get_setting_vpn (request->connection);
  service_type = nm_setting_vpn_get_service_type (vpn_setting);

  plugin = shell_network_agent_search_vpn_plugin_finish (request->self->network_agent,
                                                         res,
                                                         &err);
  if (!plugin) {
    g_warning ("Failed to lookup VPN plugin for %s: %s", service_type, err->message);
    goto err;
  }

  str = nm_vpn_plugin_info_lookup_property (plugin, "GNOME", "supports-external-ui-mode");
  if (!str_to_bool (str)) {
    g_warning ("VPN auth plugin for %s does not support external ui mode", service_type);
    goto err;
  }

  str = nm_vpn_plugin_info_get_auth_dialog (plugin);
  if (!g_file_test (str, G_FILE_TEST_IS_EXECUTABLE)) {
    g_warning ("VPN auto plugin for %s (%s) not executable", service_type, str);
    goto err;
  }

  auth_helper_argv = g_ptr_array_new ();
  g_ptr_array_add (auth_helper_argv, (gpointer) str);
  g_ptr_array_add (auth_helper_argv, "-u");
  g_ptr_array_add (auth_helper_argv, (gpointer) nm_connection_get_uuid (request->connection));
  g_ptr_array_add (auth_helper_argv, "-n");
  g_ptr_array_add (auth_helper_argv, (gpointer) nm_connection_get_id (request->connection));
  g_ptr_array_add (auth_helper_argv, "-s");
  g_ptr_array_add (auth_helper_argv, (gpointer) nm_setting_vpn_get_service_type (vpn_setting));
  g_ptr_array_add (auth_helper_argv, "--external-ui-mode");
  if (request->flags & NM_SECRET_AGENT_GET_SECRETS_FLAG_ALLOW_INTERACTION)
    g_ptr_array_add (auth_helper_argv, "-i");
  if (request->flags & NM_SECRET_AGENT_GET_SECRETS_FLAG_REQUEST_NEW)
    g_ptr_array_add (auth_helper_argv, "-r");

  str = nm_vpn_plugin_info_lookup_property (plugin, "GNOME", "supports-hints");
  if (str_to_bool (str)) {
    for (int i = 0; request->hints[i]; i++) {
      g_ptr_array_add (auth_helper_argv, "-t");
      g_ptr_array_add (auth_helper_argv, request->hints[i]);
    }
  }

  g_ptr_array_add (auth_helper_argv, NULL);

  if (!g_spawn_async_with_pipes (NULL,
                                 (char **) auth_helper_argv->pdata,
                                 NULL, /* envp */
                                 G_SPAWN_DO_NOT_REAP_CHILD,
                                 NULL, /* setup-func */
                                 NULL, /* user_data */
                                 &auth_helper_pid,
                                 &auth_helper_stdin_fd,
                                 &auth_helper_stdout_fd,
                                 NULL,
                                 &err)) {
    const char *helper = ((char **)auth_helper_argv->pdata)[0];
    g_warning ("Failed to spawn auth helper %s: %s", helper, err->message);
    goto err;
  }

  auth_helper_stdin  = g_unix_output_stream_new (auth_helper_stdin_fd, TRUE);
  auth_helper_stdout = g_unix_input_stream_new (auth_helper_stdout_fd, TRUE);

  auth_helper_request = g_string_new_len (NULL, 1024);
  nm_setting_vpn_foreach_data_item (vpn_setting, add_data_item_to_string, auth_helper_request);
  nm_setting_vpn_foreach_secret (vpn_setting, add_secret_to_string, auth_helper_request);
  g_string_append (auth_helper_request, "DONE\nQUIT\n");
  auth_helper_request_len = auth_helper_request->len;
  auth_helper_request_str = g_string_free (auth_helper_request, FALSE);

  data  = g_new0 (AuthHelperData, 1);
  *data = (AuthHelperData){
    .auth_helper_response = g_string_new_len (NULL, sizeof(data->read_buf)),
    .auth_helper_pid      = auth_helper_pid,
    .request              = request,
    .secrets              = NULL, // g_ptr_array_ref (secrets),
    .input_stream         = auth_helper_stdout,
    .output_stream        = auth_helper_stdin,
  };

  g_output_stream_write_async (auth_helper_stdin,
                               auth_helper_request_str,
                               auth_helper_request_len,
                               G_PRIORITY_DEFAULT,
                               request->cancel,
                               auth_helper_write_done,
                               auth_helper_request_str);

  /* Ownership of the pointer was passed on to g_output_stream_write_async(). */
  g_steal_pointer (&auth_helper_request_str);

  g_input_stream_read_async (auth_helper_stdout,
                             data->read_buf,
                             sizeof(data->read_buf),
                             G_PRIORITY_DEFAULT,
                             request->cancel,
                             auth_helper_read_done,
                             data);
  return;
err:
  shell_network_agent_respond (request->self->network_agent, request->request_id,
                               SHELL_NETWORK_AGENT_INTERNAL_ERROR);
  vpn_request_free (request);
}


static void
vpn_secret_request (PhoshNetworkAuthManager     *self,
                    char                        *request_id,
                    NMConnection                *connection,
                    char                        *setting_name,
                    char                       **hints,
                    NMSecretAgentGetSecretsFlags flags)
{
  NMSettingVpn *setting = nm_connection_get_setting_vpn (connection);
  const gchar *service_type = nm_setting_vpn_get_service_type (setting);
  VPNRequest *request = g_new0 (VPNRequest, 1);

  g_debug ("Handling VPN secrets for %s, flags: 0x%x", service_type, flags);

  request->self = g_object_ref (self);
  request->request_id = g_strdup (request_id);
  request->connection = g_object_ref (connection);
  request->setting_name = g_strdup (setting_name);
  request->hints = g_strdupv (hints);
  request->flags = flags;
  request->cancel = g_cancellable_new ();

  shell_network_agent_search_vpn_plugin (self->network_agent, service_type,
                                         on_search_vpn_plugin_ready,
                                         request);
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
  gboolean ret;

  g_return_if_fail (PHOSH_IS_NETWORK_AUTH_MANAGER (self));

  g_debug ("Request %s: wants secrets for %s connection", request_id,
           nm_connection_get_connection_type (connection));

  if (nm_connection_is_type (connection, NM_SETTING_VPN_SETTING_NAME)) {
    vpn_secret_request (self, request_id, connection, setting_name, hints, flags);
    return;
  }
  if (!nm_connection_is_type (connection, NM_SETTING_WIRELESS_SETTING_NAME)) {
    g_warning ("%s secret handling currently not supported", nm_connection_get_connection_type (connection));
    shell_network_agent_respond (self->network_agent, request_id, SHELL_NETWORK_AGENT_USER_CANCELED);
    return;
  }

  g_return_if_fail (!self->network_prompt);

  network_agent_setup_prompt (self);
  ret = phosh_network_auth_prompt_set_request (self->network_prompt,
                                               request_id, connection, setting_name,
                                               hints, flags, NULL, NULL, NULL);
  if (!ret) {
    /* TODO: queue request and process once prompt is done */
    g_warning ("Dropping request %s since prompt already busy", request_id);
    shell_network_agent_respond (self->network_agent, request_id, SHELL_NETWORK_AGENT_USER_CANCELED);
  }
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
  PhoshNetworkAuthManager *self;
  NMSecretAgentOld *agent = NM_SECRET_AGENT_OLD (object);

  g_autoptr (GError) error = NULL;

  if (!nm_secret_agent_old_register_finish (agent, result, &error)) {
    g_message ("Error registering network agent: %s", error->message);
    if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      self = PHOSH_NETWORK_AUTH_MANAGER (user_data);
      g_clear_object (&self->register_cancel);
    }
    return;
  }

  self = PHOSH_NETWORK_AUTH_MANAGER (user_data);
  g_clear_object (&self->register_cancel);
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
    phosh_async_error_warn (err, "Failed to init network agent");
    return;
  }

  self = PHOSH_NETWORK_AUTH_MANAGER (user_data);
  g_return_if_fail (PHOSH_IS_NETWORK_AUTH_MANAGER (self));
  self->network_agent = SHELL_NETWORK_AGENT (nw_agent);
  self->register_cancel = g_cancellable_new ();
  nm_secret_agent_old_register_async (NM_SECRET_AGENT_OLD (self->network_agent), self->register_cancel,
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
                              "capabilities", NM_SECRET_AGENT_CAPABILITY_VPN_HINTS,
                              "identifier", "sm.puri.phosh.NetworkAgent",
                              "auto-register", FALSE,
                              NULL);
}


static void
phosh_network_auth_manager_constructed (GObject *object)
{
  PhoshNetworkAuthManager *self = PHOSH_NETWORK_AUTH_MANAGER (object);

  self->cancel = g_cancellable_new ();

  setup_network_agent (self);
  g_debug ("Network-auth-manager initialized");

  G_OBJECT_CLASS (phosh_network_auth_manager_parent_class)->constructed (object);
}


static void
phosh_network_auth_manager_dispose (GObject *object)
{
  PhoshNetworkAuthManager *self = PHOSH_NETWORK_AUTH_MANAGER (object);

  g_cancellable_cancel (self->register_cancel);
  g_clear_object (&self->register_cancel);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  g_clear_object (&self->network_agent);

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
