/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "testlib.h"

#pragma once

typedef struct _PhoshTestCompositorFixture {
  char                     *tmpdir;
  GTestDBus                *bus;
  PhoshTestCompositorState *state;
} PhoshTestCompositorFixture;


void phosh_test_compositor_setup    (PhoshTestCompositorFixture *fixture, gconstpointer unused);
void phosh_test_compositor_teardown (PhoshTestCompositorFixture *fixture, gconstpointer unused);

#define PHOSH_COMPOSITOR_TEST_ADD(name, func) g_test_add ((name), PhoshTestCompositorFixture, NULL, \
                                                          (gpointer)phosh_test_compositor_setup, \
                                                          (gpointer)(func), \
                                                          (gpointer)phosh_test_compositor_teardown)

