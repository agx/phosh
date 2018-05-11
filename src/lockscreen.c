/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "config.h"
#include "lockscreen.h"
#include "wwaninfo.h"

#include <string.h>
#include <glib/gi18n.h>
#include <math.h>

#define HANDY_USE_UNSTABLE_API
#include <libhandy-0.0/handy.h>

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-wall-clock.h>

#define TEST_PIN "1234"
#define LOCKSCREEN_IDLE_SECONDS 5

enum {
  LOCKSCREEN_UNLOCK,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


typedef struct _PhoshLockscreen
{
  GtkWindow parent;
} PhoshLockscreen;


typedef struct PhoshLockscreen {
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
  guint      idle_timer;
  gint64     last_input;

  GnomeWallClock *wall_clock;
} PhoshLockscreenPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshLockscreen, phosh_lockscreen, GTK_TYPE_WINDOW)


static void
show_info_page (PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);

  hdy_dialer_clear_number (HDY_DIALER (priv->dialer_keypad));
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
  if (!priv->idle_timer) {
    priv->last_input = g_get_monotonic_time ();
    priv->idle_timer = g_timeout_add_seconds (LOCKSCREEN_IDLE_SECONDS,
                                              (GSourceFunc) keypad_check_idle,
                                              self);
  }
}


static void
keypad_update_labels (PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);
  const gchar *number;
  g_autofree gchar *stars = NULL;

  number = hdy_dialer_get_number (HDY_DIALER (priv->dialer_keypad));
  stars = g_strnfill (strlen(number), '*');
  gtk_label_set_label (GTK_LABEL (priv->lbl_keypad), stars);
  gtk_label_set_label (GTK_LABEL (priv->lbl_unlock_status), _("Enter PIN to unlock"));
}


static void
keypad_number_notified_cb (PhoshLockscreen *self)
{
  PhoshLockscreenPrivate *priv;
  const gchar *number;

  g_assert (PHOSH_IS_LOCKSCREEN (self));

  priv = phosh_lockscreen_get_instance_private (self);

  number = hdy_dialer_get_number (HDY_DIALER (priv->dialer_keypad));
  if (strlen (number) == strlen (TEST_PIN)) {
    if (!g_strcmp0 (number, TEST_PIN)) {
      /* FIXME: handle real PIN
       * https://code.puri.sm/Librem5/phosh/issues/25
       */
      g_signal_emit(self, signals[LOCKSCREEN_UNLOCK], 0);
    } else {
      gtk_label_set_label (GTK_LABEL (priv->lbl_unlock_status), _("Wrong PIN"));
      hdy_dialer_clear_number (HDY_DIALER (priv->dialer_keypad));
    }
  }
  priv->last_input = g_get_monotonic_time ();
}


static void
keypad_symbol_clicked_cb (PhoshLockscreen *self, gchar symbol, HdyDialer *keypad)
{
  PhoshLockscreenPrivate *priv = phosh_lockscreen_get_instance_private (self);
  g_assert (PHOSH_IS_LOCKSCREEN (self));
  g_assert (HDY_IS_DIALER (keypad));

  keypad_update_labels (self);
  priv->last_input = g_get_monotonic_time ();
}


static void
info_pan_cb (PhoshLockscreen *self, GtkPanDirection dir, gdouble offset, GtkGesturePan *pan)
{
  if (dir == GTK_PAN_DIRECTION_UP)
    show_unlock_page (self);
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

  GdkDisplay *display = gdk_display_get_default ();
  /* There's no primary monitor on nested wayland so just use the
     first one for now */
  GdkMonitor *monitor = gdk_display_get_monitor (display, 0);
  GdkRectangle geom;

  G_OBJECT_CLASS (phosh_lockscreen_parent_class)->constructed (object);

  g_return_if_fail(monitor);
  gdk_monitor_get_geometry (monitor, &geom);

  /* window properties */
  gtk_window_set_title (GTK_WINDOW (self), "phosh lockscreen");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_window_resize (GTK_WINDOW (self), geom.width, geom.height);
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
                                                "keypad_symbol_clicked_cb",
                                                G_CALLBACK(keypad_symbol_clicked_cb));
  gtk_widget_class_bind_template_callback_full (widget_class,
                                                "keypad_number_notified_cb",
                                                G_CALLBACK(keypad_number_notified_cb));


  /* info page */
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, ebox_info);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, grid_info);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, lbl_clock);
  PHOSH_TYPE_WWAN_INFO; /* make sure the type is known */
  gtk_widget_class_bind_template_child_private (widget_class, PhoshLockscreen, wwaninfo);
}


static void
phosh_lockscreen_init (PhoshLockscreen *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_lockscreen_new (void)
{
  return g_object_new (PHOSH_LOCKSCREEN_TYPE, NULL);
}
