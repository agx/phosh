/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-app-auth-prompt"

#include "phosh-config.h"

#include "app-auth-prompt.h"
#include "auth-prompt-option.h"

#include <handy.h>
#include <glib/gi18n.h>

/**
 * PhoshAppAuthPrompt:
 *
 * A system modal prompt to authorize applications
 *
 * The #PhoshAppAuthPrompt is used to authorize applications. It's used
 * by the #PhoshLocationManager and for org.freedesktop.impl.Access.
 */

enum {
  CLOSED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


enum {
  PROP_0,

  PROP_ICON,
  PROP_SUBTITLE,
  PROP_BODY,
  PROP_DENY_LABEL,
  PROP_GRANT_LABEL,
  PROP_OFFER_REMEMBER,
  PROP_CHOICES,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];


typedef struct _PhoshAppAuthPrompt {
  PhoshSystemModalDialog parent;

  GIcon           *icon;
  char            *subtitle;
  char            *body;
  char            *deny_label;
  char            *grant_label;
  gboolean         offer_remember;
  GVariant        *choices;

  GtkWidget       *icon_app;
  GtkWidget       *lbl_subtitle;
  GtkWidget       *lbl_body;
  GtkWidget       *btn_grant;
  GtkWidget       *btn_deny;
  GtkWidget       *checkbtn_remember;
  GtkWidget       *list_box_choices;

  gboolean         grant_access;
  gboolean         remember;
} PhoshAppAuthPrompt;


G_DEFINE_TYPE (PhoshAppAuthPrompt, phosh_app_auth_prompt, PHOSH_TYPE_SYSTEM_MODAL_DIALOG)

#define CHOICE_FORMAT "(ssa(ss)s)"
#define OPTION_FORMAT "(ss)"

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
  case PROP_CHOICES:
    self->choices = g_value_dup_variant (value);
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
  case PROP_CHOICES:
    g_value_set_variant (value, self->choices);
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
  phosh_system_modal_dialog_close (PHOSH_SYSTEM_MODAL_DIALOG (self));
}


static void
on_dialog_canceled (PhoshAppAuthPrompt *self)
{
  g_return_if_fail (PHOSH_IS_APP_AUTH_PROMPT (self));
  self->remember = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->checkbtn_remember));

  g_signal_emit (self, signals[CLOSED], 0);
  phosh_system_modal_dialog_close (PHOSH_SYSTEM_MODAL_DIALOG (self));
}


static void
add_switch_option ( PhoshAppAuthPrompt *self,
                    gchar *choice_id,
                    gchar *choice_label,
                    char *default_option_id)
{
  GtkWidget *action_row_choice;
  GtkWidget *switch_choice;

  action_row_choice = g_object_new (HDY_TYPE_ACTION_ROW,
                                    "visible", TRUE,
                                    "title", choice_label,
                                    "activatable", TRUE,
                                    "selectable", FALSE,
                                    NULL);
  g_object_set_data_full (G_OBJECT (action_row_choice), "choice-id", g_strdup (choice_id), g_free);
  gtk_widget_set_visible (action_row_choice, TRUE);
  hdy_preferences_row_set_title (HDY_PREFERENCES_ROW (action_row_choice), choice_label);

  switch_choice = g_object_new (GTK_TYPE_SWITCH,
                                "visible", TRUE,
                                "halign", GTK_ALIGN_CENTER,
                                "valign", GTK_ALIGN_CENTER,
                                NULL);
  hdy_action_row_set_activatable_widget (HDY_ACTION_ROW (action_row_choice), switch_choice);
  gtk_container_add (GTK_CONTAINER (action_row_choice), switch_choice);

  gtk_container_add (GTK_CONTAINER (self->list_box_choices), action_row_choice);
}

static char *
get_choice_option_name (gpointer item, gpointer unused) {
  PhoshAuthPromptOption *option = PHOSH_AUTH_PROMPT_OPTION (item);
  return g_strdup (phosh_auth_prompt_option_get_label (option));
}

static void
add_combo_option (PhoshAppAuthPrompt *self,
                  char *choice_id,
                  char *choice_label,
                  GVariantIter *iter_options,
                  char *default_option_id)
{
  int index = 0;
  int selected_index = 0;
  char *option_id;
  char *option_label;
  GListStore *store;
  GtkWidget *combo_row_options;

  combo_row_options = hdy_combo_row_new ();
  g_object_set_data_full (G_OBJECT (combo_row_options), "choice-id", g_strdup (choice_id), g_free);
  gtk_widget_set_visible (combo_row_options, TRUE);
  hdy_preferences_row_set_title (HDY_PREFERENCES_ROW (combo_row_options), choice_label);
  gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (combo_row_options), TRUE);
  gtk_list_box_row_set_selectable (GTK_LIST_BOX_ROW (combo_row_options), FALSE);

  store = g_list_store_new (PHOSH_TYPE_AUTH_PROMPT_OPTION);
  while (g_variant_iter_loop (iter_options, OPTION_FORMAT, &option_id, &option_label)) {
    GObject *option;
    option = g_object_new (PHOSH_TYPE_AUTH_PROMPT_OPTION,
                          "id", option_id,
                          "label", option_label,
                          NULL);
    if (strcmp (option_id, default_option_id) == 0) {
      selected_index = index;
    }
    g_list_store_append(store, option);
    g_object_unref (option);
    index += 1;
  }
  hdy_combo_row_bind_name_model (HDY_COMBO_ROW (combo_row_options),
                                 G_LIST_MODEL (store),
                                 get_choice_option_name,
                                 NULL,
                                 NULL);
  hdy_combo_row_set_selected_index (HDY_COMBO_ROW (combo_row_options), selected_index);
  gtk_container_add (GTK_CONTAINER (self->list_box_choices), combo_row_options);
}


