/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-lockscreen"

#include "config.h"

#include "auth.h"
#include "bt-info.h"
#include "lockscreen.h"

#include <locale.h>
#include <string.h>
#include <glib/gi18n.h>
#include <math.h>
#include <time.h>

#include <handy.h>

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-wall-clock.h>

#define LOCKSCREEN_IDLE_SECONDS 5

/**
 * SECTION:lockscreen
 * @short_description: The main lock screen
 * @Title: PhoshLockscreen
 *
 * The lock screen featuring the clock
 * and unlock keypad.
 *
 * # CSS nodes
 *
 * #PhoshLockscreen has a CSS name with the name phosh-lockscreen.
 */


typedef enum {
  POS_OVERVIEW = 0,
  POS_UNLOCK   = 1,
} PhoshLocksreenPos;

enum {
  LOCKSCREEN_UNLOCK,
  WAKEUP_OUTPUT,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


typedef struct _PhoshLockscreen
{
  PhoshLayerSurface parent;
} PhoshLockscreen;


typedef struct {
  GtkWidget *carousel;

  /* info page */
  GtkWidget *box_info;
  GtkWidget *lbl_clock;
  GtkWidget *lbl_date;

  /* unlock page */
  GtkWidget *box_unlock;
  GtkWidget *keypad;
  GtkWidget  *entry_pin;
  GtkGesture *long_press_del_gesture;
  GtkWidget *lbl_unlock_status;
  GtkWidget *btn_submit;
  GtkWidget *btn_emergency;
  guint      idle_timer;
  gint64     last_input;
  PhoshAuth *auth;

  GnomeWallClock *wall_clock;
} PhoshLockscreenPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshLockscreen, phosh_lockscreen, PHOSH_TYPE_LAYER_SURFACE)


static void
clear_input (PhoshLockscreen *self, gboolean clear_all) {
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);

  if (clear_all) {
    gtk_label_set_label (GTK_LABEL (priv->lbl_unlock_status), _("Enter Passcode"));
    gtk_editable_delete_text (GTK_EDITABLE (priv->entry_pin), 0, -1);
  } else {
    g_signal_emit_by_name (priv->entry_pin, "backspace", NULL);
  }
}


static void
show_info_page (PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);
  if (hdy_carousel_get_position (HDY_CAROUSEL (priv->carousel)) <= 0)
    return;

  hdy_carousel_scroll_to (HDY_CAROUSEL (priv->carousel), priv->box_info);
}


static gboolean
keypad_check_idle (PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);
  gint64 now = g_get_monotonic_time ();

  g_assert (PHOSH_IS_LOCKSCREEN (self));
  if (now - priv->last_input > LOCKSCREEN_IDLE_SECONDS * 1000 * 1000) {
    show_info_page (self);
    priv->idle_timer = 0;
    return FALSE;
  }
  return TRUE;
}


static void
show_unlock_page (PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);
  if (hdy_carousel_get_position (HDY_CAROUSEL (priv->carousel)) > 0)
    return;

  hdy_carousel_scroll_to (HDY_CAROUSEL (priv->carousel), priv->box_unlock);

  /* skip signal on init */
  if (signals[WAKEUP_OUTPUT])
    g_signal_emit (self, signals[WAKEUP_OUTPUT], 0);
}


static gboolean
finish_shake_label (PhoshLockscreen *self) {
  clear_input (self, TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (self), TRUE);
  return FALSE;
}


