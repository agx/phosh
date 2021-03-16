/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-app-auth-prompt"

#include "config.h"

#include "app-auth-prompt.h"

#include <glib/gi18n.h>

/**
 * SECTION:app-auth-prompt
 * @short_description: A system modal prompt to authorize applications
 * @Title: PhoshAppAuthPrompt
 *
 * The #PhoshAppAuthPrompt is used to authorize applications. It's used
 * by the #PhoshLocationManager and will later on be used for org.freedesktop.impl.Access.
 */

enum {
  CLOSED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


enum {
  PROP_0,

  PROP_ICON,
  PROP_TITLE,
  PROP_SUBTITLE,
  PROP_BODY,
  PROP_DENY_LABEL,
  PROP_GRANT_LABEL,
  PROP_OFFER_REMEMBER,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];


typedef struct _PhoshAppAuthPrompt {
  PhoshSystemModal parent;

  GIcon           *icon;
  char            *title;
  char            *subtitle;
  char            *body;
  char            *deny_label;
  char            *grant_label;
  gboolean         offer_remember;

  GtkWidget       *icon_app;
  GtkWidget       *lbl_title;
  GtkWidget       *lbl_subtitle;
  GtkWidget       *lbl_body;
  GtkWidget       *btn_grant;
  GtkWidget       *btn_deny;
  GtkWidget       *checkbtn_remember;

  gboolean         grant_access;
  gboolean         remember;
} PhoshAppAuthPrompt;


G_DEFINE_TYPE (PhoshAppAuthPrompt, phosh_app_auth_prompt, PHOSH_TYPE_SYSTEM_MODAL)


static void
phosh_app_auth_prompt_set_property (GObject      *obj,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PhoshAppAuthPrompt *self = PHOSH_APP_AUTH_PROMPT (obj);

  switch (prop_id) {
  case PROP_ICON:
    self->icon = g_value_dup_object (value);
    break;
  case PROP_TITLE:
    self->title = g_value_dup_string (value);
    break;
  case PROP_SUBTITLE:
    self->subtitle = g_value_dup_string (value);
    break;
  case PROP_BODY:
    self->body = g_value_dup_string (value);
    break;
  case PROP_GRANT_LABEL:
    self->grant_label = g_value_dup_string (value);
    break;
  case PROP_DENY_LABEL:
    self->deny_label = g_value_dup_string (value);
    break;
  case PROP_OFFER_REMEMBER:
    self->offer_remember = g_value_get_boolean (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    break;
  }
}


static void
phosh_app_auth_prompt_get_property (GObject    *obj,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  PhoshAppAuthPrompt *self = PHOSH_APP_AUTH_PROMPT (obj);

  switch (prop_id) {
  case PROP_ICON:
    g_value_set_object (value, self->icon);
    break;
  case PROP_TITLE:
    g_value_set_string (value, self->title);
    break;
  case PROP_SUBTITLE:
    g_value_set_string (value, self->subtitle);
    break;
  case PROP_BODY:
    g_value_set_string (value, self->body);
    break;
  case PROP_GRANT_LABEL:
    g_value_set_string (value, self->grant_label);
    break;
  case PROP_DENY_LABEL:
    g_value_set_string (value, self->deny_label);
    break;
  case PROP_OFFER_REMEMBER:
    g_value_set_boolean (value, self->offer_remember);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    break;
  }
}


static void
on_btn_grant_clicked (PhoshAppAuthPrompt *self, GtkButton *btn)
{
  self->grant_access = TRUE;
  self->remember = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->checkbtn_remember));

  g_signal_emit (self, signals[CLOSED], 0);
  gtk_widget_destroy (GTK_WIDGET (self));
}


static void
on_btn_deny_clicked (PhoshAppAuthPrompt *self, GtkButton *btn)
{
  self->grant_access = FALSE;
  self->remember = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->checkbtn_remember));

  g_signal_emit (self, signals[CLOSED], 0);
  gtk_widget_destroy (GTK_WIDGET (self));
}


static gboolean
on_key_press_event (PhoshAppAuthPrompt *self, GdkEventKey *event, gpointer data)
{
  gboolean handled = FALSE;

  g_return_val_if_fail (PHOSH_IS_APP_AUTH_PROMPT (self), FALSE);

  switch (event->keyval) {
  case GDK_KEY_Escape:
    on_btn_deny_clicked (self, NULL);
    handled = TRUE;
    break;
  default:
    /* nothing to do */
    break;
  }

  return handled;
}


static void
phosh_app_auth_prompt_finalize (GObject *obj)
{
  PhoshAppAuthPrompt *self = PHOSH_APP_AUTH_PROMPT (obj);

  g_clear_object (&self->icon);
  g_clear_pointer (&self->title, g_free);
  g_clear_pointer (&self->subtitle, g_free);
  g_clear_pointer (&self->body, g_free);
  g_clear_pointer (&self->grant_label, g_free);
  g_clear_pointer (&self->deny_label, g_free);

  G_OBJECT_CLASS (phosh_app_auth_prompt_parent_class)->finalize (obj);
}


