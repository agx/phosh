/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Arun Mani J <arunmani@peartree.to>
 */

#include "simple-custom-quick-setting.h"
#include "quick-setting.h"
#include "status-icon.h"

#include <glib/gi18n.h>

/**
 * PhoshSimpleCustomQuickSetting:
 *
 * A simple custom quick setting for demonstration purposes.
 */
struct _PhoshSimpleCustomQuickSetting {
  PhoshQuickSetting        parent;

  PhoshStatusIcon         *info;
};

G_DEFINE_TYPE (PhoshSimpleCustomQuickSetting, phosh_simple_custom_quick_setting, PHOSH_TYPE_QUICK_SETTING);

static void
on_clicked (PhoshSimpleCustomQuickSetting *self)
{
  gboolean active = phosh_quick_setting_get_active (PHOSH_QUICK_SETTING (self));

  if (active) {
    phosh_status_icon_set_icon_name (self->info, "face-shutmouth-symbolic");
    phosh_status_icon_set_info (self->info, _("I'm Inactive"));
  } else {
    phosh_status_icon_set_icon_name (self->info, "face-smile-big-symbolic");
    phosh_status_icon_set_info (self->info, _("I'm Active"));
  }

  phosh_quick_setting_set_active (PHOSH_QUICK_SETTING (self), !active);
}

static void
phosh_simple_custom_quick_setting_class_init (PhoshSimpleCustomQuickSettingClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/plugins/simple-custom-quick-setting/qs.ui");

  gtk_widget_class_bind_template_child (widget_class, PhoshSimpleCustomQuickSetting, info);

  gtk_widget_class_bind_template_callback (widget_class, on_clicked);
}

static void
phosh_simple_custom_quick_setting_init (PhoshSimpleCustomQuickSetting *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  on_clicked (self);
}
