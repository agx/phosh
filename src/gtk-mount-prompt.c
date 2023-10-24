/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-gtk-mount-prompt"

#include "phosh-config.h"

#include "gtk-mount-prompt.h"

#include <glib/gi18n.h>

/**
 * PhoshGtkMountPrompt:
 *
 * A modal prompt for #PhoshGtkMountManager
 *
 * The #PhoshGtkMountPrompt is used to query the needed information
 * for the #PhoshGtkMountManager.
 */

enum {
  PROP_0,
  PROP_MESSAGE,
  PROP_ICON_NAME,
  PROP_DEFAULT_USER,
  PROP_DEFAULT_DOMAIN,
  PROP_PIDS,
  PROP_CHOICES,
  PROP_ASK_FLAGS,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];


enum {
  CLOSED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


struct _PhoshGtkMountPrompt {
  PhoshSystemModalDialog parent;

  GtkWidget             *lbl_msg;
  GtkWidget             *lbl_password;
  GtkWidget             *img_icon;
  GtkWidget             *lbl_user;
  GtkWidget             *entry_user;
  GtkWidget             *lbl_domain;
  GtkWidget             *entry_domain;
  GtkWidget             *entry_password;
  GtkEntryBuffer        *password_buffer;
  gboolean               pw_visible;

  char                  *message;
  char                  *icon_name;
  char                  *default_user;
  char                  *default_domain;
  GStrv                  choices;
  int                    choice;
  GVariant              *pids;
  GAskPasswordFlags      ask_flags;

  gboolean               cancelled;
};
G_DEFINE_TYPE (PhoshGtkMountPrompt, phosh_gtk_mount_prompt, PHOSH_TYPE_SYSTEM_MODAL_DIALOG);


static void G_GNUC_NULL_TERMINATED
set_visible (gboolean visible, ...)
{
  va_list args;
  GtkWidget *widget;

  va_start (args, visible);
  do {
    widget = va_arg (args, gpointer);
    if (widget == NULL)
      break;
    gtk_widget_set_visible (widget, visible);
  } while (TRUE);
  va_end (args);
}


static void
set_entries_visible (PhoshGtkMountPrompt *self, gboolean visible)
{
  set_visible (visible, self->lbl_password, self->entry_password, NULL);
  set_visible (visible, self->lbl_user, self->entry_user, NULL);
  set_visible (visible, self->lbl_domain, self->entry_domain, NULL);
}


static void
set_message (PhoshGtkMountPrompt *self, const char *message)
{
  g_autofree char *primary = NULL;
  char *secondary = NULL;

  g_return_if_fail (PHOSH_IS_GTK_MOUNT_PROMPT (self));

  if (!g_strcmp0 (self->message, message))
    return;

  g_clear_pointer (&self->message, g_free);
  self->message = g_strdup (message);

  primary = strstr (message, "\n");
  if (primary) {
    secondary = primary + 1;
    primary = g_strndup (message, primary - message);
  }

  phosh_system_modal_dialog_set_title (PHOSH_SYSTEM_MODAL_DIALOG (self), primary ?: message);
  if (secondary) {
    gtk_label_set_label (GTK_LABEL (self->lbl_msg), secondary);
    gtk_widget_show (self->lbl_msg);
  } else {
    gtk_widget_hide (self->lbl_msg);
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_MESSAGE]);
}


static void
set_icon_name (PhoshGtkMountPrompt *self, const char *icon_name)
{
  g_return_if_fail (PHOSH_IS_GTK_MOUNT_PROMPT (self));

  if (!g_strcmp0 (self->icon_name, icon_name))
    return;

  g_clear_pointer (&self->icon_name, g_free);
  self->icon_name = g_strdup (icon_name);

  gtk_image_set_from_icon_name (GTK_IMAGE (self->img_icon),
                                (icon_name && strlen (icon_name)) ?
                                self->icon_name : "dialog-password",
                                GTK_ICON_SIZE_DIALOG);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ICON_NAME]);
}


