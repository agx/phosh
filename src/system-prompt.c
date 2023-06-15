/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Based on gnome-shell's shell-keyring-prompt.c
 * Author: Stef Walter <stefw@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-system-prompt"

#include "phosh-config.h"

#include "system-prompt.h"

#define GCR_API_SUBJECT_TO_CHANGE
#include <gcr/gcr.h>

#include <glib/gi18n.h>

/**
 * PhoshSystemPrompt:
 *
 * A modal system prompt
 *
 * The #PhoshSystemPrompt is used to ask for PINs and passwords
 */

enum {
  PROP_0,

  /* GcrPromptIface */
  PROP_MESSAGE,
  PROP_DESCRIPTION,
  PROP_WARNING,
  PROP_CHOICE_LABEL,
  PROP_CHOICE_CHOSEN,
  PROP_PASSWORD_NEW,
  PROP_PASSWORD_STRENGTH,
  PROP_CALLER_WINDOW,
  PROP_CONTINUE_LABEL,
  PROP_CANCEL_LABEL,

  /* our own */
  PROP_PASSWORD_VISIBLE,
  PROP_CONFIRM_VISIBLE,
  PROP_WARNING_VISIBLE,
  PROP_CHOICE_VISIBLE,
  PROP_LAST_PROP,
};

typedef enum
{
  PROMPTING_NONE,
  PROMPTING_FOR_CONFIRM,
  PROMPTING_FOR_PASSWORD
} PromptingMode;


typedef struct
{
  char *message;
  char *description;
  char *warning;
  char *choice_label;
  gboolean choice_chosen;
  gboolean password_new;
  guint password_strength;
  char *continue_label;
  char *cancel_label;

  GtkWidget *btn_cancel;
  GtkWidget *btn_continue;
  GtkWidget *checkbtn_choice;
  GtkWidget *entry_confirm;
  GtkWidget *entry_password;
  GtkWidget *grid;
  GtkWidget *lbl_choice;
  GtkWidget *lbl_confirm;
  GtkWidget *lbl_description;
  GtkWidget *lbl_password;
  GtkWidget *lbl_warning;
  GtkWidget *pbar_quality;

  GtkEntryBuffer *password_buffer;
  GtkEntryBuffer *confirm_buffer;

  GTask *task;
  GcrPromptReply last_reply;
  PromptingMode mode;
  gboolean shown;
} PhoshSystemPromptPrivate;


struct _PhoshSystemPrompt
{
  PhoshSystemModalDialog parent;
};


static void phosh_system_prompt_iface_init (GcrPromptIface *iface);
G_DEFINE_TYPE_WITH_CODE(PhoshSystemPrompt, phosh_system_prompt, PHOSH_TYPE_SYSTEM_MODAL_DIALOG,
                        G_IMPLEMENT_INTERFACE (GCR_TYPE_PROMPT,
                                               phosh_system_prompt_iface_init)
                        G_ADD_PRIVATE (PhoshSystemPrompt));


static void
phosh_system_prompt_set_property (GObject      *obj,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  PhoshSystemPrompt *self = PHOSH_SYSTEM_PROMPT (obj);
  PhoshSystemPromptPrivate *priv = phosh_system_prompt_get_instance_private (self);

  switch (prop_id) {
  case PROP_MESSAGE:
    g_free (priv->message);
    priv->message = g_value_dup_string (value);
    g_object_notify (obj, "message");
    break;
  case PROP_DESCRIPTION:
    g_free (priv->description);
    priv->description = g_value_dup_string (value);
    g_object_notify (obj, "description");
    break;
  case PROP_WARNING:
    g_free (priv->warning);
    priv->warning = g_value_dup_string (value);
    g_object_notify (obj, "warning");
    g_object_notify (obj, "warning-visible");
    break;
  case PROP_CHOICE_LABEL:
    g_free (priv->choice_label);
    priv->choice_label = g_value_dup_string (value);
    g_object_notify (obj, "choice-label");
    g_object_notify (obj, "choice-visible");
    break;
  case PROP_CHOICE_CHOSEN:
    priv->choice_chosen = g_value_get_boolean (value);
    g_object_notify (obj, "choice-chosen");
    break;
  case PROP_PASSWORD_NEW:
    priv->password_new = g_value_get_boolean (value);
    g_object_notify (obj, "password-new");
    g_object_notify (obj, "confirm-visible");
    break;
  case PROP_CALLER_WINDOW:
    /* ignored */
    break;
  case PROP_CONTINUE_LABEL:
    g_free (priv->continue_label);
    priv->continue_label = g_value_dup_string (value);
    g_object_notify (obj, "continue-label");
    break;
  case PROP_CANCEL_LABEL:
    g_free (priv->cancel_label);
    priv->cancel_label = g_value_dup_string (value);
    g_object_notify (obj, "cancel-label");
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    break;
  }
}


