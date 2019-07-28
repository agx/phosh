/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-home"

#include "config.h"
#include "home.h"
#include "osk/osk-button.h"

/**
 * SECTION:phosh-home
 * @short_description: The ome button at the bottom of the screen
 * @Title: PhoshHome
 *
 * The #PhoshHome is displayed at the bottom of the screen. It features
 * the home button and the button to toggle the OSK.
 */
enum {
  HOME_ACTIVATED,
  OSK_ACTIVATED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct _PhoshHome
{
  PhoshLayerSurface parent;

  GtkWidget *btn_home;
  GtkWidget *btn_osk;
};
G_DEFINE_TYPE(PhoshHome, phosh_home, PHOSH_TYPE_LAYER_SURFACE);


static void
home_clicked_cb (PhoshHome *self, GtkButton *btn)
{
  g_return_if_fail (PHOSH_IS_HOME (self));
  g_return_if_fail (GTK_IS_BUTTON (btn));
  g_signal_emit(self, signals[HOME_ACTIVATED], 0);
}


static void
osk_clicked_cb (PhoshHome *self, GtkButton *btn)
{
  g_return_if_fail (PHOSH_IS_HOME (self));
  g_return_if_fail (GTK_IS_BUTTON (btn));
  g_signal_emit(self, signals[OSK_ACTIVATED], 0);
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

  G_OBJECT_CLASS (phosh_home_parent_class)->constructed (object);
}


static void
phosh_home_class_init (PhoshHomeClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_home_constructed;

  signals[HOME_ACTIVATED] = g_signal_new ("home-activated",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
  signals[OSK_ACTIVATED] = g_signal_new ("osk-activated",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  PHOSH_TYPE_OSK_BUTTON;
  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/home.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, btn_home);
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, btn_osk);

  gtk_widget_class_set_css_name (widget_class, "phosh-home");
}


static void
phosh_home_init (PhoshHome *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_home_new (struct zwlr_layer_shell_v1 *layer_shell,
                struct wl_output *wl_output)
{
  return g_object_new (PHOSH_TYPE_HOME,
                       "layer-shell", layer_shell,
                       "wl-output", wl_output,
                       "height", PHOSH_HOME_HEIGHT,
                       "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_TOP,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", PHOSH_HOME_HEIGHT,
                       "namespace", "phosh home",
                       NULL);
}