static gboolean
shake_label (GtkWidget *widget,
                         GdkFrameClock *frame_clock,
                         gpointer data) {
  PhoshLockscreen *self = PHOSH_LOCKSCREEN (widget);
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);
  gint64 start_time = g_variant_get_int64 (data);
  gint64 end_time = start_time + 1000 * 300;
  gint64 now = gdk_frame_clock_get_frame_time (frame_clock);

  float t = (float) (now - start_time) / (float) (end_time - start_time);
  float pos = sin (t * 10) * 0.05 + 0.5;

  if (now > end_time) {
    /* Stop the animation only when we would step over the idle position (0.5) */
    if ((gtk_entry_get_alignment (GTK_ENTRY (priv->entry_pin)) > 0.5 && pos < 0.5) || pos > 0.5) {
      gtk_entry_set_alignment (GTK_ENTRY (priv->entry_pin), 0.5);
      g_timeout_add (400, (GSourceFunc) finish_shake_label, self);
      return FALSE;
    }
  }

  gtk_entry_set_alignment (GTK_ENTRY (priv->entry_pin), pos);
  return TRUE;
}


/* callback of async auth task */
static void
auth_async_cb (PhoshAuth *auth, GAsyncResult *result, PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv;
  GError *error = NULL;
  gboolean authenticated;

  priv = phosh_lockscreen_get_instance_private (self);
  authenticated = phosh_auth_authenticate_async_finish (auth, result, &error);
  if (error != NULL) {
    g_warning ("Auth failed unexpected: %s", error->message);
    return;
  }

  g_object_ref (self);
  if (authenticated) {
    g_signal_emit(self, signals[LOCKSCREEN_UNLOCK], 0);
    g_clear_object (&priv->auth);
  } else {
    GdkFrameClock *clock;
    gint64 now;
    /* give visual feedback on error */
    clock = gtk_widget_get_frame_clock (priv->entry_pin);
    now = gdk_frame_clock_get_frame_time (clock);
    gtk_widget_add_tick_callback (GTK_WIDGET (self),
                                  shake_label,
                                  g_variant_ref_sink (g_variant_new_int64 (now)),
                                  (GDestroyNotify) g_variant_unref);


  }
  priv->last_input = g_get_monotonic_time ();
  g_object_unref (self);
}


static void
delete_button_clicked_cb (PhoshLockscreen *self,
                          GtkWidget      *widget)
{
  g_return_if_fail (PHOSH_IS_LOCKSCREEN (self));

  clear_input (self, FALSE);
}


static void
long_press_del_cb (PhoshLockscreen *self,
                   double           x,
                   double           y,
                   GtkGesture      *gesture)
{
  g_return_if_fail (PHOSH_IS_LOCKSCREEN (self));
  g_debug ("Long press on delete button");
  clear_input (self, TRUE);
}


static void
input_changed_cb (PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv;
  guint16 length;

  g_assert (PHOSH_IS_LOCKSCREEN (self));
  priv = phosh_lockscreen_get_instance_private (self);
  priv->last_input = g_get_monotonic_time ();

  length = gtk_entry_get_text_length (GTK_ENTRY (priv->entry_pin));

  gtk_widget_set_sensitive (priv->btn_submit, length != 0);
}


static void
submit_cb (PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv;
  const char *input;
  guint16 length;

  g_assert (PHOSH_IS_LOCKSCREEN (self));

  priv = phosh_lockscreen_get_instance_private (self);
  priv->last_input = g_get_monotonic_time ();

  length = gtk_entry_get_text_length (GTK_ENTRY (priv->entry_pin));
  if (length == 0) {
    return;
  }

  input = gtk_entry_get_text (GTK_ENTRY (priv->entry_pin));

  gtk_label_set_label (GTK_LABEL (priv->lbl_unlock_status), _("Checking…"));
  gtk_widget_set_sensitive (GTK_WIDGET (self), FALSE);

  if (priv->auth == NULL)
    priv->auth = PHOSH_AUTH (phosh_auth_new ());
  phosh_auth_authenticate_async_start (priv->auth,
                                       input,
                                       NULL,
                                       (GAsyncReadyCallback)auth_async_cb,
                                       self);
}


