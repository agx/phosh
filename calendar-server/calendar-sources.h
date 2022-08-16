/*
 * Copyright (C) 2004 Free Software Foundation, Inc.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Authors:
 *     Mark McLoughlin  <mark@skynet.ie>
 *     William Jon McCann  <mccann@jhu.edu>
 *     Martin Grimme  <martin@pycage.de>
 *     Christian Kellner  <gicmo@xatom.net>
 */

#ifndef __CALENDAR_SOURCES_H__
#define __CALENDAR_SOURCES_H__

#include <glib-object.h>

#define EDS_DISABLE_DEPRECATED
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
#include <libedataserver/libedataserver.h>
#include <libecal/libecal.h>
G_GNUC_END_IGNORE_DEPRECATIONS

G_BEGIN_DECLS

#define CALENDAR_TYPE_SOURCES (calendar_sources_get_type ())
G_DECLARE_FINAL_TYPE (CalendarSources, calendar_sources,
                      CALENDAR, SOURCES, GObject)

CalendarSources *calendar_sources_get                (void);
ESourceRegistry *calendar_sources_get_registry       (CalendarSources *sources);
GSList          *calendar_sources_ref_clients        (CalendarSources *sources);
gboolean         calendar_sources_has_clients        (CalendarSources *sources);

void             calendar_sources_connect_client     (CalendarSources *sources,
                                                      ESource *source,
                                                      ECalClientSourceType source_type,
                                                      guint32 wait_for_connected_seconds,
                                                      GCancellable *cancellable,
                                                      GAsyncReadyCallback callback,
                                                      gpointer user_data);
EClient         *calendar_sources_connect_client_finish
                                                     (CalendarSources *sources,
                                                      GAsyncResult *result,
                                                      GError **error);

/* Set the environment variable CALENDAR_SERVER_DEBUG to show debug */
void            print_debug                          (const gchar *str,
                                                      ...) G_GNUC_PRINTF (1, 2);

G_END_DECLS

#endif /* __CALENDAR_SOURCES_H__ */
