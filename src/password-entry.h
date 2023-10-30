/*
 * Copyright (C) 2023 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_PASSWORD_ENTRY (phosh_password_entry_get_type ())

G_DECLARE_FINAL_TYPE (PhoshPasswordEntry, phosh_password_entry, PHOSH, PASSWORD_ENTRY, GtkEntry)

PhoshPasswordEntry *phosh_password_entry_new (void);

G_END_DECLS