/*
 * Copyright (C) 2024 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gio/gio.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_BACKGROUND_IMAGE (phosh_background_image_get_type ())

G_DECLARE_FINAL_TYPE (PhoshBackgroundImage, phosh_background_image, PHOSH, BACKGROUND_IMAGE, GObject)

PhoshBackgroundImage     *phosh_background_image_new_sync               (GFile                *file,
                                                                         GCancellable         *cancellable,
                                                                         GError               **error);
void                      phosh_background_image_new                    (GFile                *file,
                                                                         GCancellable         *cancellable,
                                                                         GAsyncReadyCallback   callback,
                                                                         gpointer              user_data);
PhoshBackgroundImage     *phosh_background_image_new_finish             (GAsyncResult          *res,
                                                                         GError               **error);
GdkPixbuf                *phosh_background_image_get_pixbuf             (PhoshBackgroundImage *self);
GFile                    *phosh_background_image_get_file               (PhoshBackgroundImage *self);



G_END_DECLS
