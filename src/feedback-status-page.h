/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "status-page.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_FEEDBACK_STATUS_PAGE (phosh_feedback_status_page_get_type ())

G_DECLARE_FINAL_TYPE (PhoshFeedbackStatusPage, phosh_feedback_status_page, PHOSH, FEEDBACK_STATUS_PAGE,
                      PhoshStatusPage)

PhoshFeedbackStatusPage *phosh_feedback_status_page_new (void);

G_END_DECLS