static gboolean
key_press_event_cb (PhoshLockscreen *self, GdkEventKey *event, gpointer data)
{
  PhoshLockscreenPrivate *priv;
  gboolean handled = FALSE;

  g_assert (PHOSH_IS_LOCKSCREEN (self));
  priv = phosh_lockscreen_get_instance_private (self);

  if (gtk_entry_im_context_filter_keypress (GTK_ENTRY (priv->entry_pin), event)) {
    show_unlock_page (self);
    handled = TRUE;
  } else {
    switch (event->keyval) {
    case GDK_KEY_space:
      show_unlock_page (self);
      handled = TRUE;
      break;
    case GDK_KEY_Escape:
      show_info_page (self);
      handled = TRUE;
      break;
    case GDK_KEY_Delete:
    case GDK_KEY_KP_Delete:
    case GDK_KEY_BackSpace:
      clear_input (self, FALSE);
      handled = TRUE;
      break;
    case GDK_KEY_Return:
    case GDK_KEY_ISO_Enter:
    case GDK_KEY_KP_Enter:
      submit_cb (self),
      handled = TRUE;
      break;
    default:
      /* nothing to do */
      break;
    }
  }

  priv->last_input = g_get_monotonic_time ();
  return handled;
}


/**
 * date_fmt:
 *
 * Get a date format based on LC_TIME.
 * This is done by temporarily swithcing LC_MESSAGES so we can look up
 * the format in our message catalog.  This will fail if LANGUAGE is
 * set to something different since LANGUAGE overrides
 * LC_{ALL,MESSAGE}.
 */
static const char *
date_fmt (void)
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
 * local_date:
 *
 * Get the local date as string
 * We honor LC_MESSAGES so we e.g. don't get a translated date when
 * the user has LC_MESSAGES=en_US.UTF-8 but LC_TIME to their local
 * time zone.
 */
static char *
local_date (void)
{
  time_t current = time (NULL);
  struct tm local;
  g_autofree char *date = NULL;
  const char *fmt;
  const char *locale;

  g_return_val_if_fail (current != (time_t) -1, NULL);
  g_return_val_if_fail (localtime_r (&current, &local), NULL);

  date = g_malloc0 (256);
  fmt = date_fmt ();
  locale = setlocale (LC_MESSAGES, NULL);
  if (locale) /* make sure weekday and month use LC_MESSAGES */
    setlocale (LC_TIME, locale);
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
  /* Can't use a string literal since it needs to be translated */
  g_return_val_if_fail (strftime (date, 255, fmt, &local), NULL);
#pragma GCC diagnostic error "-Wformat-nonliteral"
  setlocale (LC_TIME, "");
  return g_steal_pointer (&date);
}


static void
wall_clock_notify_cb (PhoshLockscreen *self,
                      GParamSpec *pspec,
                      GnomeWallClock *wall_clock)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);
  const char *time;
  g_autofree char *date = NULL;

  time = gnome_wall_clock_get_clock(wall_clock);
  gtk_label_set_text (GTK_LABEL (priv->lbl_clock), time);

  date = local_date ();
  gtk_label_set_label (GTK_LABEL (priv->lbl_date), date);
}


static void
carousel_position_notified_cb (PhoshLockscreen *self,
                               GParamSpec      *pspec,
                               HdyCarousel     *carousel)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);
  double position;

  position = hdy_carousel_get_position (HDY_CAROUSEL (priv->carousel));

  if (position <= POS_OVERVIEW) {
    clear_input (self, TRUE);
    return;
  }

  if (position >= POS_UNLOCK) {
    if (!priv->idle_timer) {
      priv->last_input = g_get_monotonic_time ();
      priv->idle_timer = g_timeout_add_seconds (LOCKSCREEN_IDLE_SECONDS,
                                                (GSourceFunc) keypad_check_idle,
                                                self);
    }

    return;
  }

  if (priv->idle_timer) {
    g_source_remove (priv->idle_timer);
    priv->idle_timer = 0;
  }
}


