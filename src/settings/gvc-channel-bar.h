/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * based on gvc-channel-bar.h from g-c-c which is
 * Copyright (C) 2008 Red Hat, Inc.
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define GVC_TYPE_CHANNEL_BAR (gvc_channel_bar_get_type ())
G_DECLARE_FINAL_TYPE (GvcChannelBar, gvc_channel_bar, GVC, CHANNEL_BAR, GtkBox)

GtkWidget *         gvc_channel_bar_new                 (void);

void                gvc_channel_bar_set_name            (GvcChannelBar *bar,
                                                         const char    *name);
void                gvc_channel_bar_set_icon_name       (GvcChannelBar *bar,
                                                         const char    *icon_name);
void                gvc_channel_bar_set_low_icon_name   (GvcChannelBar *bar,
                                                         const char    *icon_name);
void                gvc_channel_bar_set_high_icon_name  (GvcChannelBar *bar,
                                                         const char    *icon_name);

void                gvc_channel_bar_set_orientation     (GvcChannelBar *bar,
                                                         GtkOrientation orientation);
GtkOrientation      gvc_channel_bar_get_orientation     (GvcChannelBar *bar);

GtkAdjustment *     gvc_channel_bar_get_adjustment      (GvcChannelBar *bar);

gboolean            gvc_channel_bar_get_is_muted        (GvcChannelBar *bar);
void                gvc_channel_bar_set_is_muted        (GvcChannelBar *bar,
                                                         gboolean       is_muted);
gboolean            gvc_channel_bar_get_show_mute       (GvcChannelBar *bar);
void                gvc_channel_bar_set_show_mute       (GvcChannelBar *bar,
                                                         gboolean       show_mute);
void                gvc_channel_bar_set_size_group      (GvcChannelBar *bar,
                                                         GtkSizeGroup  *group);
void                gvc_channel_bar_set_is_amplified    (GvcChannelBar *bar,
                                                         gboolean amplified);
void                gvc_channel_bar_set_base_volume     (GvcChannelBar *bar,
                                                         guint32        base_volume);
gboolean            gvc_channel_bar_get_ellipsize       (GvcChannelBar *bar);
void                gvc_channel_bar_set_ellipsize       (GvcChannelBar *bar,
                                                         gboolean       ellipsized);

gboolean            gvc_channel_bar_scroll              (GvcChannelBar  *bar,
                                                         GdkEventScroll *event);

double              gvc_channel_bar_get_volume          (GvcChannelBar *self);

G_END_DECLS