static void
phosh_system_prompt_get_property (GObject    *obj,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  PhoshSystemPrompt *self = PHOSH_SYSTEM_PROMPT (obj);
  PhoshSystemPromptPrivate *priv = phosh_system_prompt_get_instance_private (self);

  switch (prop_id) {
  case PROP_MESSAGE:
    g_value_set_string (value, priv->message ? priv->message : "");
    break;
  case PROP_DESCRIPTION:
    g_value_set_string (value, priv->description ? priv->description : "");
    break;
  case PROP_WARNING:
    g_value_set_string (value, priv->warning ? priv->warning : "");
    break;
  case PROP_CHOICE_LABEL:
    g_value_set_string (value, priv->choice_label ? priv->choice_label : "");
    break;
  case PROP_CHOICE_CHOSEN:
    g_value_set_boolean (value, priv->choice_chosen);
    break;
  case PROP_PASSWORD_NEW:
    g_value_set_boolean (value, priv->password_new);
    break;
  case PROP_PASSWORD_STRENGTH:
    g_value_set_int (value, priv->password_strength);
    break;
  case PROP_CALLER_WINDOW:
    g_value_set_string (value, "");
    break;
  case PROP_CONTINUE_LABEL:
    g_value_set_string (value, priv->continue_label);
    break;
  case PROP_CANCEL_LABEL:
    g_value_set_string (value, priv->cancel_label);
    break;
  case PROP_PASSWORD_VISIBLE:
    g_value_set_boolean (value, priv->mode == PROMPTING_FOR_PASSWORD);
    break;
  case PROP_CONFIRM_VISIBLE:
    g_value_set_boolean (value, priv->password_new &&
                         priv->mode == PROMPTING_FOR_CONFIRM);
    break;
  case PROP_WARNING_VISIBLE:
    g_value_set_boolean (value, priv->warning && priv->warning[0]);
    break;
  case PROP_CHOICE_VISIBLE:
    g_value_set_boolean (value, priv->choice_label && priv->choice_label[0]);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    break;
  }
}



static void
phosh_system_prompt_password_async (GcrPrompt *prompt,
                                    GCancellable *cancellable,
                                    GAsyncReadyCallback callback,
                                    gpointer user_data)
{
  PhoshSystemPrompt *self = PHOSH_SYSTEM_PROMPT (prompt);
  PhoshSystemPromptPrivate *priv = phosh_system_prompt_get_instance_private (self);
  GObject *obj;

  g_debug ("Starting system password prompt: %s", __func__);
  if (priv->task != NULL) {
    g_warning ("this prompt can only show one prompt at a time");
    return;
  }
  priv->mode = PROMPTING_FOR_PASSWORD;
  priv->task = g_task_new (self, NULL, callback, user_data);
  g_task_set_source_tag (priv->task, phosh_system_prompt_password_async);

  if (!gtk_entry_get_text_length (GTK_ENTRY (priv->entry_password)))
    gtk_widget_set_sensitive (priv->btn_continue, FALSE);
  gtk_widget_set_sensitive (priv->grid, TRUE);
  gtk_widget_grab_focus (priv->entry_password);

  obj = G_OBJECT (self);
  g_object_notify (obj, "password-visible");
  g_object_notify (obj, "confirm-visible");
  g_object_notify (obj, "warning-visible");
  g_object_notify (obj, "choice-visible");
  priv->shown = TRUE;
}


