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

typedef struct _PhoshAuth {
  GObject       parent;

  pam_handle_t *pamh;
} PhoshAuth;


G_DEFINE_TYPE (PhoshAuth, phosh_auth, G_TYPE_OBJECT)


static int
pam_conversation_cb (int                        num_msg,
                     const struct pam_message **msg,
                     struct pam_response      **resp,
                     void                      *appdata_ptr)
{
  const char *authtok = appdata_ptr;
  int ret = PAM_CONV_ERR;
  g_autofree struct pam_response *pam_resp = g_new0 (struct pam_response, num_msg);

  if (pam_resp == NULL)
    return PAM_BUF_ERR;

  for (int i = 0; i < num_msg; ++i) {
    switch (msg[i]->msg_style) {
    case PAM_PROMPT_ECHO_OFF:
    case PAM_PROMPT_ECHO_ON:
      pam_resp[i].resp = g_strdup (authtok);
      ret = PAM_SUCCESS;
      break;
    case PAM_ERROR_MSG: /* TBD */
    case PAM_TEXT_INFO: /* TBD */
    default:
      break;
    }
  }

  if (ret == PAM_SUCCESS)
    *resp = g_steal_pointer (&pam_resp);

  return ret;
}


/* return TRUE if auth token is correct, FALSE otherwise */
static gboolean
authenticate (PhoshAuth *self, const char *authtok)
{
  int ret;
  gboolean authenticated = FALSE;
  const char *username;
  const struct pam_conv conv = {
    .conv = pam_conversation_cb,
    .appdata_ptr = (void*)authtok,
  };

  if (self->pamh == NULL) {
    username = g_get_user_name ();
    ret = pam_start ("phosh", username, &conv, &self->pamh);
    if (ret != PAM_SUCCESS) {
      g_warning ("PAM start error %s", pam_strerror (self->pamh, ret));
      goto out;
    }
  }

  ret = pam_authenticate (self->pamh, 0);
  if (ret == PAM_SUCCESS) {
    authenticated = TRUE;
  } else {
    if (ret != PAM_AUTH_ERR)
      g_warning ("pam_authenticate error %s", pam_strerror (self->pamh, ret));
    goto out;
  }

  ret = pam_end (self->pamh, ret);
  if (ret != PAM_SUCCESS)
    g_warning ("pam_end error %d", ret);
  self->pamh = NULL;

 out:
  return authenticated;
}


static void
authenticate_thread (GTask        *task,
                     gpointer      source_object,
                     gpointer      task_data,
                     GCancellable *cancellable)
{
  PhoshAuth *self = PHOSH_AUTH (source_object);
  char *authtok = task_data;
  gboolean ret;

  if (task_data == NULL) {
    g_task_return_boolean (task, FALSE);
    return;
  }

  ret = authenticate (self, authtok);
  g_task_return_boolean (task, ret);
}


static void
phosh_auth_finalize (GObject *object)
{
  PhoshAuth *self = PHOSH_AUTH (object);
  GObjectClass *parent_class = G_OBJECT_CLASS (phosh_auth_parent_class);
  int ret;

  if (self->pamh) {
    ret = pam_end (self->pamh, PAM_AUTH_ERR);
    if (ret != PAM_SUCCESS)
      g_warning ("pam_end error %s", pam_strerror (self->pamh, ret));
    self->pamh = NULL;
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
                               const char          *authtok,
                               GCancellable        *cancellable,
                               GAsyncReadyCallback  callback,
                               gpointer             callback_data)
{
  g_autoptr (GTask) task = NULL;

  task = g_task_new (self, cancellable, callback, callback_data);
  g_task_set_task_data (task, g_strdup (authtok), g_free);

  g_task_run_in_thread (task, authenticate_thread);
}


gboolean
phosh_auth_authenticate_finish (PhoshAuth     *self,
                                GAsyncResult  *result,
                                GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);
  return g_task_propagate_boolean (G_TASK (result), error);
}
