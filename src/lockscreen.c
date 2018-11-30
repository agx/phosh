/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-lockscreen"

#include "config.h"
#include "auth.h"
#include "lockscreen.h"
#include "wwaninfo.h"
#include "batteryinfo.h"

#include <string.h>
#include <glib/gi18n.h>
#include <math.h>

#define HANDY_USE_UNSTABLE_API
#include <handy.h>

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-wall-clock.h>

#define LOCKSCREEN_IDLE_SECONDS 5

enum {
  LOCKSCREEN_UNLOCK,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


typedef struct _PhoshLockscreen
{
  PhoshLayerSurface parent;
} PhoshLockscreen;


typedef struct {
  GtkWidget *stack;

  /* info page */
  GtkWidget *ebox_info;
  GtkWidget *grid_info;
  GtkWidget *lbl_clock;
  GtkGesture *ebox_pan_gesture;

  /* unlock page */
  GtkWidget *grid_unlock;
  GtkWidget *dialer_keypad;
  GtkWidget *lbl_keypad;
  GtkWidget *lbl_unlock_status;
  GtkWidget *wwaninfo;
  GtkWidget *batteryinfo;
  guint      idle_timer;
  gint64     last_input;
  PhoshAuth *auth;

  GnomeWallClock *wall_clock;
} PhoshLockscreenPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshLockscreen, phosh_lockscreen, PHOSH_TYPE_LAYER_SURFACE)


static void
keypad_update_labels (PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);
  gint len;
  gchar *pos;
  const gchar *number;
  g_autofree gchar *dots = NULL;

  number = hdy_dialer_get_number (HDY_DIALER (priv->dialer_keypad));
  len = strlen (number);
  dots = pos = g_malloc0 (len * 3 + 1);
  g_return_if_fail (dots);
  for (int i = 0; i < len; i++)
    pos = g_stpcpy (pos, "●");
  gtk_label_set_text (GTK_LABEL (priv->lbl_keypad), dots);
  gtk_label_set_label (GTK_LABEL (priv->lbl_unlock_status), _("Enter PIN to unlock"));
}


static void
show_info_page (PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);

  hdy_dialer_clear_number (HDY_DIALER (priv->dialer_keypad));
  keypad_update_labels (self);
  gtk_stack_set_visible_child (GTK_STACK (priv->stack), priv->ebox_info);
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

  gtk_stack_set_visible_child (GTK_STACK (priv->stack), priv->grid_unlock);
  hdy_dialer_set_show_action_buttons (HDY_DIALER (priv->dialer_keypad), FALSE);
  gtk_widget_grab_focus (GTK_WIDGET (priv->dialer_keypad));
  if (!priv->idle_timer) {
    priv->last_input = g_get_monotonic_time ();
    priv->idle_timer = g_timeout_add_seconds (LOCKSCREEN_IDLE_SECONDS,
                                              (GSourceFunc) keypad_check_idle,
                                              self);
  }
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
    /* FIXME: give visual feedback */
    hdy_dialer_clear_number (HDY_DIALER (priv->dialer_keypad));
    gtk_widget_set_sensitive (priv->dialer_keypad, TRUE);
    gtk_widget_grab_focus (GTK_WIDGET (priv->dialer_keypad));
  }
  /* FIXME: must clear out the buffer and use secmem, see secmem branch */
  priv->last_input = g_get_monotonic_time ();
  g_object_unref (self);
}


static void
keypad_number_notified_cb (PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv;
  const gchar *number;

  g_assert (PHOSH_IS_LOCKSCREEN (self));
  priv = phosh_lockscreen_get_instance_private (self);
  number = hdy_dialer_get_number (HDY_DIALER (priv->dialer_keypad));
  priv->last_input = g_get_monotonic_time ();

  keypad_update_labels (self);
  if (strlen (number) == phosh_auth_get_pin_length()) {
    gtk_label_set_label (GTK_LABEL (priv->lbl_unlock_status), _("Checking PIN"));
    gtk_widget_set_sensitive (priv->dialer_keypad, FALSE);

    if (priv->auth == NULL)
      priv->auth = PHOSH_AUTH (phosh_auth_new ());
    phosh_auth_authenticate_async_start (priv->auth,
                                         number,
                                         NULL,
                                         (GAsyncReadyCallback)auth_async_cb,
                                         self);
  }
}


static void
info_pan_cb (PhoshLockscreen *self, GtkPanDirection dir, gdouble offset, GtkGesturePan *pan)
{
  if (dir == GTK_PAN_DIRECTION_UP)
    show_unlock_page (self);
}


static gboolean
key_press_event_cb (PhoshLockscreen *self, GdkEventKey *event, gpointer data)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);
  gboolean handled = FALSE;
  g_autofree gchar *number = NULL;

  if (gtk_stack_get_visible_child (GTK_STACK (priv->stack)) == priv->ebox_info) {
    switch (event->keyval) {
    case GDK_KEY_space:
      show_unlock_page (self);
      handled = TRUE;
      break;
    case GDK_KEY_1...GDK_KEY_9:
      number = g_strdup_printf ("%d", event->keyval - GDK_KEY_1 + 1);
      hdy_dialer_set_number (HDY_DIALER (priv->dialer_keypad), number);
      show_unlock_page (self);
      handled = TRUE;
      break;
    default:
      /* nothing to do */
      break;
    }
  } else if (gtk_stack_get_visible_child (GTK_STACK (priv->stack)) == priv->grid_unlock) {
    switch (event->keyval) {
    case GDK_KEY_Escape:
      show_info_page (self);
      handled = TRUE;
      break;
    default:
      /* nothing to do */
      break;
    }
  }
  return handled;
}


static void
wall_clock_notify_cb (PhoshLockscreen *self,
                      GParamSpec *pspec,
                      GnomeWallClock *wall_clock)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);
  const gchar *str;

  str = gnome_wall_clock_get_clock(wall_clock);
  gtk_label_set_text (GTK_LABEL (priv->lbl_clock), str);
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

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "phosh-lockscreen");

  gtk_stack_set_visible_child (GTK_STACK (priv->stack), priv->ebox_info);

  priv->ebox_pan_gesture = gtk_gesture_pan_new (GTK_WIDGET (priv->ebox_info),
                                                GTK_ORIENTATION_VERTICAL);
  g_signal_connect_swapped (priv->ebox_pan_gesture,
                            "pan",
                            G_CALLBACK (info_pan_cb),
                            self);

  gtk_widget_add_events (GTK_WIDGET (self), GDK_KEY_PRESS_MASK);
  g_signal_connect (G_OBJECT (self),
                    "key_press_event",
                    G_CALLBACK (key_press_event_cb),
                    NULL);

  priv->wall_clock = g_object_new (GNOME_TYPE_WALL_CLOCK, NULL);
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

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/lockscreen.ui");
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, stack);

  /* unlock page */
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, grid_unlock);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, dialer_keypad);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, lbl_keypad);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, lbl_unlock_status);
  gtk_widget_class_bind_template_callback_full (widget_class,
                                                "keypad_number_notified_cb",
                                                G_CALLBACK(keypad_number_notified_cb));
  /* info page */
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, ebox_info);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, grid_info);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, lbl_clock);
  PHOSH_TYPE_WWAN_INFO; /* make sure the type is known */
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, wwaninfo);
  PHOSH_TYPE_BATTERY_INFO; /* make sure the type is known */
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, batteryinfo);
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
