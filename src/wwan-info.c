/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-wwan-info"

#include <glib/gi18n.h>

#include "phosh-config.h"
#include "wwan-info.h"
#include "wwan/wwan-manager.h"

#include "shell.h"

/**
 * PhoshWWanInfo:
 *
 * A widget to display the wwan status
 *
 * A good indicator whether to show the icon is the
 * #PhoshWWanInfo:present property that indicates if
 * hardware is present.
 */
enum {
  PROP_0,
  PROP_SHOW_DETAIL,
  PROP_PRESENT,
  PROP_ENABLED,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];


struct _PhoshWWanInfo
{
  PhoshStatusIcon  parent;

  PhoshWWan       *wwan;
  gboolean         present;
  gboolean         enabled;
  gboolean         show_detail;
};

G_DEFINE_TYPE (PhoshWWanInfo, phosh_wwan_info, PHOSH_TYPE_STATUS_ICON)

static void
phosh_wwan_info_set_property (GObject *object,
                              guint property_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
  PhoshWWanInfo *self = PHOSH_WWAN_INFO (object);

  switch (property_id) {
  case PROP_SHOW_DETAIL:
    phosh_wwan_info_set_show_detail (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_wwan_info_get_property (GObject *object,
                              guint property_id,
                              GValue *value,
                              GParamSpec *pspec)
{
  PhoshWWanInfo *self = PHOSH_WWAN_INFO (object);

  switch (property_id) {
  case PROP_SHOW_DETAIL:
    g_value_set_boolean (value, self->show_detail);
    break;
  case PROP_PRESENT:
    g_value_set_boolean (value, self->present);
    break;
  case PROP_ENABLED:
    g_value_set_boolean (value, self->enabled);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


enum quality {
  QUALITY_EXCELLENT = 0,
  QUALITY_GOOD,
  QUALITY_OK,
  QUALITY_WEAK,
  QUALITY_NONE,
};

static const char *quality_data[] = {
  "network-cellular-signal-excellent-symbolic",
  "network-cellular-signal-good-symbolic",
  "network-cellular-signal-ok-symbolic",
  "network-cellular-signal-weak-symbolic",
  "network-cellular-signal-none-symbolic",
  NULL,
};

static const char *quality_no_data[] = {
  "network-cellular-no-data-signal-excellent-symbolic",
  "network-cellular-no-data-signal-good-symbolic",
  "network-cellular-no-data-signal-ok-symbolic",
  "network-cellular-no-data-signal-weak-symbolic",
  "network-cellular-no-data-signal-none-symbolic",
  NULL,
};

static const char *
signal_quality_icon_name (guint quality, gboolean data_enabled)
{
  const char **q = data_enabled ? quality_data : quality_no_data;

  if (quality > 80)
    return q[QUALITY_EXCELLENT];
  else if (quality > 55)
    return q[QUALITY_GOOD];
  else if (quality > 30)
    return q[QUALITY_OK];
  else if (quality > 5)
    return q[QUALITY_WEAK];
  else
    return q[QUALITY_NONE];
}


static void
update_icon_data(PhoshWWanInfo *self, GParamSpec *psepc, PhoshWWan *wwan)
{
  GtkWidget *access_tec_widget;
  guint quality;
  const char *icon_name = NULL;
  const char *access_tec;
  gboolean present, enabled, data_enabled;

  g_return_if_fail (PHOSH_IS_WWAN_INFO (self));
  present = phosh_wwan_is_present (self->wwan);
  g_debug ("Updating wwan present: %d", present);
  if (present != self->present) {
    self->present = present;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRESENT]);
  }

  access_tec_widget = phosh_status_icon_get_extra_widget (PHOSH_STATUS_ICON (self));

  enabled = phosh_wwan_is_enabled (self->wwan);
  if (!present) {
    icon_name = "network-cellular-disabled-symbolic";
  } else if (!phosh_wwan_has_sim (self->wwan)) {
    icon_name = "auth-sim-missing-symbolic";
  } else if (!phosh_wwan_is_unlocked (self->wwan)) {
      icon_name = "auth-sim-locked-symbolic";
  } else if (!enabled) {
    icon_name = "network-cellular-disabled-symbolic";
  }

  if (self->enabled != enabled) {
    self->enabled = enabled;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);
  }

  if (icon_name) {
    phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), icon_name);
    gtk_widget_hide (access_tec_widget);
    return;
  }

  /* Signal quality */
  quality = phosh_wwan_get_signal_quality (self->wwan);
  data_enabled = phosh_wwan_manager_get_data_enabled (PHOSH_WWAN_MANAGER (self->wwan));
  icon_name = signal_quality_icon_name (quality, data_enabled);
  phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), icon_name);

  if (!self->show_detail) {
    gtk_widget_hide (access_tec_widget);
    return;
  }

  /* Access technology */
  access_tec = phosh_wwan_get_access_tec (self->wwan);
  if (access_tec == NULL) {
    gtk_widget_hide (access_tec_widget);
    return;
  }

  gtk_label_set_text (GTK_LABEL (access_tec_widget), access_tec);
  gtk_widget_show (access_tec_widget);
}