static void
set_default_user (PhoshGtkMountPrompt *self, const char *default_user)
{
  g_return_if_fail (PHOSH_IS_GTK_MOUNT_PROMPT (self));

  if (!g_strcmp0 (self->default_user, default_user))
    return;

  g_clear_pointer (&self->default_user, g_free);
  self->default_user = g_strdup (default_user);

  gtk_entry_set_text (GTK_ENTRY (self->entry_domain), self->default_user);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_DEFAULT_USER]);
}


static void
set_default_domain (PhoshGtkMountPrompt *self, const char *default_domain)
{
  g_return_if_fail (PHOSH_IS_GTK_MOUNT_PROMPT (self));

  if (!g_strcmp0 (self->default_domain, default_domain))
    return;

  g_clear_pointer (&self->default_domain, g_free);
  self->default_domain = g_strdup (default_domain);

  gtk_entry_set_text (GTK_ENTRY (self->entry_domain), self->default_domain);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_DEFAULT_DOMAIN]);
}


static void
on_button_clicked (PhoshGtkMountPrompt *self, GtkButton *btn)
{
  int num;

  g_return_if_fail (PHOSH_IS_GTK_MOUNT_PROMPT (self));
  g_return_if_fail (GTK_IS_BUTTON (btn));

  num = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "phosh-num"));
  self->choice = num;

  g_signal_emit (self, signals[CLOSED], 0);
}


static void
set_choices (PhoshGtkMountPrompt *self, GStrv choices)
{
  g_autoptr (GList) buttons = NULL;

  g_return_if_fail (PHOSH_IS_GTK_MOUNT_PROMPT (self));

  if (choices == NULL)
    return;

  g_clear_pointer (&self->choices, g_strfreev);
  self->choices = g_strdupv (choices);
  buttons = phosh_system_modal_dialog_get_buttons (PHOSH_SYSTEM_MODAL_DIALOG (self));
  for (GList *elem = buttons; elem; elem = elem->next)
    gtk_widget_destroy (GTK_WIDGET (elem->data));

  for (int i = 0; i < g_strv_length (self->choices); i++) {
    GtkWidget *btn = gtk_button_new_with_label (self->choices[i]);
    g_object_set_data (G_OBJECT (btn), "phosh-num", GINT_TO_POINTER (i));
    gtk_widget_show (GTK_WIDGET (btn));
    phosh_system_modal_dialog_add_button (PHOSH_SYSTEM_MODAL_DIALOG (self), btn, -1);
    g_signal_connect_swapped (btn, "clicked", G_CALLBACK (on_button_clicked), self);

    if (i == 0) {
      GtkStyleContext *context = gtk_widget_get_style_context (btn);
      gtk_style_context_add_class (context, "suggested-action");
      gtk_widget_grab_focus (btn);
    }
  }
  set_entries_visible (self, FALSE);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CHOICES]);
}


static void
set_ask_flags (PhoshGtkMountPrompt *self, GAskPasswordFlags flags)
{
  g_return_if_fail (PHOSH_IS_GTK_MOUNT_PROMPT (self));

  if (self->ask_flags == flags)
    return;

  self->ask_flags = flags;
  g_debug ("Flags 0x%.2x", flags);

  if (flags & G_ASK_PASSWORD_SAVING_SUPPORTED) {
    /* TODO */
  }

  set_visible (!!(flags & G_ASK_PASSWORD_NEED_PASSWORD),
               self->lbl_password, self->entry_password, NULL);

  set_visible (!!(flags & G_ASK_PASSWORD_NEED_USERNAME),
               self->lbl_user, self->entry_user, NULL);

  set_visible (!!(flags & G_ASK_PASSWORD_NEED_DOMAIN),
               self->lbl_domain, self->entry_domain, NULL);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ASK_FLAGS]);
}


