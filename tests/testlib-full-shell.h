/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib.h"

G_BEGIN_DECLS

typedef struct _PhoshTestFullShellFixture {
  GThread                  *comp_and_shell;
  GAsyncQueue              *queue;
  PhoshTestCompositorState *state;
  GTestDBus                *bus;
  char                     *log_domains;
  char                     *tmpdir;
} PhoshTestFullShellFixture;


typedef struct _PhoshTestFullShellFixtureCfg {
  char *log_domains;
} PhoshTestFullShellFixtureCfg;


PhoshTestFullShellFixtureCfg *phosh_test_full_shell_fixture_cfg_new (const char *log_domains);
void phosh_test_full_shell_fixture_cfg_dispose (PhoshTestFullShellFixtureCfg *self);
void phosh_test_full_shell_setup (PhoshTestFullShellFixture *fixture, gconstpointer data);
void phosh_test_full_shell_teardown (PhoshTestFullShellFixture *fixture, gconstpointer unused);

#define PHOSH_FULL_SHELL_TEST_ADD(name, cfg, func) g_test_add ((name), \
                                                               PhoshTestFullShellFixture, (cfg), \
                                                          (gpointer)phosh_test_full_shell_setup, \
                                                          (gpointer)(func), \
                                                          (gpointer)phosh_test_full_shell_teardown)

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PhoshTestFullShellFixtureCfg, phosh_test_full_shell_fixture_cfg_dispose)

G_END_DECLS
