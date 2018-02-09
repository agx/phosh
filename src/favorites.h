/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#ifndef __PHOSH_FAVORITES_H__
#define __PHOSH_FAVORITES_H__

#include <gtk/gtk.h>

#define PHOSH_FAVORITES_TYPE                 (phosh_favorites_get_type ())
#define PHOSH_FAVORITES(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHOSH_FAVORITES_TYPE, PhoshFavorites))
#define PHOSH_FAVORITES_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), PHOSH_FAVORITES_TYPE, PhoshFavoritesClass))
#define PHOSH_IS_FAVORITES(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHOSH_FAVORITES_TYPE))
#define PHOSH_IS_FAVORITES_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), PHOSH_FAVORITES_TYPE))
#define PHOSH_FAVORITES_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), PHOSH_FAVORITES_TYPE, PhoshFavoritesClass))

typedef struct PhoshFavorites PhoshFavorites;
typedef struct PhoshFavoritesClass PhoshFavoritesClass;
typedef struct PhoshFavoritesPrivate PhoshFavoritesPrivate;

struct PhoshFavorites
{
  GtkWindow parent;

  PhoshFavoritesPrivate *priv;
};

struct PhoshFavoritesClass
{
  GtkWindowClass parent_class;
};

GType phosh_favorites_get_type (void) G_GNUC_CONST;

GtkWidget * phosh_favorites_new (void);

#endif /* __PHOSH_FAVORITES_H__ */
