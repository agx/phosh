/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include <glib-object.h>

void phosh_session_register (const char *client_id);
void phosh_session_unregister (void);
void phosh_session_shutdown (void);
