/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-wall-clock.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_WALL_CLOCK phosh_wall_clock_get_type ()

G_DECLARE_DERIVABLE_TYPE (PhoshWallClock, phosh_wall_clock, PHOSH, WALL_CLOCK, GObject)


struct _PhoshWallClockClass
{
  GObjectClass      parent_class;

  const char *      (*get_clock)  (PhoshWallClock *self, gboolean time_only);
  gint64            (*get_time_t) (PhoshWallClock *self);
};


PhoshWallClock  *phosh_wall_clock_new                 (void);
void             phosh_wall_clock_set_default         (PhoshWallClock *self);
PhoshWallClock  *phosh_wall_clock_get_default         (void);
const char      *phosh_wall_clock_get_clock           (PhoshWallClock *self, gboolean time_only);
char            *phosh_wall_clock_local_date          (PhoshWallClock *self);
char            *phosh_wall_clock_string_for_datetime (PhoshWallClock      *self,
                                                       GDateTime           *datetime,
                                                       GDesktopClockFormat  clock_format,
                                                       gboolean             show_full_date);

G_END_DECLS
