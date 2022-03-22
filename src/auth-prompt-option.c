/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Florian Loers <florianloers@mailbox.org>
 */

#include "auth-prompt-option.h"

enum {
  PROP_0,
  PROP_ID,
  PROP_LABEL,
  LAST_PROP,
};
static GParamSpec *props[LAST_PROP];

struct _PhoshAuthPromptOption {
  GObject parent;

  char *id;
  char *label;
};

G_DEFINE_TYPE (PhoshAuthPromptOption, phosh_auth_prompt_option, G_TYPE_OBJECT)

static void
phosh_auth_prompt_option_set_property (GObject *object,
                                       guint property_id,
                                       const GValue *value,
                                       GParamSpec *pspec)
{
  PhoshAuthPromptOption *obj = (PhoshAuthPromptOption *)object;

  switch (property_id) {
  case PROP_ID:
    g_free (obj->id);
    obj->id = g_value_dup_string (value);
    break;
  case PROP_LABEL:
    g_free (obj->label);
    obj->label = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
phosh_auth_prompt_option_get_property (GObject *object,
                                       guint property_id,
                                       GValue *value,
                                       GParamSpec *pspec)
{
  PhoshAuthPromptOption *obj = (PhoshAuthPromptOption *)object;

  switch (property_id) {
  case PROP_ID:
    g_value_set_string (value, obj->id);
    break;
  case PROP_LABEL:
    g_value_set_string (value, obj->label);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
phosh_auth_prompt_option_finalize (GObject *obj)
{
  PhoshAuthPromptOption *self = PHOSH_AUTH_PROMPT_OPTION (obj);

  g_free (self->label);
  g_free (self->id);

  G_OBJECT_CLASS (phosh_auth_prompt_option_parent_class)->finalize (obj);
}

static void
phosh_auth_prompt_option_class_init (PhoshAuthPromptOptionClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = phosh_auth_prompt_option_get_property;
  object_class->set_property = phosh_auth_prompt_option_set_property;
  object_class->finalize = phosh_auth_prompt_option_finalize;

  /**
   * PhoshAuthPromptOption:id
   *
   * The internal identifier of this PhoshAuthPromptOption.
   */
  props[PROP_ID] = g_param_spec_string ("id", "", "",
                                             NULL, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  /**
   * PhoshAuthPromptOption:label
   *
   * The visible label of this PhoshAuthPromptOption.
   */
  props[PROP_LABEL] = g_param_spec_string ("label", "", "",
                                                NULL, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
phosh_auth_prompt_option_init (PhoshAuthPromptOption *obj)
{
}

const char *
phosh_auth_prompt_option_get_id (PhoshAuthPromptOption *self)
{
  return self->id;
}

const char *
phosh_auth_prompt_option_get_label (PhoshAuthPromptOption *self)
{
  return self->label;
}