static const char *
phosh_system_prompt_password_finish (GcrPrompt *prompt,
                                     GAsyncResult *result,
                                     GError **error)
{
  g_return_val_if_fail (g_task_get_source_object (G_TASK (result)) == prompt, NULL);
  g_return_val_if_fail (g_async_result_is_tagged (result,
                                                  phosh_system_prompt_password_async), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}


static void
phosh_system_prompt_close (GcrPrompt *prompt)
{
  PhoshSystemPrompt *self = PHOSH_SYSTEM_PROMPT (prompt);
  PhoshSystemPromptPrivate *priv = phosh_system_prompt_get_instance_private (self);

  if (!priv->shown)
    return;

  priv->shown = FALSE;
  phosh_system_modal_dialog_close (PHOSH_SYSTEM_MODAL_DIALOG (self));
}


static void
phosh_system_prompt_confirm_async (GcrPrompt *prompt,
                                   GCancellable *cancellable,
                                   GAsyncReadyCallback callback,
                                   gpointer user_data)
{
  PhoshSystemPrompt *self = PHOSH_SYSTEM_PROMPT (prompt);
  PhoshSystemPromptPrivate *priv = phosh_system_prompt_get_instance_private (self);
  GObject *obj;

  g_debug ("Starting system confirmation prompt: %s", __func__);
  if (priv->task != NULL) {
    g_warning ("this prompt can only show one prompt at a time");
    return;
  }
  priv->mode = PROMPTING_FOR_CONFIRM;
  priv->task = g_task_new (self, NULL, callback, user_data);
  g_task_set_source_tag (priv->task, phosh_system_prompt_confirm_async);

  gtk_widget_set_sensitive (priv->btn_continue, TRUE);
  gtk_widget_set_sensitive (priv->grid, TRUE);
  gtk_widget_grab_focus (priv->entry_confirm);

  obj = G_OBJECT (self);
  g_object_notify (obj, "password-visible");
  g_object_notify (obj, "confirm-visible");
  g_object_notify (obj, "warning-visible");
  g_object_notify (obj, "choice-visible");
  priv->shown = TRUE;
}


static GcrPromptReply
phosh_system_prompt_confirm_finish (GcrPrompt *prompt,
                                    GAsyncResult *result,
                                    GError **error)
{
  GTask *task = G_TASK (result);
  gssize res;

  g_debug ("Finishing system confirmation prompt: %s", __func__);
  g_return_val_if_fail (g_task_get_source_object (task) == prompt,
                        GCR_PROMPT_REPLY_CANCEL);
  g_return_val_if_fail (g_async_result_is_tagged (result,
                                                  phosh_system_prompt_confirm_async), GCR_PROMPT_REPLY_CANCEL);

  res = g_task_propagate_int (task, error);
  return res == -1 ? GCR_PROMPT_REPLY_CANCEL : (GcrPromptReply)res;
}


static gboolean
prompt_complete (PhoshSystemPrompt *self)
{
  PhoshSystemPromptPrivate *priv = phosh_system_prompt_get_instance_private (self);
  GTask *res;
  PromptingMode mode;
  const char *password;
  const char *confirm;
  const char *env;

  g_return_val_if_fail (PHOSH_IS_SYSTEM_PROMPT (self), FALSE);
  g_return_val_if_fail (priv->mode != PROMPTING_NONE, FALSE);
  g_return_val_if_fail (priv->task != NULL, FALSE);

  password = gtk_entry_buffer_get_text (priv->password_buffer);

  if (priv->mode == PROMPTING_FOR_CONFIRM) {
    /* Is it a new password? */
    if (priv->password_new) {
      confirm = gtk_entry_buffer_get_text (priv->confirm_buffer);
      /* Do the passwords match? */
      if (!g_str_equal (password, confirm)) {
        gcr_prompt_set_warning (GCR_PROMPT (self), _("Passwords do not match."));
        return FALSE;
      }

      /* Don't allow blank passwords if in paranoid mode */
      env = g_getenv ("GNOME_KEYRING_PARANOID");
      if (env && *env) {
        gcr_prompt_set_warning (GCR_PROMPT (self), _("Password cannot be blank"));
        return FALSE;
      }
    }
  }

  res = priv->task;
  mode = priv->mode;
  priv->task = NULL;
  priv->mode = PROMPTING_NONE;

  if (mode == PROMPTING_FOR_CONFIRM)
    g_task_return_int (res, (gssize)GCR_PROMPT_REPLY_CONTINUE);
  else
    g_task_return_pointer (res, (gpointer)password, NULL);
  g_object_unref (res);

  gtk_widget_set_sensitive (priv->btn_continue, FALSE);
  gtk_widget_set_sensitive (priv->grid, FALSE);

  return TRUE;
}


static void
prompt_cancel (PhoshSystemPrompt *self)
{
  PhoshSystemPromptPrivate *priv;
  GTask *res;
  PromptingMode mode;

  g_return_if_fail (PHOSH_IS_SYSTEM_PROMPT (self));

  priv = phosh_system_prompt_get_instance_private (self);
  g_debug ("Canceling system password prompt for task %p", priv->task);

  /*
   * If canceled while not prompting, we should just close the prompt,
   * the user wants it to go away.
   */
  if (priv->mode == PROMPTING_NONE) {
    if (priv->shown)
      gcr_prompt_close (GCR_PROMPT (self));
    return;
  }

  g_return_if_fail (priv->task != NULL);

  res = priv->task;
  mode = priv->mode;
  priv->task = NULL;
  priv->mode = PROMPTING_NONE;

  if (mode == PROMPTING_FOR_CONFIRM)
    g_task_return_int (res, (gssize) GCR_PROMPT_REPLY_CANCEL);
  else
    g_task_return_pointer (res, NULL, NULL);
  g_object_unref (res);
}


static void
on_password_changed (PhoshSystemPrompt *self,
                     GtkEditable *editable)
{
  PhoshSystemPromptPrivate *priv;
  int upper, lower, digit, misc;
  const char *password;
  double pwstrength;
  int length, i;

  g_return_if_fail (PHOSH_IS_SYSTEM_PROMPT (self));
  g_return_if_fail (GTK_IS_EDITABLE (editable));

  priv = phosh_system_prompt_get_instance_private (self);

  if (!gtk_entry_get_text_length (GTK_ENTRY (editable)))
    return;

  gtk_widget_set_sensitive (priv->btn_continue, TRUE);
  password = gtk_entry_get_text (GTK_ENTRY (editable));

  /*
   * This code is based on the Master Password dialog in Firefox
   * (pref-masterpass.js)
   * Original code triple-licensed under the MPL, GPL, and LGPL
   * so is license-compatible with this file
   */

  length = strlen (password);
  upper = 0;
  lower = 0;
  digit = 0;
  misc = 0;

  for ( i = 0; i < length ; i++) {
    if (g_ascii_isdigit (password[i]))
      digit++;
    else if (g_ascii_islower (password[i]))
      lower++;
    else if (g_ascii_isupper (password[i]))
      upper++;
    else
      misc++;
  }

  if (length > 5)
    length = 5;
  if (digit > 3)
    digit = 3;
  if (upper > 3)
    upper = 3;
  if (misc > 3)
    misc = 3;

  pwstrength = ((length * 0.1) - 0.2) +
    (digit * 0.1) +
    (misc * 0.15) +
    (upper * 0.1);

  if (pwstrength < 0.0)
    pwstrength = 0.0;
  if (pwstrength > 1.0)
    pwstrength = 1.0;

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->pbar_quality), pwstrength);
}


