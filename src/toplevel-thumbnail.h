/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include "wlr-screencopy-unstable-v1-client-protocol.h"
#include "thumbnail.h"
#include "toplevel.h"

#define PHOSH_TYPE_TOPLEVEL_THUMBNAIL (phosh_toplevel_thumbnail_get_type())

G_DECLARE_FINAL_TYPE (PhoshToplevelThumbnail,
                      phosh_toplevel_thumbnail,
                      PHOSH,
                      TOPLEVEL_THUMBNAIL,
                      PhoshThumbnail)

PhoshToplevelThumbnail *phosh_toplevel_thumbnail_new_from_toplevel (PhoshToplevel                   *toplevel,
                                                                    guint32                          max_width,
                                                                    guint32                          max_height);
