/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * based on gvc-channel-bar.h from g-c-c which is
 * Copyright (C) 2008 Red Hat, Inc.
 */

#define G_LOG_DOMAIN "phosh-settings-volctrl"

#include "phosh-config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include <pulse/pulseaudio.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "gvc-channel-bar.h"
#include "gvc-mixer-control.h"

#define SCALE_SIZE 128
#define ADJUSTMENT_MAX_NORMAL PA_VOLUME_NORM
#define ADJUSTMENT_MAX_AMPLIFIED PA_VOLUME_UI_MAX
#define ADJUSTMENT_MAX (self->is_amplified ? ADJUSTMENT_MAX_AMPLIFIED : ADJUSTMENT_MAX_NORMAL)
#define SCROLLSTEP (ADJUSTMENT_MAX / 100.0 * 5.0)


enum
{
  PROP_0,
  PROP_IS_MUTED,
  PROP_ICON_NAME,
  PROP_IS_AMPLIFIED,
  LAST_PROP,
};
static GParamSpec *props[LAST_PROP];


enum {
  VALUE_CHANGED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


struct _GvcChannelBar
{
  GtkBox         parent_instance;

  GtkWidget     *scale_box;
  GtkWidget     *image;
  GtkWidget     *scale;
  GtkAdjustment *adjustment;
  gboolean       is_muted;
  char          *icon_name;
  GtkSizeGroup  *size_group;
  gboolean       click_lock;
  gboolean       is_amplified;
  guint32        base_volume;
};

G_DEFINE_TYPE (GvcChannelBar, gvc_channel_bar, GTK_TYPE_BOX)


static void
update_image (GvcChannelBar *self)
{
  g_autoptr (GIcon) gicon = NULL;

  if (self->icon_name) {
      gicon = g_themed_icon_new_with_default_fallbacks (self->icon_name);
      gtk_image_set_from_gicon (GTK_IMAGE (self->image), gicon, -1);
  }

  gtk_widget_set_visible (self->image, self->icon_name != NULL);
}


void
gvc_channel_bar_set_size_group (GvcChannelBar *self,
                                GtkSizeGroup  *group)
{
  g_return_if_fail (GVC_IS_CHANNEL_BAR (self));

  self->size_group = group;

  if (self->size_group != NULL)
    gtk_size_group_add_widget (self->size_group, self->scale_box);

  gtk_widget_queue_draw (GTK_WIDGET (self));
}


void
gvc_channel_bar_set_icon_name (GvcChannelBar  *self,
                               const char     *name)
{
  g_return_if_fail (GVC_IS_CHANNEL_BAR (self));

  if (g_strcmp0 (self->icon_name, name) == 0)
    return;

  g_free (self->icon_name);
  self->icon_name = g_strdup (name);
  update_image (self);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ICON_NAME]);
}


GtkAdjustment *
gvc_channel_bar_get_adjustment (GvcChannelBar *self)
{
  g_return_val_if_fail (GVC_IS_CHANNEL_BAR (self), NULL);

  return self->adjustment;
}


static gboolean
on_scale_button_press_event (GtkWidget      *widget,
                             GdkEventButton *event,
                             GvcChannelBar  *self)
{
  self->click_lock = TRUE;

  return FALSE;
}


static gboolean
on_scale_button_release_event (GtkWidget      *widget,
                               GdkEventButton *event,
                               GvcChannelBar  *self)
{
  double value;

  self->click_lock = FALSE;
  value = gtk_adjustment_get_value (self->adjustment);

  /* this means the adjustment moved away from zero and
   * therefore we should unmute and set the volume. */
  gvc_channel_bar_set_is_muted (self, ((int)value == (int)0.0));

  return FALSE;
}