static void
phosh_app_auth_prompt_constructed (GObject *object)
{
  PhoshAppAuthPrompt *self = PHOSH_APP_AUTH_PROMPT (object);

  G_OBJECT_CLASS (phosh_app_auth_prompt_parent_class)->constructed (object);

  g_object_bind_property (self, "title", self->lbl_title, "label", G_BINDING_DEFAULT);
  g_object_bind_property (self, "subtitle", self->lbl_subtitle, "label", G_BINDING_DEFAULT);
  g_object_bind_property (self, "body", self->lbl_body, "label", G_BINDING_DEFAULT);
  g_object_bind_property (self, "grant-label", self->btn_grant, "label", G_BINDING_DEFAULT);
  g_object_bind_property (self, "deny-label", self->btn_deny, "label", G_BINDING_DEFAULT);
  g_object_bind_property (self, "icon", self->icon_app, "gicon", G_BINDING_DEFAULT);
  g_object_bind_property (self, "offer-remember", self->checkbtn_remember, "visible", G_BINDING_DEFAULT);

  gtk_widget_grab_default (self->btn_grant);

  gtk_widget_add_events (GTK_WIDGET (self), GDK_KEY_PRESS_MASK);
  g_signal_connect (G_OBJECT (self),
                    "key_press_event",
                    G_CALLBACK (on_key_press_event),
                    NULL);
}


static void
phosh_app_auth_prompt_class_init (PhoshAppAuthPromptClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_app_auth_prompt_get_property;
  object_class->set_property = phosh_app_auth_prompt_set_property;
  object_class->constructed = phosh_app_auth_prompt_constructed;
  object_class->finalize = phosh_app_auth_prompt_finalize;

  props[PROP_ICON] =
    g_param_spec_object (
      "icon",
      "Icon",
      "The auth dialog icon",
      G_TYPE_ICON,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_TITLE] =
    g_param_spec_string (
      "title",
      "Title",
      "The auth dialog title",
      "",
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_SUBTITLE] =
    g_param_spec_string (
      "subtitle",
      "Subtitle",
      "The auth dialog subtitle",
      "",
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_BODY] =
    g_param_spec_string (
      "body",
      "Body",
      "The auth dialog body",
      "",
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_GRANT_LABEL] =
    g_param_spec_string (
      "grant-label",
      "Grant label",
      "The auth dialog's grant access button label",
      "",
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_DENY_LABEL] =
    g_param_spec_string (
      "deny-label",
      "Deny label",
      "The auth dialog's deny access button label",
      "",
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_OFFER_REMEMBER] =
    g_param_spec_boolean (
      "offer-remember",
      "Offer Remember",
      "Whether to offer to remember the auth decision result",
      FALSE,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  signals[CLOSED] = g_signal_new ("closed",
                                  G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                  NULL, G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/app-auth-prompt.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshAppAuthPrompt, icon_app);
  gtk_widget_class_bind_template_child (widget_class, PhoshAppAuthPrompt, lbl_title);
  gtk_widget_class_bind_template_child (widget_class, PhoshAppAuthPrompt, lbl_subtitle);
  gtk_widget_class_bind_template_child (widget_class, PhoshAppAuthPrompt, lbl_body);
  gtk_widget_class_bind_template_child (widget_class, PhoshAppAuthPrompt, btn_grant);
  gtk_widget_class_bind_template_child (widget_class, PhoshAppAuthPrompt, btn_deny);
  gtk_widget_class_bind_template_child (widget_class, PhoshAppAuthPrompt, checkbtn_remember);
  gtk_widget_class_bind_template_callback (widget_class, on_btn_grant_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_btn_deny_clicked);
}


static void
phosh_app_auth_prompt_init (PhoshAppAuthPrompt *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_app_auth_prompt_new (GIcon      *icon,
                           const char *title,
                           const char *subtitle,
                           const char *body,
                           const char *grant_label,
                           const char *deny_label,
                           gboolean    offer_remember)
{
  return g_object_new (PHOSH_TYPE_APP_AUTH_PROMPT,
                       "icon", icon,
                       "title", title,
                       "subtitle", subtitle,
                       "body", body,
                       "grant-label", grant_label,
                       "deny-label", deny_label,
                       "offer-remember", offer_remember,
                       NULL);
}

gboolean
phosh_app_auth_prompt_get_grant_access (GtkWidget *self)
{
  g_return_val_if_fail (PHOSH_IS_APP_AUTH_PROMPT (self), FALSE);

  return PHOSH_APP_AUTH_PROMPT (self)->grant_access;
}