static void
phosh_lockscreen_constructed (GObject *object)
{
  PhoshLockscreen *self = PHOSH_LOCKSCREEN (object);
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);

  G_OBJECT_CLASS (phosh_lockscreen_parent_class)->constructed (object);

  /* window properties */
  gtk_window_set_title (GTK_WINDOW (self), "phosh lockscreen");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_widget_realize(GTK_WIDGET (self));

  gtk_widget_add_events (GTK_WIDGET (self), GDK_KEY_PRESS_MASK);
  g_signal_connect (G_OBJECT (self),
                    "key_press_event",
                    G_CALLBACK (key_press_event_cb),
                    NULL);

  priv->wall_clock = g_object_new (GNOME_TYPE_WALL_CLOCK,
                                   "time-only", TRUE,
                                   NULL);
  g_signal_connect_swapped (priv->wall_clock,
                            "notify::clock",
                            G_CALLBACK (wall_clock_notify_cb),
                            self);
  wall_clock_notify_cb (self, NULL, priv->wall_clock);
}


static void
phosh_lockscreen_dispose (GObject *object)
{
  PhoshLockscreen *self = PHOSH_LOCKSCREEN (object);
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);

  g_clear_object (&priv->wall_clock);
  if (priv->idle_timer) {
    g_source_remove (priv->idle_timer);
    priv->idle_timer = 0;
  }

  G_OBJECT_CLASS (phosh_lockscreen_parent_class)->dispose (object);
}


static void
phosh_lockscreen_class_init (PhoshLockscreenClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_lockscreen_constructed;
  object_class->dispose = phosh_lockscreen_dispose;

  signals[LOCKSCREEN_UNLOCK] = g_signal_new ("lockscreen-unlock",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
  /**
   * PhoshLockscreen::wakeup-output
   * @self: The #PhoshLockscreen emitting this signal
   *
   * Emitted when the output showing the lock screen should be woken
   * up.
   */
  signals[WAKEUP_OUTPUT] = g_signal_new ("wakeup-output",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  gtk_widget_class_set_css_name (widget_class, "phosh-lockscreen");
  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/lockscreen.ui");
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, carousel);
  gtk_widget_class_bind_template_callback_full (widget_class,
                                                "carousel_position_notified_cb",
                                                G_CALLBACK(carousel_position_notified_cb));

  /* unlock page */
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, box_unlock);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, keypad);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, entry_pin);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, lbl_unlock_status);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, long_press_del_gesture);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, btn_submit);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, btn_emergency);

  gtk_widget_class_bind_template_callback (widget_class, long_press_del_cb);
  gtk_widget_class_bind_template_callback (widget_class, delete_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, submit_cb);
  gtk_widget_class_bind_template_callback (widget_class, input_changed_cb);

  /* info page */
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, box_info);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, lbl_clock);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, lbl_date);
  gtk_widget_class_bind_template_callback (widget_class, show_unlock_page);
}


static void
phosh_lockscreen_init (PhoshLockscreen *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_lockscreen_new (gpointer layer_shell,
                      gpointer wl_output)
{
  return g_object_new (PHOSH_TYPE_LOCKSCREEN,
                       "layer-shell", layer_shell,
                       "wl-output", wl_output,
                       "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
                       "kbd-interactivity", TRUE,
                       "exclusive-zone", -1,
                       "namespace", "phosh lockscreen",
                       NULL);
}

/**
 * phosh_lockscreen_get_page
 * @self: The #PhoshLockscreen
 *
 * Returns: The #PhoshLockscreenPage that is currently shown
 */
PhoshLockscreenPage
phosh_lockscreen_get_page (PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);
  gdouble position = hdy_carousel_get_position (HDY_CAROUSEL (priv->carousel));

  if (position <= 0)
    return PHOSH_LOCKSCREEN_PAGE_DEFAULT;
  else
    return PHOSH_LOCKSCREEN_PAGE_UNLOCK;
}
