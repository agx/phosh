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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <pulse/pulseaudio.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "gvc-channel-bar.h"
#include "gvc-mixer-control.h"

#define SCALE_SIZE 128
#define ADJUSTMENT_MAX_NORMAL gvc_mixer_control_get_vol_max_norm(NULL)
#define ADJUSTMENT_MAX_AMPLIFIED gvc_mixer_control_get_vol_max_amplified(NULL)
#define ADJUSTMENT_MAX (self->is_amplified ? ADJUSTMENT_MAX_AMPLIFIED : ADJUSTMENT_MAX_NORMAL)
#define SCROLLSTEP (ADJUSTMENT_MAX / 100.0 * 5.0)

struct _GvcChannelBar
{
  GtkBox         parent_instance;

  GtkWidget     *scale_box;
  GtkWidget     *image;
  GtkWidget     *scale;
  GtkAdjustment *adjustment;
  GtkAdjustment *zero_adjustment;
  gboolean       is_muted;
  char          *icon_name;
  GtkSizeGroup  *size_group;
  gboolean       click_lock;
  gboolean       is_amplified;
  guint32        base_volume;
};

enum
{
  PROP_0,
  PROP_IS_MUTED,
  PROP_ADJUSTMENT,
  PROP_ICON_NAME,
  PROP_IS_AMPLIFIED,
};

static gboolean on_scale_button_press_event   (GtkWidget      *widget,
                                               GdkEventButton *event,
                                               GvcChannelBar  *self);
static gboolean on_scale_button_release_event (GtkWidget      *widget,
                                               GdkEventButton *event,
                                               GvcChannelBar  *self);
static gboolean on_scale_scroll_event         (GtkWidget      *widget,
                                               GdkEventScroll *event,
                                               GvcChannelBar  *self);

G_DEFINE_TYPE (GvcChannelBar, gvc_channel_bar, GTK_TYPE_BOX)


static GtkWidget *
_scale_box_new (GvcChannelBar *self)
{
  GtkWidget            *box;

  self->scale_box = box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (box), self->image, FALSE, FALSE, 0);

  self->scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, self->adjustment);

  gtk_widget_set_size_request (self->scale, SCALE_SIZE, -1);
  gtk_box_pack_start (GTK_BOX (box), self->scale, TRUE, TRUE, 0);

  gtk_widget_add_events (self->scale, GDK_SCROLL_MASK);

  g_signal_connect (G_OBJECT (self->scale), "button-press-event",
                    G_CALLBACK (on_scale_button_press_event), self);
  g_signal_connect (G_OBJECT (self->scale), "button-release-event",
                    G_CALLBACK (on_scale_button_release_event), self);
  g_signal_connect (G_OBJECT (self->scale), "scroll-event",
                    G_CALLBACK (on_scale_scroll_event), self);

  if (self->size_group != NULL)
    gtk_size_group_add_widget (self->size_group, box);

  gtk_scale_set_draw_value (GTK_SCALE (self->scale), FALSE);

  return box;
}


static void
update_image (GvcChannelBar *self)
{
  gtk_image_set_from_icon_name (GTK_IMAGE (self->image),
                                self->icon_name,
                                GTK_ICON_SIZE_BUTTON);

  if (self->icon_name != NULL)
    gtk_widget_show (self->image);
  else
    gtk_widget_hide (self->image);
}


void
gvc_channel_bar_set_size_group (GvcChannelBar *self,
                                GtkSizeGroup  *group)
{
  g_return_if_fail (GVC_IS_CHANNEL_BAR (self));

  self->size_group = group;

  if (self->size_group != NULL) {
    gtk_size_group_add_widget (self->size_group,
                               self->scale_box);
  }
  gtk_widget_queue_draw (GTK_WIDGET (self));
}


void
gvc_channel_bar_set_icon_name (GvcChannelBar  *self,
                               const char     *name)
{
  g_return_if_fail (GVC_IS_CHANNEL_BAR (self));

  g_free (self->icon_name);
  self->icon_name = g_strdup (name);
  update_image (self);
  g_object_notify (G_OBJECT (self), "icon-name");
}


static void
gvc_channel_bar_set_adjustment (GvcChannelBar *self,
                                GtkAdjustment *adjustment)
{
  g_return_if_fail (GVC_CHANNEL_BAR (self));
  g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  if (self->adjustment != NULL) {
    g_object_unref (self->adjustment);
  }
  self->adjustment = g_object_ref_sink (adjustment);

  if (self->scale != NULL) {
    gtk_range_set_adjustment (GTK_RANGE (self->scale), adjustment);
  }

  g_object_notify (G_OBJECT (self), "adjustment");
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
  GtkAdjustment *adj;
  double value;

  self->click_lock = FALSE;

  adj = gtk_range_get_adjustment (GTK_RANGE (widget));

  value = gtk_adjustment_get_value (adj);

  /* this means the adjustment moved away from zero and
   * therefore we should unmute and set the volume. */
  gvc_channel_bar_set_is_muted (self, ((int)value == (int)0.0));

  /* TODO: we might want to play a sound here */
  return FALSE;
}