static void
add_choice_to_gvariant (GtkWidget *child, gpointer builder)
{
  GVariant *choice[2];

  choice[0] = g_variant_new_string (g_object_get_data (G_OBJECT (child), "choice-id"));
  if (HDY_IS_COMBO_ROW (child)) {
    HdyComboRow *row = HDY_COMBO_ROW (child);
    GListModel *model = hdy_combo_row_get_model (row);
    gint selected_index = hdy_combo_row_get_selected_index (row);
    PhoshAuthPromptOption *option = (PhoshAuthPromptOption*) g_list_model_get_item (model, selected_index);

    if (option != NULL) {
      choice[1] = g_variant_new_string (phosh_auth_prompt_option_get_id (option));
    }
  } else {
    HdyActionRow *row = HDY_ACTION_ROW (child);
    GtkSwitch *gtk_switch = GTK_SWITCH (hdy_action_row_get_activatable_widget (row));

    choice[1] = g_variant_new_string (gtk_switch_get_state(gtk_switch) ? "true" : "false");
  }
  g_variant_builder_add_value ((GVariantBuilder*) builder, g_variant_new_tuple (choice, 2));
}


static void
phosh_app_auth_prompt_finalize (GObject *obj)
{
  PhoshAppAuthPrompt *self = PHOSH_APP_AUTH_PROMPT (obj);

  g_clear_object (&self->icon);
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

  gtk_widget_grab_default (self->btn_grant);

  if (self->choices != NULL) {
    GVariantIter iter_choices;
    char *choice_id;
    char *choice_label;
    g_autoptr (GVariantIter) iter_options = NULL;
    char *default_option_id;

    g_variant_iter_init (&iter_choices, self->choices);

    if (g_variant_iter_n_children (&iter_choices) == 0) {
      gtk_widget_set_visible (self->list_box_choices, FALSE);
    }

    while (g_variant_iter_loop (&iter_choices, CHOICE_FORMAT,
                                &choice_id,
                                &choice_label,
                                &iter_options,
                                &default_option_id)) {
      if (g_variant_iter_n_children (iter_options) == 0) {
        add_switch_option (self, choice_id, choice_label, default_option_id);
      } else {
        add_combo_option (self, choice_id, choice_label, iter_options, default_option_id);
      }
    }
  }
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

  props[PROP_CHOICES] =
    g_param_spec_variant (
      "choices",
      "Choices",
      "The dialogs shown permissions and their possible values",
      G_VARIANT_TYPE (PHOSH_APP_AUTH_PROMPT_CHOICES_FORMAT),
      NULL,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  signals[CLOSED] = g_signal_new ("closed",
                                  G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                  NULL, G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/app-auth-prompt.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshAppAuthPrompt, icon_app);
  gtk_widget_class_bind_template_child (widget_class, PhoshAppAuthPrompt, lbl_subtitle);
  gtk_widget_class_bind_template_child (widget_class, PhoshAppAuthPrompt, lbl_body);
  gtk_widget_class_bind_template_child (widget_class, PhoshAppAuthPrompt, btn_grant);
  gtk_widget_class_bind_template_child (widget_class, PhoshAppAuthPrompt, btn_deny);
  gtk_widget_class_bind_template_child (widget_class, PhoshAppAuthPrompt, checkbtn_remember);
  gtk_widget_class_bind_template_child (widget_class, PhoshAppAuthPrompt, list_box_choices);
  gtk_widget_class_bind_template_callback (widget_class, on_btn_grant_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_dialog_canceled);
}


static void
phosh_app_auth_prompt_init (PhoshAppAuthPrompt *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_object_bind_property (self, "subtitle", self->lbl_subtitle, "label", G_BINDING_DEFAULT);
  g_object_bind_property (self, "body", self->lbl_body, "label", G_BINDING_DEFAULT);
  g_object_bind_property (self, "grant-label", self->btn_grant, "label", G_BINDING_DEFAULT);
  g_object_bind_property (self, "deny-label", self->btn_deny, "label", G_BINDING_DEFAULT);
  g_object_bind_property (self, "icon", self->icon_app, "gicon", G_BINDING_DEFAULT);
  g_object_bind_property (self, "offer-remember", self->checkbtn_remember, "visible", G_BINDING_DEFAULT);
}


GtkWidget *
phosh_app_auth_prompt_new (GIcon      *icon,
                           const char *title,
                           const char *subtitle,
                           const char *body,
                           const char *grant_label,
                           const char *deny_label,
                           gboolean    offer_remember,
                           GVariant   *choices)
{
  return g_object_new (PHOSH_TYPE_APP_AUTH_PROMPT,
                       "icon", icon,
                       "title", title,
                       "subtitle", subtitle,
                       "body", body,
                       "grant-label", grant_label,
                       "deny-label", deny_label,
                       "offer-remember", offer_remember,
                       "choices", choices,
                       NULL);
}

gboolean
phosh_app_auth_prompt_get_grant_access (GtkWidget *self)
{
  g_return_val_if_fail (PHOSH_IS_APP_AUTH_PROMPT (self), FALSE);

  return PHOSH_APP_AUTH_PROMPT (self)->grant_access;
}


GVariant* phosh_app_auth_prompt_get_selected_choices (GtkWidget *self)
{
  GVariantBuilder builder;

  g_return_val_if_fail (PHOSH_IS_APP_AUTH_PROMPT (self), NULL);
  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a(ss)"));
  gtk_container_foreach (
    GTK_CONTAINER ( PHOSH_APP_AUTH_PROMPT (self)->list_box_choices),
    add_choice_to_gvariant,
    &builder
  );
  return g_variant_builder_end (&builder);
}
