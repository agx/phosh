/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-lockscreen"

#include "phosh-config.h"

#include "auth.h"
#include "bt-info.h"
#include "calls-manager.h"
#include "keypad.h"
#include "lockscreen.h"
#include "notifications/notify-manager.h"
#include "notifications/notification-frame.h"
#include "osk-button.h"
#include "osk-manager.h"
#include "shell.h"
#include "util.h"
#include "widget-box.h"

#include <string.h>
#include <glib/gi18n.h>
#include <math.h>
#include <time.h>

#include <handy.h>
#include <cui-call-display.h>


#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-wall-clock.h>

#define LOCKSCREEN_IDLE_SECONDS 5
#define LOCKSCREEN_SMALL_DATE_AND_TIME_CLASS "p-small"

#define LOCKSCREEN_SMALL_DISPLAY 700

/**
 * PhoshLockscreen:
 *
 * The main lock screen
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
  PROP_0,
  PROP_CALLS_MANAGER,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

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
  HdyDeck           *deck;
  GtkWidget         *carousel;

  /* info page */
  GtkWidget         *box_info;
  GtkWidget         *box_datetime;
  GtkWidget         *lbl_clock;
  GtkWidget         *lbl_date;
  GtkWidget         *list_notifications;
  GtkRevealer       *rev_media_player;
  GtkRevealer       *rev_notifications;
  GSettings         *settings;
  guint              reveals;

  /* unlock page */
  GtkWidget         *box_unlock;
  GtkWidget         *keypad_revealer;
  GtkWidget         *keypad;
  GtkWidget         *entry_pin;
  GtkGesture        *long_press_del_gesture;
  GtkWidget         *lbl_unlock_status;
  GtkWidget         *btn_submit;
  guint              idle_timer;
  gint64             last_input;
  PhoshAuth         *auth;
  GSettings         *keypad_settings;

  /* widget box */
  GtkWidget         *widget_box;

  /* Call page */
  GtkBox            *box_call_display;
  CuiCallDisplay    *call_display;

  GnomeWallClock    *wall_clock;
  PhoshCallsManager *calls_manager;
  char              *active; /* opaque handle to the active call */
} PhoshLockscreenPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshLockscreen, phosh_lockscreen, PHOSH_TYPE_LAYER_SURFACE)


static void
phosh_lockscreen_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  PhoshLockscreen *self = PHOSH_LOCKSCREEN (object);
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);

  switch (property_id) {
  case PROP_CALLS_MANAGER:
    priv->calls_manager = g_value_dup_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_lockscreen_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PhoshLockscreen *self = PHOSH_LOCKSCREEN (object);
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);

  switch (property_id) {
  case PROP_CALLS_MANAGER:
    g_value_set_object (value, priv->calls_manager);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
clear_input (PhoshLockscreen *self, gboolean clear_all)
{
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
    return G_SOURCE_REMOVE;
  }
  return G_SOURCE_CONTINUE;
}


static void
show_unlock_page (PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);

  if (hdy_carousel_get_position (HDY_CAROUSEL (priv->carousel)) >= POS_UNLOCK)
    return;

  hdy_carousel_scroll_to (HDY_CAROUSEL (priv->carousel), priv->box_unlock);

  /* skip signal on init */
  if (signals[WAKEUP_OUTPUT])
    g_signal_emit (self, signals[WAKEUP_OUTPUT], 0);
}


static gboolean
finish_shake_label (PhoshLockscreen *self)
{
  clear_input (self, TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (self), TRUE);
  return G_SOURCE_REMOVE;
}


static gboolean
shake_label (GtkWidget     *widget,
             GdkFrameClock *frame_clock,
             gpointer       data)
{
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

static void
focus_pin_entry (PhoshLockscreen *self, gboolean enable_osk)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);

  if (enable_osk) {
    /* restore default OSK behavior */
    g_object_set (priv->entry_pin, "im-module", NULL, NULL);
  }

  gtk_widget_set_sensitive (priv->entry_pin, TRUE);
  gtk_entry_grab_focus_without_selecting (GTK_ENTRY (priv->entry_pin));
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

  if (authenticated) {
    g_signal_emit (self, signals[LOCKSCREEN_UNLOCK], 0);
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
    phosh_keypad_distribute (PHOSH_KEYPAD (priv->keypad));
  }
  priv->last_input = g_get_monotonic_time ();
  g_object_unref (self);
}


static void
delete_button_clicked_cb (PhoshLockscreen *self,
                          GtkWidget       *widget)
{
  g_return_if_fail (PHOSH_IS_LOCKSCREEN (self));

  clear_input (self, FALSE);
}

