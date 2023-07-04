/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_AUDIO_SETTINGS (phosh_audio_settings_get_type ())

G_DECLARE_FINAL_TYPE (PhoshAudioSettings, phosh_audio_settings, PHOSH, AUDIO_SETTINGS, GtkBin)

PhoshAudioSettings *phosh_audio_settings_new                     (void);
gboolean            phosh_audio_settings_get_output_is_headphone (PhoshAudioSettings *self);
void                phosh_audio_settings_hide_details            (PhoshAudioSettings *self);

G_END_DECLS
