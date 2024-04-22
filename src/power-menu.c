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
  DONE,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = {0};


struct _PhoshPowerMenu {
  PhoshSystemModalDialog parent;

  gboolean               unused;
};
G_DEFINE_TYPE (PhoshPowerMenu, phosh_power_menu, PHOSH_TYPE_SYSTEM_MODAL_DIALOG)


static void
on_power_menu_done (PhoshPowerMenu *self)
{
  g_return_if_fail (PHOSH_IS_POWER_MENU (self));
  g_signal_emit (self, signals[DONE], 0);
}


static void
phosh_power_menu_class_init (PhoshPowerMenuClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  signals[DONE] = g_signal_new ("done",
                                G_TYPE_FROM_CLASS (klass),
                                G_SIGNAL_RUN_LAST,
                                0, NULL, NULL, NULL,
                                G_TYPE_NONE, 0);

  g_type_ensure (PHOSH_TYPE_FADING_LABEL);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/power-menu.ui");
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
