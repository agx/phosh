/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-wwaninfo"

#include "config.h"

#include "wwaninfo.h"
#include "wwan/phosh-wwan-mm.h"

#define WWAN_INFO_DEFAULT_ICON_SIZE 24

/**
 * SECTION:phosh-wwan-info
 * @short_description: A widget to display the wwan status
 * @Title: PhoshWWanInfo
 */
enum {
  PROP_0,
  PROP_SIZE,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];


struct _PhoshWWanInfo
{
  GtkBox parent;

  PhoshWWanMM *wwan;

  GtkImage *icon;
  GtkLabel *access_tec;

  gint size;
};

G_DEFINE_TYPE (PhoshWWanInfo, phosh_wwan_info, GTK_TYPE_BOX)

static void
phosh_wwan_info_set_property (GObject *object,
                              guint property_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
  PhoshWWanInfo *self = PHOSH_WWAN_INFO (object);

  switch (property_id) {
  case PROP_SIZE:
    phosh_wwan_info_set_size (self, g_value_get_int(value));
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
  case PROP_SIZE:
    g_value_set_int (value, self->size);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static const char *
signal_quality_descriptive(guint quality)
{
  if (quality > 80)
    return "excellent";
  else if (quality > 55)
    return "good";
  else if (quality > 30)
    return "ok";
  else if (quality > 5)
    return "weak";
  else
    return "none";
}


static void
update_icon_data(PhoshWWanInfo *self, GParamSpec *psepc, PhoshWWanMM *wwan)
{
  guint quality;
  g_autoptr(GdkPixbuf) src = NULL, dest = NULL;
  g_autofree gchar *icon_name = NULL;
  const char *access_tec;
  gboolean visible;

  g_return_if_fail (PHOSH_IS_WWAN_INFO (self));
  visible = phosh_wwan_is_present (PHOSH_WWAN (self->wwan));
  g_debug ("Updating wwan icon, shown: %d", visible);
  gtk_widget_set_visible (GTK_WIDGET (self), visible);

  /* SIM missing */
  if (!phosh_wwan_has_sim (PHOSH_WWAN (self->wwan)))
    icon_name = g_strdup ("auth-sim-missing-symbolic");

  /* SIM unlock required */
  if (!phosh_wwan_is_unlocked (PHOSH_WWAN (self->wwan)))
    icon_name = g_strdup ("auth-sim-locked-symbolic");

  if (icon_name) {
    gtk_image_set_from_icon_name (GTK_IMAGE (self->icon), icon_name, -1);
    gtk_widget_show (GTK_WIDGET (self->icon));
    gtk_widget_hide (GTK_WIDGET (self->access_tec));
    return;
  }

  /* Signal quality */
  quality = phosh_wwan_get_signal_quality (PHOSH_WWAN (self->wwan));
  icon_name = g_strdup_printf ("network-cellular-signal-%s-symbolic",
                               signal_quality_descriptive (quality));
  gtk_image_set_from_icon_name (GTK_IMAGE (self->icon), icon_name, -1);
  gtk_widget_show (GTK_WIDGET (self->icon));

  /* Access technology */
  access_tec = phosh_wwan_get_access_tec (PHOSH_WWAN (self->wwan));
  if (access_tec == NULL) {
    gtk_widget_hide (GTK_WIDGET(self->access_tec));
    return;
  }

  gtk_label_set_text (self->access_tec, access_tec);
  gtk_widget_show (GTK_WIDGET(self->access_tec));
}


static gboolean
on_idle (PhoshWWanInfo *self)
{
  update_icon_data (self, NULL, NULL);
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

  if (self->size == -1)
    phosh_wwan_info_set_size (self, WWAN_INFO_DEFAULT_ICON_SIZE);

  for (int i = 0; i < g_strv_length(signals); i++) {
    g_signal_connect_swapped (self->wwan, signals[i],
                              G_CALLBACK (update_icon_data),
                              self);
  }
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
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = phosh_wwan_info_set_property;
  object_class->get_property = phosh_wwan_info_get_property;

  object_class->constructed = phosh_wwan_info_constructed;
  object_class->dispose = phosh_wwan_info_dispose;

  props[PROP_SIZE] =
    g_param_spec_int (
      "size",
      "Size",
      "The icon's size",
      -1,
      G_MAXINT,
      -1,
      G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/wwaninfo.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshWWanInfo, icon);
  gtk_widget_class_bind_template_child (widget_class, PhoshWWanInfo, access_tec);
}


static void
phosh_wwan_info_init (PhoshWWanInfo *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_wwan_info_new (void)
{
  return g_object_new (PHOSH_TYPE_WWAN_INFO, NULL);
}

void
phosh_wwan_info_set_size (PhoshWWanInfo *self, gint size)
{
  g_return_if_fail (PHOSH_IS_WWAN_INFO (self));

  if (self->size == size)
    return;

  self->size = size;
  gtk_image_set_pixel_size (GTK_IMAGE (self->icon), size);
  /* fixme: set text size */
  //gtk_label_set_ (GTK_IMAGE (self->icon), size);
  gtk_widget_queue_resize (GTK_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SIZE]);
}


gint
phosh_wwan_info_get_size (PhoshWWanInfo *self)
{
  g_return_val_if_fail (PHOSH_IS_WWAN_INFO (self), 0);

  return self->size;
}
