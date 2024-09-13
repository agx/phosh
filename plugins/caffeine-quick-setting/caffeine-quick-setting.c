/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "caffeine-quick-setting.h"
#include "plugin-shell.h"
#include "quick-setting.h"
#include "session-manager.h"
#include "status-icon.h"

#include <glib/gi18n.h>

/**
 * PhoshCaffeineQuickSetting:
 *
 * A quick setting to keep the session from going idle
 */

enum {
  PROP_0,
  PROP_INHIBITED,
  LAST_PROP,
};
static GParamSpec *props[LAST_PROP];

struct _PhoshCaffeineQuickSetting {
  PhoshQuickSetting        parent;

  PhoshStatusIcon         *info;
  guint                    cookie;
};

G_DEFINE_TYPE (PhoshCaffeineQuickSetting, phosh_caffeine_quick_setting, PHOSH_TYPE_QUICK_SETTING);


static void
phosh_caffeine_quick_setting_inhibit (PhoshCaffeineQuickSetting *self, gboolean inhibit)
{
  PhoshSessionManager *manager = phosh_shell_get_session_manager (phosh_shell_get_default ());

  if (inhibit  == !!self->cookie)
    return;

  if (inhibit) {
    self->cookie = phosh_session_manager_inhibit (manager,
                                                  PHOSH_SESSION_INHIBIT_IDLE |
                                                  PHOSH_SESSION_INHIBIT_SUSPEND,
    /* Translators: Phosh prevents the session from going idle because the caffeine quick setting is toggled */
                                                  _("Phosh on caffeine"));
  } else {
    phosh_session_manager_uninhibit (manager, self->cookie);
    self->cookie = 0;
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_INHIBITED]);
}


static void
phosh_caffeine_quick_setting_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  PhoshCaffeineQuickSetting *self = PHOSH_CAFFEINE_QUICK_SETTING (object);

  switch (property_id) {
    case PROP_INHIBITED:
      phosh_caffeine_quick_setting_inhibit (self, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_caffeine_quick_setting_get_property (GObject *object,
                                           guint property_id,
                                           GValue *value,
                                           GParamSpec *pspec)
{
  PhoshCaffeineQuickSetting *self = PHOSH_CAFFEINE_QUICK_SETTING (object);

  switch (property_id) {
    case PROP_INHIBITED:
      g_value_set_boolean (value, !!self->cookie);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
on_clicked (PhoshCaffeineQuickSetting *self)
{
  phosh_caffeine_quick_setting_inhibit (self, !self->cookie);
}


static gboolean
transform_to_icon_name (GBinding     *binding,
                        const GValue *from_value,
                        GValue       *to_value,
                        gpointer      user_data)
{
  gboolean inhibited = g_value_get_boolean (from_value);
  const char *icon_name;

  icon_name = inhibited ? "cafe-hot-symbolic" : "cafe-cold-symbolic";
  g_value_set_string (to_value, icon_name);
  return TRUE;
}


static gboolean
transform_to_label (GBinding     *binding,
                    const GValue *from_value,
                    GValue       *to_value,
                    gpointer      user_data)
{
  gboolean inhibited = g_value_get_boolean (from_value);
  const char *label;

  label = inhibited ? _("On") : _("Off");
  g_value_set_string (to_value, label);
  return TRUE;
}


static void
phosh_caffeine_quick_setting_finalize (GObject *gobject)
{
  PhoshCaffeineQuickSetting *self = PHOSH_CAFFEINE_QUICK_SETTING (gobject);

  if (self->cookie)
    phosh_caffeine_quick_setting_inhibit (self, FALSE);

  G_OBJECT_CLASS (phosh_caffeine_quick_setting_parent_class)->finalize (gobject);
}


static void
phosh_caffeine_quick_setting_class_init (PhoshCaffeineQuickSettingClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phosh_caffeine_quick_setting_finalize;
  object_class->set_property = phosh_caffeine_quick_setting_set_property;
  object_class->get_property = phosh_caffeine_quick_setting_get_property;

  props[PROP_INHIBITED] =
    g_param_spec_boolean ("inhibited", "", "",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY |G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/plugins/caffeine-quick-setting/qs.ui");

  gtk_widget_class_bind_template_child (widget_class, PhoshCaffeineQuickSetting, info);

  gtk_widget_class_bind_template_callback (widget_class, on_clicked);
}


static void
phosh_caffeine_quick_setting_init (PhoshCaffeineQuickSetting *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_icon_theme_add_resource_path (gtk_icon_theme_get_default (),
                                    "/mobi/phosh/plugins/caffeine-quick-setting/icons");

  g_object_bind_property (self, "inhibited",
                          self, "active",
                          G_BINDING_DEFAULT |
                          G_BINDING_SYNC_CREATE);

  g_object_bind_property_full (self, "inhibited",
                               self->info, "icon-name",
                               G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE,
                               transform_to_icon_name,
                               NULL, NULL, NULL);

  g_object_bind_property_full (self, "inhibited",
                               self->info, "info",
                               G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE,
                               transform_to_label,
                               NULL, NULL, NULL);
}
