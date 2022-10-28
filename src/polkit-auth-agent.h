/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "phosh-config.h"

#define POLKIT_AGENT_I_KNOW_API_IS_SUBJECT_TO_CHANGE
#include <polkitagent/polkitagent.h>

#include <glib-object.h>

G_BEGIN_DECLS

#ifndef PHOSH_HAVE_POLKIT_SUBJECT_CLEANUP
G_DEFINE_AUTOPTR_CLEANUP_FUNC(PolkitSubject, g_object_unref)
#endif

#ifndef PHOSH_HAVE_POLKIT_AGENT_LISTENER_CLEANUP
G_DEFINE_AUTOPTR_CLEANUP_FUNC(PolkitAgentListener, g_object_unref)
#endif

#define PHOSH_TYPE_POLKIT_AUTH_AGENT (phosh_polkit_auth_agent_get_type())

G_DECLARE_FINAL_TYPE (PhoshPolkitAuthAgent, phosh_polkit_auth_agent, PHOSH, POLKIT_AUTH_AGENT, PolkitAgentListener)

PhoshPolkitAuthAgent * phosh_polkit_auth_agent_new           (void);
void                   phosh_polkit_authentication_agent_register (PhoshPolkitAuthAgent *agent,
                                                                   GError              **error);

G_END_DECLS