gboolean
gvc_channel_bar_scroll (GvcChannelBar *self, GdkEventScroll *event)
{
  double value;
  GdkScrollDirection direction;
  double dx, dy;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (GVC_IS_CHANNEL_BAR (self), FALSE);

  direction = event->direction;

  /* Switch direction for RTL */
  if (gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL) {
    if (direction == GDK_SCROLL_RIGHT)
      direction = GDK_SCROLL_LEFT;
    else if (direction == GDK_SCROLL_LEFT)
      direction = GDK_SCROLL_RIGHT;
  }
  /* Switch side scroll to vertical */
  if (direction == GDK_SCROLL_RIGHT)
    direction = GDK_SCROLL_UP;
  else if (direction == GDK_SCROLL_LEFT)
    direction = GDK_SCROLL_DOWN;

  if (!gdk_event_get_scroll_deltas ((GdkEvent*)event, &dx, &dy)) {
    dx = 0.0;
    dy = 0.0;

    switch (direction) {
    case GDK_SCROLL_UP:
    case GDK_SCROLL_LEFT:
      dy = 1.0;
      break;
    case GDK_SCROLL_DOWN:
    case GDK_SCROLL_RIGHT:
      dy = -1.0;
      break;
    case GDK_SCROLL_SMOOTH:
    default:
      ;
    }
  }

  value = gtk_adjustment_get_value (self->adjustment);

  if (dy > 0) {
    if (value + dy * SCROLLSTEP > ADJUSTMENT_MAX)
      value = ADJUSTMENT_MAX;
    else
      value = value + dy * SCROLLSTEP;
  } else if (dy < 0) {
    if (value + dy * SCROLLSTEP < 0)
      value = 0.0;
    else
      value = value + dy * SCROLLSTEP;
  }

  gvc_channel_bar_set_is_muted (self, ((int) value == 0));
  gtk_adjustment_set_value (self->adjustment, value);

  return TRUE;
}


static gboolean
on_scale_scroll_event (GtkWidget      *widget,
                       GdkEventScroll *event,
                       GvcChannelBar  *self)
{
  return gvc_channel_bar_scroll (self, event);
}


static void
on_adjustment_value_changed (GtkAdjustment *adjustment, GvcChannelBar *self)
{
  if (!self->is_muted || self->click_lock)
    g_signal_emit (self, signals[VALUE_CHANGED], 0);
}


void
gvc_channel_bar_set_is_muted (GvcChannelBar *self,
                              gboolean       is_muted)
{
  g_return_if_fail (GVC_IS_CHANNEL_BAR (self));

  if (is_muted == self->is_muted)
    return;

  /* Update our internal state before telling the
   * front-end about our changes */
  self->is_muted = is_muted;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_IS_MUTED]);

  if (is_muted)
    gtk_adjustment_set_value (self->adjustment, 0.0);
}


gboolean
gvc_channel_bar_get_is_muted  (GvcChannelBar *self)
{
  g_return_val_if_fail (GVC_IS_CHANNEL_BAR (self), FALSE);
  return self->is_muted;
}


void
gvc_channel_bar_set_is_amplified (GvcChannelBar *self, gboolean amplified)
{
  g_return_if_fail (GVC_IS_CHANNEL_BAR (self));

  if (self->is_amplified == amplified)
    return;

  self->is_amplified = amplified;
  gtk_adjustment_set_upper (self->adjustment, ADJUSTMENT_MAX);
  gtk_scale_clear_marks (GTK_SCALE (self->scale));

  if (amplified) {
    g_autofree char *str = NULL;

    if (G_APPROX_VALUE (self->base_volume, floor (ADJUSTMENT_MAX_NORMAL), DBL_EPSILON)) {
      str = g_strdup_printf ("<small>%s</small>", C_("volume", "100%"));
      gtk_scale_add_mark (GTK_SCALE (self->scale), ADJUSTMENT_MAX_NORMAL,
                          GTK_POS_BOTTOM, str);
    } else {
      str = g_strdup_printf ("<small>%s</small>", C_("volume", "Unamplified"));
      gtk_scale_add_mark (GTK_SCALE (self->scale), self->base_volume,
                          GTK_POS_BOTTOM, str);
      /* Only show 100% if it's higher than the base volume */
      if (self->base_volume < ADJUSTMENT_MAX_NORMAL) {
        str = g_strdup_printf ("<small>%s</small>", C_("volume", "100%"));
        gtk_scale_add_mark (GTK_SCALE (self->scale), ADJUSTMENT_MAX_NORMAL,
                            GTK_POS_BOTTOM, str);
      }
    }

    /* Ideally we would use baseline alignment for all
     * these widgets plus the scale but neither GtkScale
     * nor GtkSwitch support baseline alignment yet. */
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_IS_AMPLIFIED]);
}


void
gvc_channel_bar_set_base_volume (GvcChannelBar *self,
                                 pa_volume_t    base_volume)
{
  g_return_if_fail (GVC_IS_CHANNEL_BAR (self));

  if (base_volume == 0) {
    self->base_volume = ADJUSTMENT_MAX_NORMAL;
    return;
  }

  /* Note that you need to call _is_amplified() afterwards to update the marks */
  self->base_volume = base_volume;
}