static void
on_btn_continue_clicked (PhoshSystemPrompt *self, GtkButton *btn)
{
  prompt_complete (self);
}


static void
on_dialog_canceled (PhoshSystemPrompt *self)
{
  prompt_cancel (self);
}

static void
phosh_system_prompt_iface_init (GcrPromptIface *iface)
{
  iface->prompt_confirm_async = phosh_system_prompt_confirm_async;
  iface->prompt_confirm_finish = phosh_system_prompt_confirm_finish;
  iface->prompt_password_async = phosh_system_prompt_password_async;
  iface->prompt_password_finish = phosh_system_prompt_password_finish;
  iface->prompt_close = phosh_system_prompt_close;
}


static void
phosh_system_prompt_dispose (GObject *obj)
{
  PhoshSystemPrompt *self = PHOSH_SYSTEM_PROMPT (obj);
  PhoshSystemPromptPrivate *priv = phosh_system_prompt_get_instance_private (self);

  if (priv->shown)
    gcr_prompt_close (GCR_PROMPT (self));

  if (priv->task)
    prompt_cancel (self);

  g_assert (priv->task == NULL);
  G_OBJECT_CLASS (phosh_system_prompt_parent_class)->dispose (obj);
}


static void
phosh_system_prompt_finalize (GObject *obj)
{
  PhoshSystemPrompt *self = PHOSH_SYSTEM_PROMPT (obj);
  PhoshSystemPromptPrivate *priv = phosh_system_prompt_get_instance_private (self);

  g_free (priv->message);
  g_free (priv->description);
  g_free (priv->warning);
  g_free (priv->choice_label);
  g_free (priv->continue_label);
  g_free (priv->cancel_label);

  G_OBJECT_CLASS (phosh_system_prompt_parent_class)->finalize (obj);
}