static void
osk_button_clicked_cb (PhoshLockscreen *self,
                       GtkWidget       *widget)
{
  PhoshLockscreenPrivate *priv;

  g_assert (PHOSH_IS_LOCKSCREEN (self));
  priv = phosh_lockscreen_get_instance_private (self);

  priv->last_input = g_get_monotonic_time ();

  focus_pin_entry (self, TRUE);
}


static void
on_osk_visibility_changed (PhoshLockscreen *self,
                           GParamSpec      *pspec,
                           PhoshOskManager *osk)
{
  PhoshLockscreenPrivate *priv;

  g_assert (PHOSH_IS_LOCKSCREEN (self));
  priv = phosh_lockscreen_get_instance_private (self);

  if (!phosh_osk_manager_get_visible (osk)) {
    g_object_set (priv->entry_pin, "im-module", "gtk-im-context-none", NULL);
  }
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
                                       g_object_ref (self));
}


static gboolean
key_press_event_cb (PhoshLockscreen *self, GdkEventKey *event, gpointer data)
{
  PhoshLockscreenPrivate *priv;
  gboolean handled = FALSE;
  gboolean on_unlock_page;
  double position;

  g_assert (PHOSH_IS_LOCKSCREEN (self));
  priv = phosh_lockscreen_get_instance_private (self);

  position = hdy_carousel_get_position (HDY_CAROUSEL (priv->carousel));
  /* Round to nearest page so we already accept keyboard input before animation ends */
  on_unlock_page = (int)round(position) == POS_UNLOCK;

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
      clear_input (self, TRUE);
      show_info_page (self);
      handled = TRUE;
      break;
    case GDK_KEY_Delete:
    case GDK_KEY_KP_Delete:
    case GDK_KEY_BackSpace:
      if (on_unlock_page == TRUE) {
        clear_input (self, FALSE);
        handled = TRUE;
      }
      break;
    case GDK_KEY_Return:
    case GDK_KEY_ISO_Enter:
    case GDK_KEY_KP_Enter:
      if (on_unlock_page == TRUE) {
        submit_cb (self);
        handled = TRUE;
      }
      break;
    default:
      /* nothing to do */
      break;
    }
  }

  priv->last_input = g_get_monotonic_time ();
  return handled;
}


static void
wall_clock_notify_cb (PhoshLockscreen *self,
                      GParamSpec      *pspec,
                      GnomeWallClock  *wall_clock)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);
  const char *time;
  g_autofree char *date = NULL;
  g_auto (GStrv) parts = NULL;

  time = gnome_wall_clock_get_clock (wall_clock);

  /* Strip " {A,P}M" from 12h time format to look less cramped */
  if (g_str_has_suffix (time, "AM") || g_str_has_suffix (time, "PM")) {
    parts = g_strsplit (time, " ", -1);

    if (g_strv_length (parts) == 2) {
      /* Glib >= 2.74: padding with figure-space */
      if (g_str_has_prefix (parts[0], "\u2007")) {
        time = parts[0] + strlen("\u2007");
      } else {
        time = parts[0];
      }
    /* Glib < 2.74: padding with ascii space */
    } else if (g_strv_length (parts) == 3) {
      time = parts[1];
    } else {
      g_warning ("Can't parse time format: %s", time);
    }
  }
  gtk_label_set_text (GTK_LABEL (priv->lbl_clock), time);

  date = phosh_util_local_date ();
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

  if (position <= POS_OVERVIEW || position >= POS_UNLOCK)
    return;

  g_clear_handle_id (&priv->idle_timer, g_source_remove);
}

static void
carousel_page_changed_cb (PhoshLockscreen *self,
                          guint            index,
                          HdyCarousel     *carousel)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshOskManager *osk_manager = phosh_shell_get_osk_manager (shell);
  gboolean osk_visible = phosh_osk_manager_get_visible (osk_manager);

  if (index == POS_OVERVIEW) {
    gtk_widget_set_sensitive (priv->entry_pin, FALSE);
    clear_input (self, TRUE);
  } else if (index == POS_UNLOCK) {
    focus_pin_entry (self, osk_visible);

    if (!priv->idle_timer) {
      priv->last_input = g_get_monotonic_time ();
      priv->idle_timer = g_timeout_add_seconds (LOCKSCREEN_IDLE_SECONDS,
                                                (GSourceFunc) keypad_check_idle,
                                                self);
    }
  } else {
    g_assert_not_reached ();
  }
}


