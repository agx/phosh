/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-home"

#include "config.h"
#include "favorites.h"
#include "home.h"
#include "shell.h"
#include "phosh-enums.h"
#include "osk/osk-button.h"

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
  GtkWidget *img_home;
  GtkWidget *btn_osk;
  GtkWidget *favorites;

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
fold_cb (PhoshHome *self, PhoshFavorites *favorites)
{
  g_return_if_fail (PHOSH_IS_HOME (self));
  g_return_if_fail (PHOSH_IS_FAVORITES (favorites));

  phosh_home_set_state (self, PHOSH_HOME_STATE_FOLDED);
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

  g_signal_connect_swapped (self->favorites,
                            "activity-launched",
                            G_CALLBACK(fold_cb),
                            self);
  g_signal_connect_swapped (self->favorites,
                            "activity-raised",
                            G_CALLBACK(fold_cb),
                            self);
  g_signal_connect_swapped (self->favorites,
                            "selection-aborted",
                            G_CALLBACK(fold_cb),
                            self);

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

  PHOSH_TYPE_OSK_BUTTON;
  PHOSH_TYPE_FAVORITES;
  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/home.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, btn_home);
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, img_home);
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, btn_osk);
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, favorites);

  gtk_widget_class_set_css_name (widget_class, "phosh-home");
}


static void
phosh_home_init (PhoshHome *self)
{
  self->state = PHOSH_HOME_STATE_FOLDED;

  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_home_new (struct zwlr_layer_shell_v1 *layer_shell,
                struct wl_output *wl_output)
{
  return g_object_new (PHOSH_TYPE_HOME,
                       "layer-shell", layer_shell,
                       "wl-output", wl_output,
                       "height", PHOSH_HOME_BUTTON_HEIGHT,
                       "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_TOP,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", PHOSH_HOME_BUTTON_HEIGHT,
                       "namespace", "phosh home",
                       NULL);
}

/**
 * phosh_home_set_state:
 *
 * Set the state of the home screen. See #PhoshHomeState.
 */
void
phosh_home_set_state (PhoshHome *self, PhoshHomeState state)
{
  int width, height;
  GtkStyleContext *context;

  g_return_if_fail (PHOSH_IS_HOME (self));

  if (self->state == state)
    return;

  self->state = state;
  g_debug ("Setting state to %s", g_enum_to_string (PHOSH_TYPE_HOME_STATE, state));

  phosh_shell_get_usable_area (phosh_shell_get_default (), NULL, NULL, &width, &height);
  /* We don't change the exclusive zone since we don't want to push all clients upward */
  if (state == PHOSH_HOME_STATE_UNFOLDED)
    height = PHOSH_HOME_BUTTON_HEIGHT + height;
  else
    height = PHOSH_HOME_BUTTON_HEIGHT;

  phosh_layer_surface_set_size (PHOSH_LAYER_SURFACE (self), width, height);

  context = gtk_widget_get_style_context(self->img_home);
  gtk_widget_hide (self->img_home);
  if (state == PHOSH_HOME_STATE_UNFOLDED) {
    gtk_style_context_add_class(context, "phosh-home-btn-image-down");
    gtk_style_context_remove_class(context, "phosh-home-btn-image-up");
  } else {
    gtk_style_context_remove_class(context, "phosh-home-btn-image-down");
    gtk_style_context_add_class(context, "phosh-home-btn-image-up");
  }
  gtk_widget_show (self->img_home);

  if (state == PHOSH_HOME_STATE_UNFOLDED)
    gtk_widget_hide (self->btn_osk);
  else
    gtk_widget_show (self->btn_osk);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HOME_STATE]);
}
