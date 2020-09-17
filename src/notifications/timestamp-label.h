/*
 * Copyright © 2020 Lugsole
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>


G_BEGIN_DECLS

#define PHOSH_TYPE_TIMESTAMP_LABEL (phosh_timestamp_label_get_type ())

G_DECLARE_FINAL_TYPE (PhoshTimestampLabel, phosh_timestamp_label, PHOSH, TIMESTAMP_LABEL, GtkLabel)

PhoshTimestampLabel  *phosh_timestamp_label_new            (void);
void                  phosh_timestamp_label_set_timestamp  (PhoshTimestampLabel *self,
                                                            GDateTime           *date);
GDateTime            *phosh_timestamp_label_get_timestamp  (PhoshTimestampLabel *self);

G_END_DECLS