static void
update_active_call (PhoshLockscreen *self, const gchar *path)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);
  PhoshCall *call;

  g_debug ("New call %s", path);
  g_signal_emit (self, signals[WAKEUP_OUTPUT], 0);

  g_free (priv->active);
  priv->active = g_strdup (path);

  call = phosh_calls_manager_get_call (priv->calls_manager, path);
  g_return_if_fail (PHOSH_IS_CALL (call));

  cui_call_display_set_call (priv->call_display, CUI_CALL (call));
}


static void
on_calls_call_added (PhoshLockscreen *self, const gchar *path)
{
  PhoshLockscreenPrivate *priv;

  g_return_if_fail (PHOSH_IS_LOCKSCREEN (self));
  priv = phosh_lockscreen_get_instance_private (self);
  g_return_if_fail (PHOSH_IS_CALLS_MANAGER (priv->calls_manager));

  update_active_call (self, path);

  hdy_deck_set_visible_child (priv->deck, GTK_WIDGET (priv->box_call_display));
}


static void
on_calls_call_removed (PhoshLockscreen *self, const gchar *path)
{
  PhoshLockscreenPrivate *priv;

  g_return_if_fail (path != NULL);
  g_return_if_fail (PHOSH_IS_LOCKSCREEN (self));
  priv = phosh_lockscreen_get_instance_private (self);

  g_debug ("Call %s removed, active: %s", path, priv->active);

  if (g_strcmp0 (path, priv->active))
    return;

  g_clear_pointer (&priv->active, g_free);

  hdy_deck_set_visible_child (priv->deck, GTK_WIDGET (priv->carousel));
}


static void
on_deck_visible_child_changed (PhoshLockscreen *self, GParamSpec *pspec, HdyDeck *deck)
{
  GtkWidget *visible_child;
  PhoshLockscreenPrivate *priv;
  gboolean swipe_forward = TRUE;
  gboolean swipe_back = TRUE;

  g_return_if_fail (HDY_IS_DECK (deck));
  g_return_if_fail (PHOSH_IS_LOCKSCREEN (self));
  priv = phosh_lockscreen_get_instance_private (self);

  visible_child = hdy_deck_get_visible_child (deck);

  /* Avoid forward swipe to calls page if there's no active call */
  if (visible_child == priv->carousel &&
      phosh_calls_manager_get_active_call_handle (priv->calls_manager) == NULL) {
    swipe_forward = FALSE;
  }

  /* Avoid backward swipe to widget-box if there's no plugin */
  if (visible_child == priv->carousel && !phosh_widget_box_has_plugins (PHOSH_WIDGET_BOX (priv->widget_box))) {
    swipe_back = FALSE;
  }

  hdy_deck_set_can_swipe_forward (deck, swipe_forward);
  hdy_deck_set_can_swipe_back (deck, swipe_back);

  /* See https://gitlab.gnome.org/World/Phosh/phosh/-/issues/922 */
  if (visible_child == priv->carousel)
    gtk_widget_queue_draw (priv->lbl_clock);
}


static GtkWidget *
create_notification_row (gpointer item, gpointer data)
{
  GtkWidget *row = NULL;
  GtkWidget *frame = NULL;
  const char *action_filters[] = { "X-Phosh-Lockscreen-Actions", NULL };

  row = g_object_new (GTK_TYPE_LIST_BOX_ROW,
                      "activatable", FALSE,
                      "visible", TRUE,
                      NULL);

  frame = phosh_notification_frame_new (FALSE, action_filters);
  phosh_notification_frame_bind_model (PHOSH_NOTIFICATION_FRAME (frame), item);

  gtk_widget_show (frame);
  gtk_container_add (GTK_CONTAINER (row), frame);

  return row;
}


static void
animate_clock (PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv;

  priv = phosh_lockscreen_get_instance_private (self);

  /* Use small clock if any additional info elements are revealed */
  phosh_util_toggle_style_class (priv->box_datetime, LOCKSCREEN_SMALL_DATE_AND_TIME_CLASS,
                                 !!priv->reveals);
}


static void
on_info_reveal_child_changed (PhoshLockscreen *self, GParamSpec *pspec, GtkRevealer *revealer)
{
  PhoshLockscreenPrivate *priv;

  g_return_if_fail (PHOSH_IS_LOCKSCREEN (self));
  priv = phosh_lockscreen_get_instance_private (self);

  if (gtk_revealer_get_reveal_child (revealer))
    priv->reveals++;
  else
    priv->reveals--;

  animate_clock (self);
}


