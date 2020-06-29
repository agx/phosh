/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-wwaninfo"

#include "config.h"

#include "wwaninfo.h"
#include "wwan/phosh-wwan-mm.h"

#include <glib/gi18n.h>

#define WWAN_INFO_DEFAULT_ICON_SIZE 24

/**
 * SECTION:wwaninfo
 * @short_description: A widget to display the wwan status
 * @Title: PhoshWWanInfo
 *
 * A good indicator whether to show the icon is the
 * #PhoshWwanInfo:present property that indicates if
 * hardware is present.
 */
enum {
  PROP_0,
  PROP_SHOW_DETAIL,
  PROP_PRESENT,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];


struct _PhoshWWanInfo
{
  GtkBox parent;

  PhoshWWanMM *wwan;
  gboolean present;
  gboolean show_detail;
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
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static const char *
signal_quality_icon_name (guint quality)
{
  if (quality > 80)
    return "network-cellular-signal-excellent-symbolic";
  else if (quality > 55)
    return "network-cellular-signal-good-symbolic";
  else if (quality > 30)
    return "network-cellular-signal-ok-symbolic";
  else if (quality > 5)
    return "network-cellular-signal-weak-symbolic";
  else
    return "network-cellular-signal-none-symbolic";
}


static void
update_icon_data(PhoshWWanInfo *self, GParamSpec *psepc, PhoshWWanMM *wwan)
{
  GtkWidget *access_tec_widget;
  guint quality;
  const gchar *icon_name = NULL;
  const char *access_tec;
  gboolean present;

  g_return_if_fail (PHOSH_IS_WWAN_INFO (self));
  present = phosh_wwan_is_present (PHOSH_WWAN (self->wwan));
  g_debug ("Updating wwan present: %d", present);
  if (present != self->present) {
    self->present = present;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRESENT]);
  }

  access_tec_widget = phosh_status_icon_get_extra_widget (PHOSH_STATUS_ICON (self));

  if (!present) {
    icon_name = ("network-cellular-disabled-symbolic");
  } else if (!phosh_wwan_has_sim (PHOSH_WWAN (self->wwan))) /* SIM missing */
    icon_name = "auth-sim-missing-symbolic";
  else { /* SIM unlock required */
    if (!phosh_wwan_is_unlocked (PHOSH_WWAN (self->wwan)))
      icon_name = "auth-sim-locked-symbolic";
  }

  if (icon_name) {
    phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), icon_name);
    gtk_widget_hide (access_tec_widget);
    return;
  }

  /* Signal quality */
  quality = phosh_wwan_get_signal_quality (PHOSH_WWAN (self->wwan));
  icon_name = signal_quality_icon_name (quality);
  phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), icon_name);

  if (!self->show_detail) {
    gtk_widget_hide (access_tec_widget);
    return;
  }

  /* Access technology */
  access_tec = phosh_wwan_get_access_tec (PHOSH_WWAN (self->wwan));
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
  const gchar *info;
  g_return_if_fail (PHOSH_IS_WWAN_INFO (self));

  info = phosh_wwan_get_operator (PHOSH_WWAN (self->wwan));
  if (!info || !g_strcmp0(info, ""))
    info = _("Cellular");

  phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), info);
}

static gboolean
on_idle (PhoshWWanInfo *self)
{
  update_icon_data (self, NULL, NULL);
  update_info (self);
  return FALSE;
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
                              NULL,
  };

  G_OBJECT_CLASS (phosh_wwan_info_parent_class)->constructed (object);

  self->wwan = phosh_wwan_mm_new();

  for (int i = 0; i < g_strv_length(signals); i++) {
    g_signal_connect_swapped (self->wwan, signals[i],
                              G_CALLBACK (update_icon_data),
                              self);
  }

  g_signal_connect_swapped (self->wwan,
                            "notify::operator",
                            G_CALLBACK (update_info),
                            self);

  g_idle_add ((GSourceFunc) on_idle, self);
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

  object_class->set_property = phosh_wwan_info_set_property;
  object_class->get_property = phosh_wwan_info_get_property;

  object_class->constructed = phosh_wwan_info_constructed;
  object_class->dispose = phosh_wwan_info_dispose;

  props[PROP_SHOW_DETAIL] =
    g_param_spec_boolean (
      "show-detail",
      "Show detail",
      "Show wwan details",
      FALSE,
      G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_PRESENT] =
    g_param_spec_boolean (
      "present",
      "Present",
      "Whether WWAN hardware is present",
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
