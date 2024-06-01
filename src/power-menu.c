/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "phosh-power-menu"

#include "phosh-config.h"

#include "fading-label.h"
#include "power-menu.h"
#include "shell.h"
#include "session-manager.h"

/**
 * PhoshPowerMenu:
 *
 * Menu on power button press
 */

enum {
  PROP_0,
  PROP_SHOW_SUSPEND,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


enum {
  DONE,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = {0};


struct _PhoshPowerMenu {
  PhoshSystemModalDialog parent;

  GtkStack *stack;
  gboolean  show_suspend;
};
G_DEFINE_TYPE (PhoshPowerMenu, phosh_power_menu, PHOSH_TYPE_SYSTEM_MODAL_DIALOG)


static void
on_power_menu_done (PhoshPowerMenu *self)
{
  g_return_if_fail (PHOSH_IS_POWER_MENU (self));
  g_signal_emit (self, signals[DONE], 0);
}



static void
phosh_power_menu_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  PhoshPowerMenu *self = PHOSH_POWER_MENU (object);

  switch (property_id) {
  case PROP_SHOW_SUSPEND:
    phosh_power_menu_set_show_suspend (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_power_menu_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PhoshPowerMenu *self = PHOSH_POWER_MENU (object);

  switch (property_id) {
  case PROP_SHOW_SUSPEND:
    g_value_set_boolean (value, phosh_power_menu_get_show_suspend (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_power_menu_class_init (PhoshPowerMenuClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_power_menu_get_property;
  object_class->set_property = phosh_power_menu_set_property;

  /**
   * PowerMenu:show-suspend:
   *
   * Whether the menu should show the option to suspend the device. Otherwise
   * the "power off" option is shown.
   */
  props[PROP_SHOW_SUSPEND] =
    g_param_spec_boolean ("show-suspend", "", "",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  signals[DONE] = g_signal_new ("done",
                                G_TYPE_FROM_CLASS (klass),
                                G_SIGNAL_RUN_LAST,
                                0, NULL, NULL, NULL,
                                G_TYPE_NONE, 0);

  g_type_ensure (PHOSH_TYPE_FADING_LABEL);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/power-menu.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshPowerMenu, stack);
  gtk_widget_class_bind_template_callback (widget_class, on_power_menu_done);

  gtk_widget_class_set_css_name (widget_class, "phosh-power-menu");
}


static void
phosh_power_menu_init (PhoshPowerMenu *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


PhoshPowerMenu *
phosh_power_menu_new (PhoshMonitor *monitor)
{
  return PHOSH_POWER_MENU (g_object_new (PHOSH_TYPE_POWER_MENU,
                                         "monitor", monitor,
                                         NULL));
}


void
phosh_power_menu_set_show_suspend (PhoshPowerMenu *self, gboolean show_suspend)
{
  const char *page;

  g_assert (PHOSH_IS_POWER_MENU (self));

  if (self->show_suspend == show_suspend)
    return;

  self->show_suspend = show_suspend;
  page = show_suspend ? "suspend" : "poweroff";
  gtk_stack_set_visible_child_name (self->stack, page);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SHOW_SUSPEND]);
}


gboolean
phosh_power_menu_get_show_suspend (PhoshPowerMenu *self)
{
  g_assert (PHOSH_IS_POWER_MENU (self));

  return self->show_suspend;
}