static void
phosh_system_prompt_constructed (GObject *object)
{
  PhoshSystemPrompt *self = PHOSH_SYSTEM_PROMPT (object);
  PhoshSystemPromptPrivate *priv = phosh_system_prompt_get_instance_private (self);

  G_OBJECT_CLASS (phosh_system_prompt_parent_class)->constructed (object);

  g_object_bind_property (self, "message", self, "title", G_BINDING_DEFAULT);
  g_object_bind_property (self, "description", priv->lbl_description, "label", G_BINDING_DEFAULT);
  g_object_bind_property (self, "password-visible", priv->lbl_password, "visible", G_BINDING_DEFAULT);

  priv->password_buffer = gcr_secure_entry_buffer_new ();
  gtk_entry_set_buffer (GTK_ENTRY (priv->entry_password), GTK_ENTRY_BUFFER (priv->password_buffer));
  g_object_bind_property (self, "password-visible", priv->entry_password,
                          "visible", G_BINDING_DEFAULT);

  g_object_bind_property (self, "confirm-visible", priv->lbl_confirm, "visible", G_BINDING_DEFAULT);

  priv->confirm_buffer = gcr_secure_entry_buffer_new ();
  gtk_entry_set_buffer (GTK_ENTRY (priv->entry_confirm), GTK_ENTRY_BUFFER (priv->confirm_buffer));
  g_object_bind_property (self, "confirm-visible", priv->entry_confirm, "visible", G_BINDING_DEFAULT);

  g_object_bind_property (self, "confirm-visible", priv->pbar_quality, "visible", G_BINDING_DEFAULT);
  g_signal_connect_swapped (priv->entry_password, "changed",
                            G_CALLBACK (on_password_changed), self);

  g_object_bind_property (self, "warning", priv->lbl_warning, "label", G_BINDING_DEFAULT);
  g_object_bind_property (self, "warning-visible", priv->lbl_warning, "visible", G_BINDING_DEFAULT);

  g_object_bind_property (self, "choice-label", priv->lbl_choice,
                          "label", G_BINDING_DEFAULT);
  g_object_bind_property (self, "choice-visible", priv->lbl_choice,
                          "visible", G_BINDING_DEFAULT);
  g_object_bind_property (self, "choice-visible", priv->checkbtn_choice,
                          "visible", G_BINDING_DEFAULT);
  g_object_bind_property (self, "choice-chosen", priv->checkbtn_choice,
                          "active", G_BINDING_BIDIRECTIONAL);

  g_object_bind_property (self, "cancel-label", priv->btn_cancel, "label", G_BINDING_DEFAULT);
  g_object_bind_property (self, "continue-label", priv->btn_continue, "label", G_BINDING_DEFAULT);

  gtk_widget_grab_default (priv->btn_continue);
}


