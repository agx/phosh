/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include <gio/gio.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_SEARCH_APPLICATION phosh_search_application_get_type()
G_DECLARE_DERIVABLE_TYPE (PhoshSearchApplication, phosh_search_application, PHOSH, SEARCH_APPLICATION, GApplication)

struct _PhoshSearchApplicationClass
{
  GApplicationClass parent_class;
};

G_END_DECLS
