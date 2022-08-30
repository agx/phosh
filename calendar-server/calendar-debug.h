/*
 * Copyright (C) 2004 Free Software Foundation, Inc.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later

 * Authors:
 *     Mark McLoughlin  <mark@skynet.ie>
 */

#ifndef __CALENDAR_DEBUG_H__
#define __CALENDAR_DEBUG_H__

#include <glib.h>

G_BEGIN_DECLS

#ifdef CALENDAR_ENABLE_DEBUG

#include <stdio.h>

#ifdef G_HAVE_ISO_VARARGS
#  define dprintf(...) fprintf (stderr, __VA_ARGS__);
#elif defined(G_HAVE_GNUC_VARARGS)
#  define dprintf(args...) fprintf (stderr, args);
#endif

#else /* if !defined (CALENDAR_DEBUG) */

#ifdef G_HAVE_ISO_VARARGS
#  define dprintf(...)
#elif defined(G_HAVE_GNUC_VARARGS)
#  define dprintf(args...)
#endif

#endif /* CALENDAR_ENABLE_DEBUG */

G_END_DECLS

#endif /* __CALENDAR_DEBUG_H__ */