static void
gvc_channel_bar_set_property (GObject       *object,
                              guint          prop_id,
                              const GValue  *value,
                              GParamSpec    *pspec)
{
  GvcChannelBar *self = GVC_CHANNEL_BAR (object);

  switch (prop_id) {
  case PROP_IS_MUTED:
    gvc_channel_bar_set_is_muted (self, g_value_get_boolean (value));
    break;
  case PROP_ICON_NAME:
    gvc_channel_bar_set_icon_name (self, g_value_get_string (value));
    break;
  case PROP_IS_AMPLIFIED:
    gvc_channel_bar_set_is_amplified (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}


static void
gvc_channel_bar_get_property (GObject     *object,
                              guint        prop_id,
                              GValue      *value,
                              GParamSpec  *pspec)
{
  GvcChannelBar *self = GVC_CHANNEL_BAR (object);

  switch (prop_id) {
  case PROP_IS_MUTED:
    g_value_set_boolean (value, self->is_muted);
    break;
  case PROP_ICON_NAME:
    g_value_set_string (value, self->icon_name);
    break;
  case PROP_IS_AMPLIFIED:
    g_value_set_boolean (value, self->is_amplified);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}


static void
gvc_channel_bar_finalize (GObject *object)
{
  GvcChannelBar *self = GVC_CHANNEL_BAR (object);

  g_free (self->icon_name);

  G_OBJECT_CLASS (gvc_channel_bar_parent_class)->finalize (object);
}


static void
gvc_channel_bar_class_init (GvcChannelBarClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = gvc_channel_bar_finalize;
  object_class->set_property = gvc_channel_bar_set_property;
  object_class->get_property = gvc_channel_bar_get_property;

  /**
   * GvcChannelBar:is-muted:
   *
   * Whether the stream is muted
   */
  props[PROP_IS_MUTED] =
    g_param_spec_boolean ("is-muted", "", "",
                          FALSE,
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT);
  /**
   * GvcChannelBar:icon-name:
   *
   * The name of icon to display for this stream
   */
  props[PROP_ICON_NAME] =
    g_param_spec_string ("icon-name", "", "",
                         NULL,
                         G_PARAM_EXPLICIT_NOTIFY |
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT);
  /**
   * GvcChannelBar:is-amplified:
   *
   * Whether the stream is digitally amplified
   */
  props[PROP_IS_AMPLIFIED] =
    g_param_spec_boolean ("is-amplified", "", "",
                          FALSE,
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  signals[VALUE_CHANGED] = g_signal_new ("value-changed",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL, NULL,
                                         G_TYPE_NONE,
                                         0);

  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/phosh/ui/gvc-channel-bar.ui");
  gtk_widget_class_bind_template_child (widget_class, GvcChannelBar, adjustment);
  gtk_widget_class_bind_template_child (widget_class, GvcChannelBar, scale_box);
  gtk_widget_class_bind_template_child (widget_class, GvcChannelBar, image);
  gtk_widget_class_bind_template_child (widget_class, GvcChannelBar, scale);
  gtk_widget_class_bind_template_callback (widget_class, on_adjustment_value_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_scale_button_press_event);
  gtk_widget_class_bind_template_callback (widget_class, on_scale_button_release_event);
  gtk_widget_class_bind_template_callback (widget_class, on_scale_scroll_event);

  gtk_widget_class_set_css_name (widget_class, "phosh-gvc-channel-bar");
}


static void
gvc_channel_bar_init (GvcChannelBar *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->base_volume = ADJUSTMENT_MAX_NORMAL;
  gtk_adjustment_set_upper (self->adjustment, ADJUSTMENT_MAX_NORMAL);
  gtk_adjustment_set_step_increment (self->adjustment, ADJUSTMENT_MAX_NORMAL / 100.0);
  gtk_adjustment_set_page_increment (self->adjustment, ADJUSTMENT_MAX_NORMAL / 10.0);

  gtk_widget_add_events (self->scale, GDK_SCROLL_MASK);
}


GtkWidget *
gvc_channel_bar_new (void)
{
  return g_object_new (GVC_TYPE_CHANNEL_BAR,
                       "orientation", GTK_ORIENTATION_HORIZONTAL,
                       "icon-name", "audio-speakers-symbolic",
                       NULL);
}


double
gvc_channel_bar_get_volume (GvcChannelBar *self)
{
  g_return_val_if_fail (GVC_IS_CHANNEL_BAR (self), 0.0);

  return gtk_adjustment_get_value (self->adjustment);
}
