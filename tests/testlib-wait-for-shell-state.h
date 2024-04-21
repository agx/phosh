/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "testlib.h"
#include "shell.h"

G_BEGIN_DECLS

typedef struct _PhoshTestWaitForShellState {
  GAsyncQueue         *queue;
  PhoshShell          *shell;
  PhoshShellStateFlags state;
  gboolean             state_enabled;
  gulong               signal_id;
} PhoshTestWaitForShellState;

PhoshTestWaitForShellState *phosh_test_wait_for_shell_state_new     (PhoshShell *shell);
void                        phosh_test_wait_for_shell_state_dispose (PhoshTestWaitForShellState *self);
void                        phosh_test_wait_for_shell_state_wait    (PhoshTestWaitForShellState *self,
                                                                     PhoshShellStateFlags state,
                                                                     gboolean enabled,
                                                                     guint64 timeout);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PhoshTestWaitForShellState, phosh_test_wait_for_shell_state_dispose)

G_END_DECLS
