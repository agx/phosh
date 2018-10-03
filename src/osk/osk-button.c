/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-osk-button"

#include "osk-button.h"
#include "phosh-osk0-dbus.h"

#include <gio/gio.h>
#include <gtk/gtk.h>

#define VIRTBOARD_DBUS_NAME      "sm.puri.OSK0"
#define VIRTBOARD_DBUS_OBJECT    "/sm/puri/OSK0"

/**
 * SECTION:phosh-osk-button
 * @short_description: A button that toggles the OSK
 * @Title: PhoshOsk
 *
 * The #PhoshOskButton is responsible for toggling the on screen keyboard
 */
typedef struct
{
  /* Currently the only impl. We can use an interface once we support
   * different OSK types */
  PhoshOsk0SmPuriOSK0 *proxy;
  gboolean visible;
  gboolean setting_visibility;
} PhoshOskButtonPrivate;

typedef struct _PhoshOskButton
{
  GtkToggleButton parent;
} PhoshOskButton;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshOskButton, phosh_osk_button, GTK_TYPE_TOGGLE_BUTTON)


static void
on_osk_show (GObject             *source_object,
             GAsyncResult        *res,
             gpointer             user_data)
{
  GVariant *variant;
  GError *err = NULL;

  variant = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &err);
  if (!variant) {
    g_warning ("Unable to toggle OSK: %s", err->message);
    g_clear_error (&err);
    return;
  }
  g_variant_unref (variant);
}


static void
phosh_osk_show (PhoshOskButton *self, gboolean show)
{
  PhoshOskButtonPrivate *priv = phosh_osk_button_get_instance_private (self);

  if (priv->proxy) {
    phosh_osk0_sm_puri_osk0_call_set_visible (
      priv->proxy,
      show,
      NULL,
      (GAsyncReadyCallback) on_osk_show,
      NULL);
  }
}


static void
toggled_cb (PhoshOskButton *self, gpointer data)
{
  PhoshOskButtonPrivate *priv = phosh_osk_button_get_instance_private (self);

  priv->setting_visibility = TRUE;
  priv->visible = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self));
  phosh_osk_show (self, priv->visible);
  priv->setting_visibility = FALSE;
}


static void
visibility_changed (PhoshOskButton *self, gboolean visible)
{
  PhoshOskButtonPrivate *priv = phosh_osk_button_get_instance_private (self);

  if (!priv->setting_visibility)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self), visible);
}


static void
dbus_props_changed_cb (PhoshOsk0SmPuriOSK0 *proxy,
                       GVariant *changed_properties,
                       GStrv invaliated,
                       gpointer *data)
{
  PhoshOskButton *self = PHOSH_OSK_BUTTON (data);
  char *property;
  GVariantIter i;
  GVariant *value;

  g_variant_iter_init (&i, changed_properties);
  while (g_variant_iter_next (&i, "{&sv}", &property, &value)) {
    g_debug ("OSK property '%s' changed", property);
    if (strcmp (property, "Visible") == 0) {
      visibility_changed (self, g_variant_get_boolean (value));
    }
    g_variant_unref (value);
  }
}


static void
dbus_name_owner_changed_cb (PhoshOskButton *self, gpointer data)
{
  PhoshOskButtonPrivate *priv;
  g_autofree char *name_owner = NULL;

  g_return_if_fail (PHOSH_IS_OSK_BUTTON (self));
  priv = phosh_osk_button_get_instance_private (self);

  name_owner = g_dbus_proxy_get_name_owner (G_DBUS_PROXY (priv->proxy));
  g_debug ("OSK bus '%s' owned by %s", VIRTBOARD_DBUS_NAME, name_owner ? name_owner : "nobody");

  if (name_owner) {
    gtk_widget_set_sensitive (GTK_WIDGET (self), TRUE);
  } else {
    gtk_widget_set_sensitive (GTK_WIDGET (self), FALSE);
    /* When there's no OSK we always want the button to be unpressed */
    visibility_changed (self, FALSE);
  };
}


static void
phosh_osk_button_constructed (GObject *object)
{
  PhoshOskButton *self = PHOSH_OSK_BUTTON (object);
  PhoshOskButtonPrivate *priv = phosh_osk_button_get_instance_private (self);
  GError *err = NULL;
  GtkWidget *image;

  G_OBJECT_CLASS (phosh_osk_button_parent_class)->constructed (object);

  priv->proxy = phosh_osk0_sm_puri_osk0_proxy_new_for_bus_sync(
    G_BUS_TYPE_SESSION,
    G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION,
    VIRTBOARD_DBUS_NAME,
    VIRTBOARD_DBUS_OBJECT,
    NULL,
    &err);

  if (priv->proxy == NULL) {
    g_warning ("Failed to register with osk: %s", err->message);
    g_clear_error (&err);
    g_return_if_fail (priv->proxy);
  }

  g_signal_connect (
    priv->proxy,
    "g-properties-changed",
    G_CALLBACK (dbus_props_changed_cb),
    self);

  g_signal_connect_swapped (
    priv->proxy,
    "notify::g-name-owner",
    G_CALLBACK (dbus_name_owner_changed_cb),
    self);
  dbus_name_owner_changed_cb (self, NULL);

  g_signal_connect (self,
                    "toggled",
                    G_CALLBACK (toggled_cb),
                    NULL);

  image = gtk_image_new_from_icon_name ("input-keyboard-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (self), image);
  gtk_button_set_always_show_image (GTK_BUTTON (self), TRUE);

  visibility_changed (self, phosh_osk0_sm_puri_osk0_get_visible (priv->proxy));
}


static void
phosh_osk_button_dispose (GObject *object)
{
  PhoshOskButtonPrivate *priv = phosh_osk_button_get_instance_private (PHOSH_OSK_BUTTON(object));

  g_clear_object (&priv->proxy);
  G_OBJECT_CLASS (phosh_osk_button_parent_class)->dispose (object);
}


static void
phosh_osk_button_class_init (PhoshOskButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_osk_button_constructed;
  object_class->dispose = phosh_osk_button_dispose;
}


static void
phosh_osk_button_init (PhoshOskButton *self)
{
}


GtkWidget *
phosh_osk_button_new (void)
{
  return g_object_new (PHOSH_TYPE_OSK_BUTTON, NULL);
}