static void
update_info (PhoshWWanInfo *self)
{
  const char *info;
  g_return_if_fail (PHOSH_IS_WWAN_INFO (self));

  info = phosh_wwan_get_operator (self->wwan);
  if (!info || !g_strcmp0(info, "")) {
    /* Translators: Refers to the cellular wireless network */
    info = _("Cellular");
  }

  phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), info);
}


static void
phosh_wwan_info_idle_init (PhoshStatusIcon *icon)
{
  PhoshWWanInfo *self = PHOSH_WWAN_INFO (icon);

  update_icon_data (self, NULL, NULL);
  update_info (self);
}


static void
phosh_wwan_info_constructed (GObject *object)
{
  PhoshWWanInfo *self = PHOSH_WWAN_INFO (object);
  GStrv signals = (char *[]) {"notify::signal-quality",
                              "notify::access-tec",
                              "notify::unlocked",
                              "notify::sim",
                              "notify::present",
                              "notify::enabled",
                              "notify::data-enabled",
                              NULL,
  };

  G_OBJECT_CLASS (phosh_wwan_info_parent_class)->constructed (object);

  self->wwan = g_object_ref (phosh_shell_get_wwan (phosh_shell_get_default ()));

  for (int i = 0; i < g_strv_length(signals); i++) {
    g_signal_connect_swapped (self->wwan, signals[i],
                              G_CALLBACK (update_icon_data),
                              self);
  }

  g_signal_connect_swapped (self->wwan,
                            "notify::operator",
                            G_CALLBACK (update_info),
                            self);
}


static void
phosh_wwan_info_dispose (GObject *object)
{
  PhoshWWanInfo *self = PHOSH_WWAN_INFO(object);

  if (self->wwan) {
    g_signal_handlers_disconnect_by_data (self->wwan, self);
    g_clear_object (&self->wwan);
  }

  G_OBJECT_CLASS (phosh_wwan_info_parent_class)->dispose (object);
}


static void
phosh_wwan_info_class_init (PhoshWWanInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhoshStatusIconClass *status_icon_class = PHOSH_STATUS_ICON_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = phosh_wwan_info_set_property;
  object_class->get_property = phosh_wwan_info_get_property;

  object_class->constructed = phosh_wwan_info_constructed;
  object_class->dispose = phosh_wwan_info_dispose;

  gtk_widget_class_set_css_name (widget_class, "phosh-wwan-info");

  status_icon_class->idle_init = phosh_wwan_info_idle_init;

  /**
   * PhoshWWanInfo:show-details:
   *
   * Whether to show detailed information
   */
  props[PROP_SHOW_DETAIL] =
    g_param_spec_boolean ("show-detail", "", "",
                          FALSE,
                          G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  /**
   * PhoshWWanInfo:present:
   *
   * Whether WWAN hardware is present
   */
  props[PROP_PRESENT] =
    g_param_spec_boolean ("present", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * PhoshWWanInfo:enabled:
   *
   * Whether WWAN hardware is enabled
   */
  props[PROP_ENABLED] =
    g_param_spec_boolean ("enabled", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_wwan_info_init (PhoshWWanInfo *self)
{
  GtkWidget *access_tec = gtk_label_new (NULL);
  phosh_status_icon_set_extra_widget (PHOSH_STATUS_ICON (self), access_tec);
}


GtkWidget *
phosh_wwan_info_new (void)
{
  return g_object_new (PHOSH_TYPE_WWAN_INFO, NULL);
}


void
phosh_wwan_info_set_show_detail (PhoshWWanInfo *self, gboolean show)
{
  GtkWidget *access_tec;

  g_return_if_fail (PHOSH_IS_WWAN_INFO (self));

  if (self->show_detail == show)
    return;

  self->show_detail = !!show;

  access_tec = phosh_status_icon_get_extra_widget (PHOSH_STATUS_ICON (self));
  gtk_widget_set_visible (access_tec, show);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SHOW_DETAIL]);
}


gboolean
phosh_wwan_info_get_show_detail (PhoshWWanInfo *self)
{
  g_return_val_if_fail (PHOSH_IS_WWAN_INFO (self), 0);

  return self->show_detail;
}
