/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-feedbackinfo"

#include "config.h"

#include "feedbackinfo.h"
#include "shell.h"

/**
 * SECTION:feedbackinfo
 * @short_description: A widget to toggle feedback modes
 * @Title: PhoshFeedbackInfo
 */

typedef struct _PhoshFeedbackInfo {
  PhoshStatusIcon parent;

  PhoshFeedbackManager *manager;
} PhoshFeedbackInfo;


G_DEFINE_TYPE (PhoshFeedbackInfo, phosh_feedback_info, PHOSH_TYPE_STATUS_ICON)

static void
on_profile_changed (PhoshFeedbackInfo *self, GParamSpec *psepc, gpointer unused)
{
  const char *profile, *name;

  g_return_if_fail (PHOSH_IS_FEEDBACK_INFO (self));

  profile = phosh_feedback_manager_get_profile (self->manager);
  if (!g_strcmp0 (profile, "quiet")) {
    /* Translators: quiet and silent are fbd profiles names:
       see https://source.puri.sm/Librem5/feedbackd#profiles
       for details */
    name = _("Quiet");
  } else if (!g_strcmp0 (profile, "silent")) {
    /* Translators: quiet and silent are fbd profiles names:
       see https://source.puri.sm/Librem5/feedbackd#profiles
       for details */
    name = _("Silent");
  } else {
    name = _("On");
  }

  phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), name);
}

static void
phosh_feedback_info_constructed (GObject *object)
{
  PhoshFeedbackInfo *self = PHOSH_FEEDBACK_INFO(object);
  PhoshShell *shell = phosh_shell_get_default ();

  self->manager = g_object_ref(phosh_shell_get_feedback_manager (shell));

  g_signal_connect_swapped (self->manager,
                            "notify::profile",
                            G_CALLBACK(on_profile_changed),
                            self);
  on_profile_changed (self, NULL, NULL);
  g_object_bind_property (self->manager, "icon-name", self, "icon-name",
                          G_BINDING_SYNC_CREATE);

  G_OBJECT_CLASS (phosh_feedback_info_parent_class)->constructed (object);
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

  object_class->constructed = phosh_feedback_info_constructed;
  object_class->dispose = phosh_feedback_info_dispose;
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
