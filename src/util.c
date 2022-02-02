/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "util.h"
#include <gtk/gtk.h>

#include <systemd/sd-login.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>


/* Just wraps gtk_widget_destroy so we can use it with g_clear_pointer */
void
phosh_cp_widget_destroy (void *widget)
{
  gtk_widget_destroy (GTK_WIDGET (widget));
}


/**
 * phosh_get_desktop_app_info_for_app_id:
 * @app_id: the app_id
 *
 * Looks up an app info object for specified application ID.
 * Tries a bunch of transformations in order to maximize compatibility
 * with X11 and non-GTK applications that may not report the exact same
 * string as their app-id and in their desktop file.
 *
 * Returns: (transfer full): GDesktopAppInfo for requested app_id
 */
GDesktopAppInfo *
phosh_get_desktop_app_info_for_app_id (const char *app_id)
{
  g_autofree char *desktop_id = NULL;
  g_autofree char *lowercase = NULL;
  GDesktopAppInfo *app_info = NULL;
  char *last_component;
  static char *mappings[][2] = {
    { "org.gnome.ControlCenter", "gnome-control-center" },
    { "gnome-usage", "org.gnome.Usage" },
  };

  g_assert (app_id);

  /* fix up applications with known broken app-id */
  for (int i = 0; i < G_N_ELEMENTS (mappings); i++) {
    if (strcmp (app_id, mappings[i][0]) == 0) {
      app_id = mappings[i][1];
      break;
    }
  }

  desktop_id = g_strdup_printf ("%s.desktop", app_id);
  g_return_val_if_fail (desktop_id, NULL);
  app_info = g_desktop_app_info_new (desktop_id);

  if (app_info)
    return app_info;

  /* try to handle the case where app-id is rev-DNS, but desktop file is not */
  last_component = strrchr(app_id, '.');
  if (last_component) {
    g_free (desktop_id);
    desktop_id = g_strdup_printf ("%s.desktop", last_component + 1);
    g_return_val_if_fail (desktop_id, NULL);
    app_info = g_desktop_app_info_new (desktop_id);
    if (app_info)
      return app_info;
  }

  /* X11 WM_CLASS is often capitalized, so try in lowercase as well */
  lowercase = g_utf8_strdown (last_component ?: app_id, -1);
  g_free (desktop_id);
  desktop_id = g_strdup_printf ("%s.desktop", lowercase);
  g_return_val_if_fail (desktop_id, NULL);
  app_info = g_desktop_app_info_new (desktop_id);

  if (!app_info)
    g_message ("Could not find application for app-id '%s'", app_id);

  return app_info;
}

/**
 * phosh_munge_app_id:
 * @app_id: the app_id
 *
 * Munges an app_id according to the rules used by
 * gnome-shell, feedbackd and phoc for gsettings:
 *
 * Returns: The munged_app id
 */
char *
phosh_munge_app_id (const char *app_id)
{
  char *id = g_strdup (app_id);
  int i;

  if (g_str_has_suffix (id, ".desktop")) {
    char *c = g_strrstr (id, ".desktop");
    if (c)
      *c = '\0';
  }

  g_strcanon (id,
              "0123456789"
              "abcdefghijklmnopqrstuvwxyz"
              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
              "-",
              '-');
  for (i = 0; id[i] != '\0'; i++)
    id[i] = g_ascii_tolower (id[i]);

  return id;
}


/**
 * phosh_strip_suffix_from_app_id:
 * @app_id: the app_id
 *
 * Strip the desktop suffix from app_id.
 *
 * Returns: (transfer full): The munged_app id
 */
char *
phosh_strip_suffix_from_app_id (const char *app_id)
{
  char *new_id = g_strdup (app_id);

  if (new_id && g_str_has_suffix (app_id, ".desktop")) {
    *(new_id + strlen (new_id) - 8 /* strlen (".desktop") */) = '\0';
  }

  return new_id;
}


gboolean
phosh_find_systemd_session (char **session_id)
{
  int n_sessions;

  g_auto (GStrv) sessions = NULL;
  char *session;
  int i;

  n_sessions = sd_uid_get_sessions (getuid (), 0, &sessions);

  if (n_sessions < 0) {
    g_debug ("Failed to get sessions for user %d", getuid ());
    return FALSE;
  }

  session = NULL;
  for (i = 0; i < n_sessions; i++) {
    int r;
    g_autofree char *type = NULL;
    g_autofree char *desktop = NULL;

    r = sd_session_get_desktop (sessions[i], &desktop);
    if (r < 0) {
      g_debug ("Couldn't get desktop for session '%s': %s",
               sessions[i], strerror (-r));
      continue;
    }

    if (g_strcmp0 (desktop, "phosh") != 0)
      continue;

    r = sd_session_get_type (sessions[i], &type);
    if (r < 0) {
      g_debug ("Couldn't get type for session '%s': %s",
               sessions[i], strerror (-r));
      continue;
    }

    if (g_strcmp0 (type, "wayland") != 0)
      continue;

    session = sessions[i];
    break;
  }

  if (session != NULL)
    *session_id = g_strdup (session);

  return session != NULL;
}

