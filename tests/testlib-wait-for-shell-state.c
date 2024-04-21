/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "testlib-wait-for-shell-state.h"


static gboolean
check_shell_state (PhoshTestWaitForShellState *self)
{
  PhoshShellStateFlags flags;

  flags = phosh_shell_get_state (self->shell);

  return ( ( self->state_enabled &&  (flags & self->state)) ||
           (!self->state_enabled && !(flags & self->state)));
}


static void
on_shell_state (PhoshShell *shell, GParamSpec *pspec, PhoshTestWaitForShellState *self)
{
  g_assert_nonnull (self);

  if (check_shell_state (self)) {
    g_signal_handler_disconnect (self->shell, self->signal_id);
    g_async_queue_push (self->queue, (gpointer) TRUE);
  }
}


PhoshTestWaitForShellState*
phosh_test_wait_for_shell_state_new (PhoshShell *shell)
{
  PhoshTestWaitForShellState *self;

  self = g_new0 (PhoshTestWaitForShellState, 1);
  self->queue = g_async_queue_new ();
  self->shell = shell;

  return self;
}


void
phosh_test_wait_for_shell_state_dispose (PhoshTestWaitForShellState *self)
{
  g_assert_nonnull (self);

  g_clear_pointer (&self->queue, g_async_queue_unref);

  g_free (self);
}


static void
on_shell_idle (gpointer data)
{
  PhoshTestWaitForShellState *self;

  self = (PhoshTestWaitForShellState*) data;

  /* make sure the state we're checking for isn't already current.
   * otherwise, we'll wait for a state change that never comes */
  if (check_shell_state (self))
    g_async_queue_push (self->queue, (gpointer)TRUE);
  else
    self->signal_id = g_signal_connect (self->shell, "notify::shell-state",
                                        G_CALLBACK (on_shell_state), self);
}


void
phosh_test_wait_for_shell_state_wait (PhoshTestWaitForShellState *self, PhoshShellStateFlags state,
                                      gboolean enabled, guint64 timeout_ms)
{
  g_autofree gchar *str_state = NULL;

  self->state = state;
  self->state_enabled = enabled;

  /* schedule an idle callback in the shell (default) context. we can't access the shell state
   * or register signal handlers from here - we're in the wrong thread. */
  g_idle_add_once (on_shell_idle, self);

  if (g_async_queue_timeout_pop (self->queue, timeout_ms * 1000) == NULL) {
    str_state = g_flags_to_string (PHOSH_TYPE_SHELL_STATE_FLAGS, state);
    g_error ("Timed out waiting for %s to be %s", str_state, enabled ? "TRUE" : "FALSE");
  }
}