static void
on_notification_items_changed (PhoshLockscreen *self,
                               guint            position,
                               guint            removed,
                               guint            added,
                               GListModel      *list)
{
  PhoshLockscreenPrivate *priv;
  gboolean is_empty;
  gboolean show_in_lockscreen;
  gboolean reveal;

  g_return_if_fail (G_IS_LIST_MODEL (list));
  g_return_if_fail (PHOSH_IS_LOCKSCREEN (self));
  priv = phosh_lockscreen_get_instance_private (self);

  show_in_lockscreen = g_settings_get_boolean (priv->settings, "show-in-lock-screen");
  is_empty = !g_list_model_get_n_items (list);

  reveal = show_in_lockscreen && !is_empty;

  gtk_revealer_set_reveal_child (priv->rev_notifications, reveal);
}


static void
phosh_lockscreen_constructed (GObject *object)
{
  PhoshLockscreen *self = PHOSH_LOCKSCREEN (object);
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);
  const char *active;
  PhoshNotifyManager *manager;
  PhoshShell *shell;
  g_autoptr (GSettings) plugin_settings = NULL;
  g_auto (GStrv) plugins = NULL;

  G_OBJECT_CLASS (phosh_lockscreen_parent_class)->constructed (object);

  /* window properties */
  gtk_window_set_title (GTK_WINDOW (self), "phosh lockscreen");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);

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

  g_signal_connect_object (priv->calls_manager,
                           "call-added",
                           G_CALLBACK (on_calls_call_added),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (priv->calls_manager,
                           "call-removed",
                           G_CALLBACK (on_calls_call_removed),
                           self,
                           G_CONNECT_SWAPPED);

  /* If a call is ongoing show it when locking until we show a notification */
  active = phosh_calls_manager_get_active_call_handle (priv->calls_manager);
  if (active)
    on_calls_call_added (self, active);

  manager = phosh_notify_manager_get_default ();
  priv->settings = g_settings_new(NOTIFICATIONS_SCHEMA_ID);
  gtk_list_box_bind_model (GTK_LIST_BOX (priv->list_notifications),
                           G_LIST_MODEL (phosh_notify_manager_get_list (manager)),
                           create_notification_row,
                           NULL,
                           NULL);
  g_signal_connect_object (phosh_notify_manager_get_list (manager),
                           "items-changed",
                           G_CALLBACK (on_notification_items_changed),
                           self,
                           G_CONNECT_SWAPPED);
  on_notification_items_changed (self, -1, -1, -1,
                                 G_LIST_MODEL (phosh_notify_manager_get_list (manager)));

  shell = phosh_shell_get_default ();
  g_object_bind_property (phosh_shell_get_osk_manager (shell), "visible",
                          priv->keypad_revealer, "reveal-child",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);
  g_signal_connect_object (phosh_shell_get_osk_manager (shell), "notify::visible",
                           G_CALLBACK (on_osk_visibility_changed), self,
                           G_CONNECT_SWAPPED);

  priv->keypad_settings = g_settings_new("sm.puri.phosh.lockscreen");
  g_settings_bind (priv->keypad_settings, "shuffle-keypad",
                   priv->keypad, "shuffle",
                   G_SETTINGS_BIND_GET);

  plugin_settings = g_settings_new ("sm.puri.phosh.plugins");
  plugins = g_settings_get_strv (plugin_settings, "lock-screen");

  if (plugins)
    phosh_widget_box_set_plugins (PHOSH_WIDGET_BOX (priv->widget_box), plugins);

  on_deck_visible_child_changed (self, NULL, priv->deck);
}

static void
deck_back_clicked_cb (GtkWidget       *sender,
                      PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);

  hdy_deck_set_visible_child (priv->deck, GTK_WIDGET (priv->carousel));
}


static void
deck_forward_clicked_cb (GtkWidget       *sender,
                         PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);

  hdy_deck_set_visible_child (priv->deck, GTK_WIDGET (priv->carousel));
}


static void
phosh_lockscreen_dispose (GObject *object)
{
  PhoshLockscreen *self = PHOSH_LOCKSCREEN (object);
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);

  g_clear_object (&priv->settings);
  g_clear_object (&priv->wall_clock);
  g_clear_handle_id (&priv->idle_timer, g_source_remove);
  g_clear_object (&priv->calls_manager);
  g_clear_pointer (&priv->active, g_free);
  g_clear_object (&priv->keypad_settings);

  G_OBJECT_CLASS (phosh_lockscreen_parent_class)->dispose (object);
}