/**
 * phosh_async_error_warn:
 * @err: (nullable): The error to check and print
 * @...: Format string followed by parameters to insert
 *       into the format string (as with printf())
 *
 * Prints a warning when @err is 'real' error. If it merely represents
 * a canceled operation it just logs a debug message. This is useful
 * to avoid this common pattern in async callbacks.
 *
 * Returns: TRUE if #err is cancellation.
 */

/**
 * phosh_dbus_service_error_warn
 * @err: (nullable): The error to check and print
 * @...: Format string followed by parameters to insert
 *       into the format string (as with printf())
 *
 * Prints a warning when @err is 'real' error. If it merely indicates
 * that the DBus service is not present at all it just logs a debug
 * message.
 *
 * Returns: TRUE if #err is cancellation.
 */

/* Helper since phosh_async_error_warn needs to be a macro to capture log_domain */
gboolean
phosh_error_warnv (const char *log_domain,
                   GError     *err,
                   GQuark      domain,
                   gint        code,
                   const gchar *fmt, ...)
{
  g_autofree char *msg = NULL;
  gboolean matched = FALSE;
  va_list args;

  if (err == NULL)
    return FALSE;

  va_start (args, fmt);
  msg = g_strdup_vprintf(fmt, args);
  va_end (args);

  if (g_error_matches (err, domain, code))
    matched = TRUE;

  g_log (log_domain,
         matched ? G_LOG_LEVEL_DEBUG : G_LOG_LEVEL_WARNING,
         "%s: %s", msg, err->message);

  return matched;
}


static void
randname (char *buf)
{
  struct timespec ts;
  long r;
  clock_gettime (CLOCK_REALTIME, &ts);
  r = ts.tv_nsec;
  for (int i = 0; i < 6; ++i) {
    buf[i] = 'A'+(r&15)+(r&16)*2;
    r >>= 5;
  }
}


static int
anonymous_shm_open (void)
{
  char name[] = "/phosh-XXXXXX";
  int retries = 100;

  do {
    int fd;

    randname (name + strlen (name) - 6);
    --retries;
    /* shm_open guarantees that O_CLOEXEC is set */
    fd = shm_open (name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd >= 0) {
      shm_unlink (name);
      return fd;
    }
  } while (retries > 0 && errno == EEXIST);

  return -1;
}

/**
 * phosh_create_shm_file:
 * @size: The file size
 *
 * Create a file share memory file of the given size.
 *
 * Returns: a file descriptor on success or -1 on error.
 */
int
phosh_create_shm_file (off_t size)
{
  int fd = anonymous_shm_open ();
  if (fd < 0) {
    return fd;
  }

  if (ftruncate (fd, size) < 0) {
    close (fd);
    return -1;
  }

  return fd;
}


/**
 * phosh_util_escape_markup:
 * @markup: The markup to escape
 * @allow_markup: Whether to allow certain markup
 *
 * Escapes the given markup either fully or (when @allow_markup is %TRUE) in a way
 * that is suitable for a notification body
 *
 * Returns: (transfer full): The escaped text
 */
char *
phosh_util_escape_markup (const char *markup, gboolean allow_markup)
{
  if (allow_markup) {
    g_autoptr (GError) err = NULL;
    g_autoptr (GRegex) amp_re = NULL;
    g_autoptr (GRegex) elem_re = NULL;
    g_autofree char *amp_esc = NULL;
    g_autofree char *escaped = NULL;

    /* Escape &whatever; */
    /* Support &amp;, &quot;, &apos;, &lt; and &gt;, escape all other occurrences of '&'. */
    amp_re = g_regex_new ("&(?!amp;|quot;|apos;|lt;|gt;)",
                          G_REGEX_JAVASCRIPT_COMPAT,
                          0,
                          &err);
    if (!amp_re) {
      g_warning ("Failed to compile regex: %s", err->message);
      goto out;
    }

    amp_esc = g_regex_replace_literal (amp_re,
                                       markup, -1, 0,
                                       "&amp;",
                                       0, /* flags */
                                       &err);
    if (err) {
      g_warning ("Failed to escape string: %s", err->message);
      goto out;
    }

    /*
     * "HTML" element escape
     * Support <b>, <i>, and <u>, escape anything else
     * so it displays as raw markup.
     * https://specifications.freedesktop.org/notification-spec/latest/ar01s04.html
     */
    elem_re = g_regex_new ("<(?!/?[biu]>)",
                           G_REGEX_JAVASCRIPT_COMPAT,
                           0,
                           &err);
    if (!elem_re) {
      g_warning ("Failed to compile regex: %s", err->message);
      goto out;
    }

    escaped = g_regex_replace_literal (elem_re,
                                       amp_esc, -1, 0,
                                       "&lt;",
                                       0, /* flags */
                                       &err);
    if (err) {
      g_warning ("Failed to escape string: %s", err->message);
      goto out;
    }

    if (pango_parse_markup(escaped, -1, 0, NULL, NULL, NULL, NULL))
      return g_steal_pointer (&escaped);
  }

 out:
  /*invalid markup or no markup allowed */
  return g_markup_escape_text (markup, -1);
}
