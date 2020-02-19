/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#pragma once

#include <glib-object.h>

#define PHOSH_TYPE_FEEDBACK_MANAGER (phosh_feedback_manager_get_type())

G_DECLARE_FINAL_TYPE (PhoshFeedbackManager,
                      phosh_feedback_manager,
                      PHOSH,
                      FEEDBACK_MANAGER,
                      GObject)

PhoshFeedbackManager *phosh_feedback_manager_new (void);
void                  phosh_feedback_manager_toggle (PhoshFeedbackManager *self);
const gchar*          phosh_feedback_manager_get_icon_name (PhoshFeedbackManager *self);
const gchar*          phosh_feedback_manager_get_profile (PhoshFeedbackManager *self);