static void
phosh_lockscreen_configured (PhoshLayerSurface *layer_surface)
{
  PhoshLockscreen *self = PHOSH_LOCKSCREEN (layer_surface);
  PhoshLockscreenPrivate *priv;
  guint height, margin = 100;

  g_return_if_fail (PHOSH_IS_LOCKSCREEN (self));

  priv = phosh_lockscreen_get_instance_private (self);
  height = phosh_layer_surface_get_configured_height (layer_surface);

  /* Avoid margin on smaller displays */
  if (height < LOCKSCREEN_SMALL_DISPLAY)
    margin = 0;

  gtk_widget_set_margin_top (GTK_WIDGET (priv->box_unlock), margin);

  PHOSH_LAYER_SURFACE_CLASS (phosh_lockscreen_parent_class)->configured (layer_surface);
}


static void
phosh_lockscreen_class_init (PhoshLockscreenClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  PhoshLayerSurfaceClass *layer_surface_class = PHOSH_LAYER_SURFACE_CLASS (klass);

  object_class->constructed = phosh_lockscreen_constructed;
  object_class->dispose = phosh_lockscreen_dispose;

  object_class->set_property = phosh_lockscreen_set_property;
  object_class->get_property = phosh_lockscreen_get_property;

  layer_surface_class->configured = phosh_lockscreen_configured;

  props[PROP_CALLS_MANAGER] =
    g_param_spec_object ("calls-manager",
                         "",
                         "",
                         PHOSH_TYPE_CALLS_MANAGER,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

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

  g_type_ensure (PHOSH_TYPE_KEYPAD);
  g_type_ensure (PHOSH_TYPE_OSK_BUTTON);
  g_type_ensure (PHOSH_TYPE_WIDGET_BOX);
  gtk_widget_class_set_css_name (widget_class, "phosh-lockscreen");
  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/lockscreen.ui");
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, carousel);
  gtk_widget_class_bind_template_callback_full (widget_class,
                                                "carousel_position_notified_cb",
                                                G_CALLBACK (carousel_position_notified_cb));
  gtk_widget_class_bind_template_callback_full (widget_class,
                                                "carousel_page_changed_cb",
                                                G_CALLBACK (carousel_page_changed_cb));

  /* main deck */
  gtk_widget_class_bind_template_callback (widget_class, deck_forward_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, deck_forward_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, deck_back_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_deck_visible_child_changed);

  /* unlock page */
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, box_unlock);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, keypad);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, keypad_revealer);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, entry_pin);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, lbl_unlock_status);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, long_press_del_gesture);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, btn_submit);

  gtk_widget_class_bind_template_callback (widget_class, long_press_del_cb);
  gtk_widget_class_bind_template_callback (widget_class, delete_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, osk_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, submit_cb);
  gtk_widget_class_bind_template_callback (widget_class, input_changed_cb);

  /* info page */
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, box_info);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, box_datetime);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, lbl_clock);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, lbl_date);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, list_notifications);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, rev_media_player);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, rev_notifications);
  gtk_widget_class_bind_template_callback (widget_class, on_info_reveal_child_changed);
  gtk_widget_class_bind_template_callback (widget_class, show_unlock_page);

  /* plugin page */
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, widget_box);

  /* Call UI */
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, deck);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, box_call_display);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, call_display);
}


static void
phosh_lockscreen_init (PhoshLockscreen *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_lockscreen_new (gpointer layer_shell,
                      gpointer wl_output,
                      PhoshCallsManager *calls_manager)
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
                       "calls-manager", calls_manager,
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
  PhoshLockscreenPrivate *priv;
  gdouble position;

  g_return_val_if_fail (PHOSH_IS_LOCKSCREEN (self), PHOSH_LOCKSCREEN_PAGE_DEFAULT);
  priv = phosh_lockscreen_get_instance_private (self);
  position = hdy_carousel_get_position (HDY_CAROUSEL (priv->carousel));

  if (position <= 0)
    return PHOSH_LOCKSCREEN_PAGE_DEFAULT;
  else
    return PHOSH_LOCKSCREEN_PAGE_UNLOCK;
}

/*
 * phosh_lockscreen_set_page
 * @self: The #PhoshLockscreen
 * PhoshLockscreenPage: the page to scroll to
 *
 * Scrolls to a specific page in the carousel. The state of the deck isn't changed.
 */
void
phosh_lockscreen_set_page (PhoshLockscreen *self, PhoshLockscreenPage page)
{
  GtkWidget *scroll_to;
  PhoshLockscreenPrivate *priv;

  g_return_if_fail (PHOSH_IS_LOCKSCREEN (self));
  priv = phosh_lockscreen_get_instance_private (self);

  scroll_to = (page == PHOSH_LOCKSCREEN_PAGE_UNLOCK) ? priv->box_unlock : priv->box_info;

  hdy_carousel_scroll_to (HDY_CAROUSEL (priv->carousel), scroll_to);
}