static void
phosh_gtk_mount_prompt_set_property (GObject      *obj,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  PhoshGtkMountPrompt *self = PHOSH_GTK_MOUNT_PROMPT (obj);

  switch (prop_id) {
  case PROP_MESSAGE:
    set_message (self, g_value_get_string (value));
    break;
  case PROP_ICON_NAME:
    set_icon_name (self, g_value_get_string (value));
    break;
  case PROP_DEFAULT_USER:
    set_default_user (self, g_value_get_string (value));
    break;
  case PROP_DEFAULT_DOMAIN:
    set_default_domain (self, g_value_get_string (value));
    break;
  case PROP_CHOICES:
    set_choices (self, g_value_get_boxed (value));
    break;
  case PROP_PIDS:
    phosh_gtk_mount_prompt_set_pids (self, g_value_get_variant (value));
    break;
  case PROP_ASK_FLAGS:
    set_ask_flags (self, g_value_get_flags (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    break;
  }
}


static void
phosh_gtk_mount_prompt_get_property (GObject    *obj,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  PhoshGtkMountPrompt *self = PHOSH_GTK_MOUNT_PROMPT (obj);

  switch (prop_id) {
  case PROP_MESSAGE:
    g_value_set_string (value, self->message ?: "");
    break;
  case PROP_ICON_NAME:
    g_value_set_string (value, self->icon_name ?: "");
    break;
  case PROP_DEFAULT_USER:
    g_value_set_string (value, self->default_user ?: "");
    break;
  case PROP_DEFAULT_DOMAIN:
    g_value_set_string (value, self->default_domain ?: "");
    break;
  case PROP_CHOICES:
    g_value_set_boxed (value, self->choices);
    break;
  case PROP_PIDS:
    g_value_set_variant (value, self->pids);
    break;
  case PROP_ASK_FLAGS:
    g_value_set_enum (value, self->ask_flags);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    break;
  }
}


static void
on_connect_clicked (PhoshGtkMountPrompt *self)
{
  g_signal_emit (self, signals[CLOSED], 0);
}


static void
on_dialog_canceled (PhoshGtkMountPrompt *self)
{
  g_return_if_fail (PHOSH_IS_GTK_MOUNT_PROMPT (self));

  self->cancelled = TRUE;

  g_signal_emit (self, signals[CLOSED], 0);
}


static void
phosh_gtk_mount_prompt_finalize (GObject *obj)
{
  PhoshGtkMountPrompt *self = PHOSH_GTK_MOUNT_PROMPT (obj);

  g_free (self->message);
  g_free (self->icon_name);
  g_free (self->default_user);
  g_free (self->default_domain);
  g_clear_pointer (&self->choices, g_strfreev);
  g_clear_pointer (&self->pids, g_variant_unref);

  G_OBJECT_CLASS (phosh_gtk_mount_prompt_parent_class)->finalize (obj);
}


static void
phosh_gtk_mount_prompt_class_init (PhoshGtkMountPromptClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_gtk_mount_prompt_get_property;
  object_class->set_property = phosh_gtk_mount_prompt_set_property;
  object_class->finalize = phosh_gtk_mount_prompt_finalize;

  props[PROP_MESSAGE] =
    g_param_spec_string ("message",
                         "",
                         "",
                         "",
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_CONSTRUCT_ONLY);
  props[PROP_ICON_NAME] =
    g_param_spec_string ("icon-name",
                         "",
                         "",
                         "",
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_CONSTRUCT_ONLY);
  props[PROP_DEFAULT_USER] =
    g_param_spec_string ("default-user",
                         "",
                         "",
                         "",
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_CONSTRUCT_ONLY);
  props[PROP_DEFAULT_DOMAIN] =
    g_param_spec_string ("default-domain",
                         "",
                         "",
                         "",
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_CONSTRUCT_ONLY);
  props[PROP_CHOICES] =
    g_param_spec_boxed ("choices",
                        "",
                        "",
                        G_TYPE_STRV,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_CONSTRUCT_ONLY);
  props[PROP_PIDS] =
    g_param_spec_variant ("pids",
                          "",
                          "",
                          G_VARIANT_TYPE_ARRAY,
                          NULL,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  props[PROP_ASK_FLAGS] =
    g_param_spec_flags ("ask-flags",
                        "",
                        "",
                        G_TYPE_ASK_PASSWORD_FLAGS,
                        0,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  signals[CLOSED] = g_signal_new ("closed",
                                  G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                  NULL, G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/gtk-mount-prompt.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshGtkMountPrompt, lbl_msg);
  gtk_widget_class_bind_template_child (widget_class, PhoshGtkMountPrompt, lbl_password);
  gtk_widget_class_bind_template_child (widget_class, PhoshGtkMountPrompt, lbl_user);
  gtk_widget_class_bind_template_child (widget_class, PhoshGtkMountPrompt, lbl_domain);
  gtk_widget_class_bind_template_child (widget_class, PhoshGtkMountPrompt, img_icon);
  gtk_widget_class_bind_template_child (widget_class, PhoshGtkMountPrompt, entry_password);
  gtk_widget_class_bind_template_child (widget_class, PhoshGtkMountPrompt, entry_user);
  gtk_widget_class_bind_template_child (widget_class, PhoshGtkMountPrompt, entry_domain);
  gtk_widget_class_bind_template_child (widget_class, PhoshGtkMountPrompt, password_buffer);
  gtk_widget_class_bind_template_callback (widget_class, on_connect_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_dialog_canceled);

  gtk_widget_class_set_css_name (widget_class, "phosh-gtk-mount-prompt");
}


static void
phosh_gtk_mount_prompt_init (PhoshGtkMountPrompt *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_gtk_mount_prompt_new (const char        *message,
                            const char        *icon_name,
                            const char        *default_user,
                            const char        *default_domain,
                            GVariant          *pids,
                            const char *const *choices,
                            GAskPasswordFlags  ask_flags)
{
  return g_object_new (PHOSH_TYPE_GTK_MOUNT_PROMPT,
                       "message", message,
                       "icon-name", icon_name,
                       "ask-flags", ask_flags,
                       "default-user", default_user,
                       "default-domain", default_domain,
                       "pids", pids,
                       "choices", choices,
                       NULL);
}


const char *
phosh_gtk_mount_prompt_get_password (PhoshGtkMountPrompt *self)
{
  g_return_val_if_fail (PHOSH_IS_GTK_MOUNT_PROMPT (self), NULL);

  return gtk_entry_buffer_get_text (self->password_buffer);
}


GAskPasswordFlags
phosh_gtk_mount_prompt_get_ask_flags (PhoshGtkMountPrompt *self)
{
  g_return_val_if_fail (PHOSH_IS_GTK_MOUNT_PROMPT (self), 0);

  return self->ask_flags;
}


/**
 * phosh_gtk_mount_prompt_get_cancelled:
 * @self: The #PhoshGtkMountPrompt
 *
 * Returns: %TRUE if the dialog was cancelled (e.g. via swipe
 * away or pressing the Esc-key.
 */
gboolean
phosh_gtk_mount_prompt_get_cancelled (PhoshGtkMountPrompt *self)
{
  g_return_val_if_fail (PHOSH_IS_GTK_MOUNT_PROMPT (self), FALSE);

  return self->cancelled;
}


/**
 * phosh_gtk_mount_prompt_get_choices:
 * @self: The #PhoshGtkMountPrompt
 *
 * Returns: (transfer none): The prompt's choices
 */
GStrv
phosh_gtk_mount_prompt_get_choices (PhoshGtkMountPrompt *self)
{
  g_return_val_if_fail (PHOSH_IS_GTK_MOUNT_PROMPT (self), NULL);

  return self->choices;
}


int
phosh_gtk_mount_prompt_get_choice (PhoshGtkMountPrompt *self)
{
  g_return_val_if_fail (PHOSH_IS_GTK_MOUNT_PROMPT (self), -1);

  return self->choice;
}


void
phosh_gtk_mount_prompt_set_pids (PhoshGtkMountPrompt *self, GVariant *pids)
{
  g_return_if_fail (PHOSH_IS_GTK_MOUNT_PROMPT (self));

  if (self->pids == pids)
    return;

  g_clear_pointer (&self->pids, g_variant_unref);

  if (pids)
    self->pids = g_variant_ref (pids);

  /* TODO: update ui */

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PIDS]);
}
