/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-home"

#include "config.h"
#include "arrow.h"
#include "overview.h"
#include "home.h"
#include "shell.h"
#include "phosh-enums.h"
#include "osk/osk-button.h"

#define HANDY_USE_UNSTABLE_API
#include <handy.h>

/**
 * SECTION:phosh-home
 * @short_description: The home screen (sometimes called overview)  and the corrsponding
 * button at the bottom of the screen
 * @Title: PhoshHome
 *
 * The #PhoshHome is displayed at the bottom of the screen. It features
 * the home button and the button to toggle the OSK.
 */
enum {
  OSK_ACTIVATED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


enum {
  PROP_0,
  PROP_HOME_STATE,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];


struct _PhoshHome
{
  PhoshLayerSurface parent;

  GtkWidget *btn_home;
  GtkWidget *arrow_home;
  GtkWidget *btn_osk;
  GtkWidget *overview;

  struct {
    gdouble progress;
    gint64 last_frame;
  } animation;

  PhoshHomeState state;
};
G_DEFINE_TYPE(PhoshHome, phosh_home, PHOSH_TYPE_LAYER_SURFACE);


static void
phosh_home_set_property (GObject *object,
                          guint property_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
  PhoshHome *self = PHOSH_HOME (object);

  switch (property_id) {
    case PROP_HOME_STATE:
      self->state = g_value_get_enum (value);
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HOME_STATE]);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_home_get_property (GObject *object,
                          guint property_id,
                          GValue *value,
                          GParamSpec *pspec)
{
  PhoshHome *self = PHOSH_HOME (object);

  switch (property_id) {
    case PROP_HOME_STATE:
      g_value_set_enum (value, self->state);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_home_resize (PhoshHome *self)
{
  gint margin;
  gint height;
  gdouble progress = hdy_ease_out_cubic (self->animation.progress);

  if (self->state == PHOSH_HOME_STATE_UNFOLDED)
    progress = 1.0 - progress;

  phosh_arrow_set_progress (PHOSH_ARROW (self->arrow_home), 1 - progress);

  gtk_window_get_size (GTK_WINDOW (self), NULL, &height);
  margin = (-height + PHOSH_HOME_BUTTON_HEIGHT) * progress;

  phosh_layer_surface_set_margins (PHOSH_LAYER_SURFACE (self), 0, 0, margin, 0);
  /* Adjust the exclusive zone since exclusive zone includes margins.
     We don't want to change the effective exclusive zone at all to
     prevent all clients from being resized. */
  phosh_layer_surface_set_exclusive_zone (PHOSH_LAYER_SURFACE (self),
                                          -margin + PHOSH_HOME_BUTTON_HEIGHT);

  phosh_layer_surface_wl_surface_commit (PHOSH_LAYER_SURFACE (self));
}


static void
home_clicked_cb (PhoshHome *self, GtkButton *btn)
{
  g_return_if_fail (PHOSH_IS_HOME (self));
  g_return_if_fail (GTK_IS_BUTTON (btn));

  phosh_home_set_state (self, !self->state);
}


static void
osk_clicked_cb (PhoshHome *self, GtkButton *btn)
{
  g_return_if_fail (PHOSH_IS_HOME (self));
  g_return_if_fail (GTK_IS_BUTTON (btn));
  g_signal_emit(self, signals[OSK_ACTIVATED], 0);
}


static void
fold_cb (PhoshHome *self, PhoshOverview *overview)
{
  g_return_if_fail (PHOSH_IS_HOME (self));
  g_return_if_fail (PHOSH_IS_OVERVIEW (overview));

  phosh_home_set_state (self, PHOSH_HOME_STATE_FOLDED);
}

static gboolean
key_press_event_cb (PhoshHome *self, GdkEventKey *event, gpointer data)
{
  gboolean handled = FALSE;
  g_return_val_if_fail (PHOSH_IS_HOME (self), FALSE);

  switch (event->keyval) {
    case GDK_KEY_Escape:
      phosh_home_set_state (self, PHOSH_HOME_STATE_FOLDED);
      handled = TRUE;
      break;
    default:
      /* nothing to do */
      break;
  }

  return handled;
}


static void
phosh_home_constructed (GObject *object)
{
  PhoshHome *self = PHOSH_HOME (object);

  g_signal_connect_object (self->btn_home,
                           "clicked",
                           G_CALLBACK (home_clicked_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->btn_osk,
                           "clicked",
                           G_CALLBACK (osk_clicked_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_swapped (self->overview,
                            "activity-launched",
                            G_CALLBACK(fold_cb),
                            self);
  g_signal_connect_swapped (self->overview,
                            "activity-raised",
                            G_CALLBACK(fold_cb),
                            self);
  g_signal_connect_swapped (self->overview,
                            "selection-aborted",
                            G_CALLBACK(fold_cb),
                            self);

  g_signal_connect (self,
                    "size-allocate",
                    G_CALLBACK (phosh_home_resize),
                    NULL);

  gtk_widget_add_events (GTK_WIDGET (self), GDK_KEY_PRESS_MASK);
  g_signal_connect (G_OBJECT (self),
                    "key_press_event",
                    G_CALLBACK (key_press_event_cb),
                    NULL);

  G_OBJECT_CLASS (phosh_home_parent_class)->constructed (object);
}


static void
phosh_home_class_init (PhoshHomeClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_home_constructed;

  object_class->set_property = phosh_home_set_property;
  object_class->get_property = phosh_home_get_property;

  signals[OSK_ACTIVATED] = g_signal_new ("osk-activated",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  props[PROP_HOME_STATE] =
    g_param_spec_enum ("state",
                       "Home State",
                       "The state of the home screen",
                       PHOSH_TYPE_HOME_STATE,
                       PHOSH_HOME_STATE_FOLDED,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  g_type_ensure (PHOSH_TYPE_ARROW);
  g_type_ensure (PHOSH_TYPE_OSK_BUTTON);
  g_type_ensure (PHOSH_TYPE_OVERVIEW);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/home.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, btn_home);
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, arrow_home);
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, btn_osk);
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, overview);

  gtk_widget_class_set_css_name (widget_class, "phosh-home");
}


static void
phosh_home_init (PhoshHome *self)
{
  self->state = PHOSH_HOME_STATE_FOLDED;
  self->animation.progress = 1.0;

  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_home_new (struct zwlr_layer_shell_v1 *layer_shell,
                struct wl_output *wl_output)
{
  return g_object_new (PHOSH_TYPE_HOME,
                       "layer-shell", layer_shell,
                       "wl-output", wl_output,
                       "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_TOP,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", PHOSH_HOME_BUTTON_HEIGHT,
                       "namespace", "phosh home",
                       NULL);
}


static gboolean
animate_cb(GtkWidget *widget,
           GdkFrameClock *frame_clock,
           gpointer user_data)
{
  gint64 time;
  gboolean finished = FALSE;
  PhoshHome *self = PHOSH_HOME (widget);

  time = gdk_frame_clock_get_frame_time (frame_clock) - self->animation.last_frame;
  if (self->animation.last_frame < 0) {
    time = 0;
  }

  self->animation.progress += 0.06666 * time / 16666.00;
  self->animation.last_frame = gdk_frame_clock_get_frame_time (frame_clock);

  if (self->animation.progress >= 1.0) {
    finished = TRUE;
    self->animation.progress = 1.0;
  }

  phosh_home_resize (self);

  return finished ? G_SOURCE_REMOVE : G_SOURCE_CONTINUE;
}

/**
 * phosh_home_set_state:
 *
 * Set the state of the home screen. See #PhoshHomeState.
 */
void
phosh_home_set_state (PhoshHome *self, PhoshHomeState state)
{
  gboolean enable_animations;
  gboolean kbd_interactivity;

  g_return_if_fail (PHOSH_IS_HOME (self));

  if (self->state == state)
    return;

  enable_animations = hdy_get_enable_animations (GTK_WIDGET (self));

  self->state = state;
  g_debug ("Setting state to %s", g_enum_to_string (PHOSH_TYPE_HOME_STATE, state));

  self->animation.last_frame = -1;
  self->animation.progress = enable_animations ? (1.0 - self->animation.progress) : 1.0;
  gtk_widget_add_tick_callback (GTK_WIDGET (self), animate_cb, NULL, NULL);

  if (state == PHOSH_HOME_STATE_UNFOLDED) {
    gtk_widget_hide (self->btn_osk);
    kbd_interactivity = TRUE;
  } else {
    gtk_widget_show (self->btn_osk);
    kbd_interactivity = FALSE;
  }
  phosh_layer_surface_set_kbd_interactivity (PHOSH_LAYER_SURFACE (self), kbd_interactivity);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HOME_STATE]);
}