static void
phosh_system_prompt_class_init (PhoshSystemPromptClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_system_prompt_get_property;
  object_class->set_property = phosh_system_prompt_set_property;
  object_class->constructed = phosh_system_prompt_constructed;
  object_class->dispose = phosh_system_prompt_dispose;
  object_class->finalize = phosh_system_prompt_finalize;

  g_object_class_override_property (object_class, PROP_MESSAGE, "message");
  g_object_class_override_property (object_class, PROP_DESCRIPTION, "description");
  g_object_class_override_property (object_class, PROP_WARNING, "warning");
  g_object_class_override_property (object_class, PROP_PASSWORD_NEW, "password-new");
  g_object_class_override_property (object_class, PROP_PASSWORD_STRENGTH, "password-strength");
  g_object_class_override_property (object_class, PROP_CHOICE_LABEL, "choice-label");
  g_object_class_override_property (object_class, PROP_CHOICE_CHOSEN, "choice-chosen");
  g_object_class_override_property (object_class, PROP_CALLER_WINDOW, "caller-window");
  g_object_class_override_property (object_class, PROP_CONTINUE_LABEL, "continue-label");
  g_object_class_override_property (object_class, PROP_CANCEL_LABEL, "cancel-label");

  /**
   * GcrPromptDialog:password-visible:
   *
   * Whether the password entry is visible or not.
   */
  g_object_class_install_property (object_class, PROP_PASSWORD_VISIBLE,
                                   g_param_spec_boolean ("password-visible", "", "",
                                                         FALSE, G_PARAM_READABLE));

  /**
   * GcrPromptDialog:confirm-visible:
   *
   * Whether the password confirm entry is visible or not.
   */
  g_object_class_install_property (object_class, PROP_CONFIRM_VISIBLE,
                                   g_param_spec_boolean ("confirm-visible", "", "",
                                                         FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  /**
   * PhoshSystemPrompt:warning-visible:
   *
   * Whether the warning label is visible or not.
   */
  g_object_class_install_property (object_class, PROP_WARNING_VISIBLE,
                                   g_param_spec_boolean ("warning-visible", "", "",
                                                         FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  /**
   * PhoshSystemPrompt:choice-visible:
   *
   * Whether the choice check box is visible or not.
   */
  g_object_class_install_property (object_class, PROP_CHOICE_VISIBLE,
                                   g_param_spec_boolean ("choice-visible", "", "",
                                                         FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/system-prompt.ui");
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSystemPrompt, grid);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSystemPrompt, lbl_description);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSystemPrompt, lbl_password);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSystemPrompt, entry_password);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSystemPrompt, lbl_confirm);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSystemPrompt, entry_confirm);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSystemPrompt, pbar_quality);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSystemPrompt, lbl_warning);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSystemPrompt, checkbtn_choice);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSystemPrompt, lbl_choice);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSystemPrompt, btn_cancel);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSystemPrompt, btn_continue);
  gtk_widget_class_bind_template_callback (widget_class, on_dialog_canceled);
  gtk_widget_class_bind_template_callback (widget_class, on_btn_continue_clicked);
}


static void
phosh_system_prompt_init (PhoshSystemPrompt *self)
{
  /*
   * This is a stupid hack to help the window act like a normal object
   * with regards to reference counting and unref. (see gcr's
   * gcr_prompt_dialog_init ui/gcr-prompt-dialog.c as of
   * e0a506eeb29bc6be01a96e805e0244a03428ebf5.
   * Otherwise it gets clean up too early.
   */
  gtk_window_set_has_user_ref_count (GTK_WINDOW (self), FALSE);
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_system_prompt_new (void)
{
  return g_object_new (PHOSH_TYPE_SYSTEM_PROMPT, NULL);
}
