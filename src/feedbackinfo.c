/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-feedbackinfo"

#include "phosh-config.h"

#include "feedbackinfo.h"
#include "shell.h"

/**
 * PhoshFeedbackInfo:
 *
 * A widget to display feedback status
 */
enum {
  PROP_0,
  PROP_MUTED,
  PROP_PRESENT,
  PROP_ENABLED,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct _PhoshFeedbackInfo {
  PhoshStatusIcon parent;

  PhoshFeedbackManager *manager;
  gboolean              muted;
  gboolean              present;
  gboolean              enabled;
} PhoshFeedbackInfo;


G_DEFINE_TYPE (PhoshFeedbackInfo, phosh_feedback_info, PHOSH_TYPE_STATUS_ICON)

static void
phosh_feedback_info_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  PhoshFeedbackInfo *self = PHOSH_FEEDBACK_INFO (object);

  switch (prop_id) {
  case PROP_PRESENT:
    self->present = g_value_get_boolean (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
phosh_feedback_info_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  PhoshFeedbackInfo *self = PHOSH_FEEDBACK_INFO (object);

  switch (property_id) {
  case PROP_MUTED:
    g_value_set_boolean (value, self->muted);
    break;
  case PROP_PRESENT:
    g_value_set_boolean (value, self->present);
    break;
  case PROP_ENABLED:
    g_value_set_boolean (value, self->enabled);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
on_profile_changed (PhoshFeedbackInfo *self, GParamSpec *psepc, gpointer unused)
{
  const char *profile, *name;
  gboolean muted = FALSE;
  gboolean enabled = FALSE;

  g_return_if_fail (PHOSH_IS_FEEDBACK_INFO (self));

  profile = phosh_feedback_manager_get_profile (self->manager);
  if (!g_strcmp0 (profile, "quiet")) {
    /* Translators: quiet and silent are fbd profiles names:
       see https://source.puri.sm/Librem5/feedbackd#profiles
       for details */
    name = _("Quiet");
    muted = TRUE;
    enabled = TRUE;
  } else if (!g_strcmp0 (profile, "silent")) {
    /* Translators: quiet and silent are fbd profiles names:
       see https://source.puri.sm/Librem5/feedbackd#profiles
       for details */
    name = _("Silent");
    muted = TRUE;
  } else {
    /* Translators: Enable LED, haptic and audio feedback */
    name = C_("feedback:enabled", "On");
    enabled = TRUE;
  }

  phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), name);

  if (muted != self->muted) {
    self->muted = muted;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_MUTED]);
  }

  if (enabled != self->enabled) {
    self->enabled = enabled;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);
  }
}


static void
phosh_feedback_info_constructed (GObject *object)
{
  PhoshFeedbackInfo *self = PHOSH_FEEDBACK_INFO(object);
  PhoshShell *shell = phosh_shell_get_default ();

  G_OBJECT_CLASS (phosh_feedback_info_parent_class)->constructed (object);

  self->manager = g_object_ref(phosh_shell_get_feedback_manager (shell));

  g_signal_connect_swapped (self->manager,
                            "notify::profile",
                            G_CALLBACK(on_profile_changed),
                            self);
  on_profile_changed (self, NULL, NULL);
  g_object_bind_property (self->manager, "icon-name", self, "icon-name",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (self->manager, "present", self, "present",
                          G_BINDING_SYNC_CREATE);
}


static void
phosh_feedback_info_dispose (GObject *object)
{
  PhoshFeedbackInfo *self = PHOSH_FEEDBACK_INFO(object);

  if (self->manager) {
    g_signal_handlers_disconnect_by_data (self->manager, self);
    g_clear_object (&self->manager);
  }

  G_OBJECT_CLASS (phosh_feedback_info_parent_class)->dispose (object);
}


static void
phosh_feedback_info_class_init (PhoshFeedbackInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_feedback_info_constructed;
  object_class->dispose = phosh_feedback_info_dispose;
  object_class->set_property = phosh_feedback_info_set_property;
  object_class->get_property = phosh_feedback_info_get_property;

  /**
   * PhoshFeedbackInfo:muted:
   *
   * Whether audio is muted (this is true for the `quiet` and `silent`
   * profiles) but not for the `full` profile.
   */
  props[PROP_MUTED] =
    g_param_spec_boolean ("muted", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  /**
   * PhoshFeedbackinfo:present:
   *
   * Whether feedback manager is present
   */
  props[PROP_PRESENT] =
    g_param_spec_boolean ("present", "", "",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  /**
   * PhoshFeedbackInfo:enabled:
   *
   * Whether feedback is enabled. This is true for the `full` and `quiet` profile.
   */
  props[PROP_ENABLED] =
    g_param_spec_boolean ("enabled", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_css_name (widget_class, "phosh-feedback-info");
}


static void
phosh_feedback_info_init (PhoshFeedbackInfo *self)
{
}


GtkWidget *
phosh_feedback_info_new (void)
{
  return g_object_new (PHOSH_TYPE_FEEDBACK_INFO, NULL);
}