gboolean
gvc_channel_bar_scroll (GvcChannelBar *self, GdkEventScroll *event)
{
  GtkAdjustment *adj;
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

  adj = gtk_range_get_adjustment (GTK_RANGE (self->scale));
  if (adj == self->zero_adjustment) {
    if (dy > 0)
      gvc_channel_bar_set_is_muted (self, FALSE);
    return TRUE;
  }

  value = gtk_adjustment_get_value (adj);

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
  adj = gtk_range_get_adjustment (GTK_RANGE (self->scale));
  gtk_adjustment_set_value (adj, value);

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
on_zero_adjustment_value_changed (GtkAdjustment *adjustment,
                                  GvcChannelBar *self)
{
  double value;

  if (self->click_lock != FALSE) {
    return;
  }

  value = gtk_adjustment_get_value (self->zero_adjustment);
  gtk_adjustment_set_value (self->adjustment, value);

  /* this means the adjustment moved away from zero and
   * therefore we should unmute and set the volume. */
  gvc_channel_bar_set_is_muted (self, value > 0.0);
}


void
gvc_channel_bar_set_is_muted (GvcChannelBar *self,
                              gboolean       is_muted)
{
  g_return_if_fail (GVC_IS_CHANNEL_BAR (self));

  if (is_muted != self->is_muted) {
    /* Update our internal state before telling the
     * front-end about our changes */
    self->is_muted = is_muted;
    g_object_notify (G_OBJECT (self), "is-muted");
  }
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

  self->is_amplified = amplified;
  gtk_adjustment_set_upper (self->adjustment, ADJUSTMENT_MAX);
  gtk_adjustment_set_upper (self->zero_adjustment, ADJUSTMENT_MAX);
  gtk_scale_clear_marks (GTK_SCALE (self->scale));

  if (amplified) {
    g_autofree char *str = NULL;

    if ((int)self->base_volume == (int)ADJUSTMENT_MAX_NORMAL) {
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
  case PROP_ADJUSTMENT:
    gvc_channel_bar_set_adjustment (self, g_value_get_object (value));
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
  case PROP_ADJUSTMENT:
    g_value_set_object (value, gvc_channel_bar_get_adjustment (self));
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
  GvcChannelBar *self;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GVC_IS_CHANNEL_BAR (object));

  self = GVC_CHANNEL_BAR (object);

  g_return_if_fail (self != NULL);

  g_free (self->icon_name);

  G_OBJECT_CLASS (gvc_channel_bar_parent_class)->finalize (object);
}


static void
gvc_channel_bar_class_init (GvcChannelBarClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gvc_channel_bar_finalize;
  object_class->set_property = gvc_channel_bar_set_property;
  object_class->get_property = gvc_channel_bar_get_property;

  g_object_class_install_property (object_class,
                                   PROP_IS_MUTED,
                                   g_param_spec_boolean ("is-muted",
                                                         "is muted",
                                                         "Whether stream is muted",
                                                         FALSE,
                                                         G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class,
                                   PROP_ADJUSTMENT,
                                   g_param_spec_object ("adjustment",
                                                        "Adjustment",
                                                        "The GtkAdjustment that contains the current value of this scale button object",
                                                        GTK_TYPE_ADJUSTMENT,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name",
                                                        "Icon Name",
                                                        "Name of icon to display for this stream",
                                                        NULL,
                                                        G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class,
                                   PROP_IS_AMPLIFIED,
                                   g_param_spec_boolean ("is-amplified",
                                                         "Is amplified",
                                                         "Whether the stream is digitally amplified",
                                                         FALSE,
                                                         G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
}


static void
gvc_channel_bar_init (GvcChannelBar *self)
{
  GtkWidget *frame;

  self->image = gtk_image_new ();

  self->base_volume = ADJUSTMENT_MAX_NORMAL;

  self->adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0,
                                                         0.0,
                                                         ADJUSTMENT_MAX_NORMAL,
                                                         ADJUSTMENT_MAX_NORMAL/100.0,
                                                         ADJUSTMENT_MAX_NORMAL/10.0,
                                                         0.0));
  g_object_ref_sink (self->adjustment);

  self->zero_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0,
                                                              0.0,
                                                              ADJUSTMENT_MAX_NORMAL,
                                                              ADJUSTMENT_MAX_NORMAL/100.0,
                                                              ADJUSTMENT_MAX_NORMAL/10.0,
                                                              0.0));
  g_object_ref_sink (self->zero_adjustment);

  g_signal_connect (self->zero_adjustment,
                    "value-changed",
                    G_CALLBACK (on_zero_adjustment_value_changed),
                    self);

  /* frame */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  gtk_box_pack_start (GTK_BOX (self), frame, TRUE, TRUE, 0);
  gtk_widget_show_all (frame);

  /* box with scale */
  self->scale_box = _scale_box_new (self);

  gtk_container_add (GTK_CONTAINER (frame), self->scale_box);
  gtk_widget_show_all (frame);
}


GtkWidget *
gvc_channel_bar_new (void)
{
  GObject *self;
  self = g_object_new (GVC_TYPE_CHANNEL_BAR,
                       "orientation", GTK_ORIENTATION_HORIZONTAL,
                       "icon-name", "audio-speakers-symbolic",
                       NULL);
  return GTK_WIDGET (self);
}
