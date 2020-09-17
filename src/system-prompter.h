/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#define GCR_API_SUBJECT_TO_CHANGE
#include <gcr/gcr-base.h>

GcrSystemPrompter *phosh_system_prompter_register(void);
void               phosh_system_prompter_unregister(void);
