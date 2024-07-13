/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-auth"

#include "phosh-config.h"
#include "auth.h"

#include <security/pam_appl.h>

/**
 * PhoshAuth:
 *
 * PAM authentication handling
 */

typedef struct
{
  pam_handle_t *pamh;
} PhoshAuthPrivate;


typedef struct _PhoshAuth
{
  GObject parent;
} PhoshAuth;


G_DEFINE_TYPE_WITH_PRIVATE (PhoshAuth, phosh_auth, G_TYPE_OBJECT)


static int
pam_conversation_cb(int num_msg, const struct pam_message **msg,
                    struct pam_response **resp, void *data)
{
  const char *pin = data;
  int ret = PAM_CONV_ERR;
  struct pam_response *pam_resp = calloc(num_msg,
                                         sizeof(struct pam_response));

  if (pam_resp == NULL)
    return PAM_BUF_ERR;

  for (int i = 0; i < num_msg; ++i) {
    switch (msg[i]->msg_style) {
    case PAM_PROMPT_ECHO_OFF:
    case PAM_PROMPT_ECHO_ON:
      pam_resp[i].resp = g_strdup(pin);
      ret = PAM_SUCCESS;
      break;
    case PAM_ERROR_MSG: /* TBD */
    case PAM_TEXT_INFO: /* TBD */
    default:
      break;
    }
  }

  if (ret == PAM_SUCCESS)
    *resp = pam_resp;
  else
    free (pam_resp);
  return ret;
}


/* return TRUE if pin is correct, FALSE otherwise */
static gboolean
authenticate (PhoshAuth *self, const char *number)
{
  PhoshAuthPrivate *priv = phosh_auth_get_instance_private (self);
  int ret;
  gboolean authenticated = FALSE;
  const char *username;
  const struct pam_conv conv = {
    .conv = pam_conversation_cb,
    .appdata_ptr = (void*)number,
  };

  if (priv->pamh == NULL) {
    username = g_get_user_name ();
    ret = pam_start("phosh", username, &conv, &priv->pamh);
    if (ret != PAM_SUCCESS) {
      g_warning ("PAM start error %s", pam_strerror (priv->pamh, ret));
      goto out;
    }
  }

  ret = pam_authenticate(priv->pamh, 0);
  if (ret == PAM_SUCCESS) {
    authenticated = TRUE;
  } else {
    if (ret != PAM_AUTH_ERR)
      g_warning("pam_authenticate error %s", pam_strerror (priv->pamh, ret));
    goto out;
  }

  ret = pam_end(priv->pamh, ret);
  if (ret != PAM_SUCCESS)
    g_warning("pam_end error %d", ret);
  priv->pamh = NULL;

 out:
  return authenticated;
}


static void
authenticate_thread (GTask *task,
                    gpointer source_object,
                    gpointer task_data,
                    GCancellable *cancellable)
{
  PhoshAuth *self = PHOSH_AUTH (source_object);
  const char *number = task_data;
  gboolean ret;

  if (task_data == NULL) {
    g_task_return_boolean (task, FALSE);
    return;
  }

  ret = authenticate (self, number);
  g_task_return_boolean (task, ret);
}


static void
phosh_auth_finalize (GObject *object)
{
  PhoshAuthPrivate *priv = phosh_auth_get_instance_private (PHOSH_AUTH(object));
  GObjectClass *parent_class = G_OBJECT_CLASS (phosh_auth_parent_class);
  int ret;

  if (priv->pamh) {
    ret = pam_end(priv->pamh, PAM_AUTH_ERR);
    if (ret != PAM_SUCCESS)
      g_warning("pam_end error %s", pam_strerror (priv->pamh, ret));
    priv->pamh = NULL;
  }

  parent_class->finalize (object);
}


static void
phosh_auth_class_init (PhoshAuthClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phosh_auth_finalize;
}


static void
phosh_auth_init (PhoshAuth *self)
{
}


GObject *
phosh_auth_new (void)
{
  return g_object_new (PHOSH_TYPE_AUTH, NULL);
}


void
phosh_auth_authenticate_async (PhoshAuth           *self,
                               const char          *number,
                               GCancellable        *cancellable,
                               GAsyncReadyCallback  callback,
                               gpointer             callback_data)
{
  GTask *task;

  task = g_task_new (self, cancellable, callback, callback_data);
  g_task_set_task_data (task, (gpointer) number, NULL);
  g_task_run_in_thread (task, authenticate_thread);
  g_object_unref (task);
}


gboolean
phosh_auth_authenticate_finish (PhoshAuth     *self,
                                GAsyncResult  *result,
                                GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);
  return g_task_propagate_boolean (G_TASK (result), error);
}
