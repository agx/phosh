/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-util"

#include "util.h"
#include <gtk/gtk.h>

#include <locale.h>
#include <glib/gi18n.h>

#include <systemd/sd-login.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>


#if !GLIB_CHECK_VERSION(2, 73, 2)
#define G_REGEX_DEFAULT 0
#endif


static gboolean have_gnome_software = -1;


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
  g_autofree char *session = NULL;
  int r;

  g_return_val_if_fail (session_id != NULL && *session_id == NULL, FALSE);

  r = sd_pid_get_session (getpid(), &session);
  if (r == 0) {
    *session_id = g_steal_pointer (&session);
    return TRUE;
  }

  /* Not in a system session, so let logind make a pick */
  r = sd_uid_get_display(getuid(), &session);
  if (r == 0) {
    *session_id = g_steal_pointer (&session);
    return TRUE;
  }

  return FALSE;
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
 * Create a shared memory file of the given size.
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
 * phoc_util_date_fmt:
 *
 * Get a date format based on LC_TIME.
 * This is done by temporarily swithcing LC_MESSAGES so we can look up
 * the format in our message catalog.  This will fail if LANGUAGE is
 * set to something different since LANGUAGE overrides
 * LC_{ALL,MESSAGE}.
 */
static const char *
phosh_util_date_fmt (void)
{
  const char *locale;
  const char *fmt;

  locale = setlocale (LC_TIME, NULL);
  if (locale) /* Lookup date format via messages catalog */
    setlocale (LC_MESSAGES, locale);
  /* Translators: This is a time format for a date in
     long format */
  fmt = _("%A, %B %-e");
  setlocale (LC_MESSAGES, "");
  return fmt;
}

/**
 * phoc_util_local_date:
 *
 * Get the local date as string
 * We honor LC_MESSAGES so we e.g. don't get a translated date when
 * the user has LC_MESSAGES=en_US.UTF-8 but LC_TIME to their local
 * time zone.
 *
 * Returns: The local date as string
 */
char *
phosh_util_local_date (void)
{
  time_t current = time (NULL);
  struct tm local;
  g_autofree char *date = NULL;
  const char *fmt;
  const char *locale;

  g_return_val_if_fail (current != (time_t) -1, NULL);
  g_return_val_if_fail (localtime_r (&current, &local), NULL);

  date = g_malloc0 (256);
  fmt = phosh_util_date_fmt ();
  locale = setlocale (LC_MESSAGES, NULL);
  if (locale) /* make sure weekday and month use LC_MESSAGES */
    setlocale (LC_TIME, locale);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
  /* Can't use a string literal since it needs to be translated */
  g_return_val_if_fail (strftime (date, 255, fmt, &local), NULL);
#pragma GCC diagnostic pop
  setlocale (LC_TIME, "");
  return g_steal_pointer (&date);
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
                          G_REGEX_DEFAULT,
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
                           G_REGEX_DEFAULT,
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


gboolean
phosh_util_gesture_is_touch (GtkGestureSingle *gesture)
{
  GdkEventSequence *seq;
  const GdkEvent *event;
  GdkDevice *device;

  g_return_val_if_fail (GTK_IS_GESTURE_SINGLE (gesture), FALSE);

  seq = gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture));
  event = gtk_gesture_get_last_event (GTK_GESTURE (gesture), seq);

  if (event == NULL)
    return FALSE;

  if (event->type != GDK_BUTTON_PRESS && event->type != GDK_BUTTON_RELEASE)
    return FALSE;

  device = gdk_event_get_source_device (event);

  if (device == NULL)
    return FALSE;

  if (gdk_device_get_source (device) != GDK_SOURCE_TOUCHSCREEN)
    return FALSE;

  return TRUE;
}


/**
 * phosh_util_have_gnome_software:
 * @scan: Whether to scan $PATH again
 *
 * Returns: the (cached) answer if gnome-software can be found in the path.
 */
gboolean
phosh_util_have_gnome_software (gboolean scan)
{
  g_autofree gchar *path = NULL;

  if (have_gnome_software >= 0 && !scan)
    return have_gnome_software;

  path = g_find_program_in_path ("gnome-software");
  have_gnome_software =  !!path;
  return have_gnome_software;
}

/**
 * phosh_util_toggle_style_class:
 * @widget: Widget to change styling of
 * @style_class: The name of CSS class
 * @toggle: Whether the class should be set or unset
 *
 * Adds or removes the specified style class on the widget.
 */
void
phosh_util_toggle_style_class (GtkWidget *widget, const char *style_class, gboolean toggle)
{
  GtkStyleContext *context = gtk_widget_get_style_context (widget);

  if (toggle)
    gtk_style_context_add_class (context, style_class);
  else
    gtk_style_context_remove_class (context, style_class);
}

/**
 * phosh_util_get_stylesheet:
 * @theme_name: A theme name
 *
 * Returns: The stylesheet to be used for the given theme
 */
const char *
phosh_util_get_stylesheet (const char *theme_name)
{
  const char *style;

  if (g_strcmp0 (theme_name, "HighContrast") == 0)
    style = "/sm/puri/phosh/stylesheet/adwaita-hc-light.css";
  else
    style = "/sm/puri/phosh/stylesheet/adwaita-dark.css";

  return style;
}
