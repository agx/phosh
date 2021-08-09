/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Stubs so we don't need to run the shell */

#include "lockscreen-manager.h"

gboolean
phosh_lockscreen_manager_set_page  (PhoshLockscreenManager *self,
                                    PhoshLockscreenPage     page)
{
  return FALSE;
}
