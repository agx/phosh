/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Florian Loers <florianloers@mailbox.org>
 */

#define G_LOG_DOMAIN "phosh-portal-access-manager"

#include "app-auth-prompt.h"
#include "dbus/portal-dbus.h"
#include "portal-access-manager.h"
#include "portal-request.h"
#include "shell.h"
#include "util.h"

#define PORTAL_DBUS_NAME "sm.puri.Phosh.Portal"
#define PORTAL_OBJECT_PATH "/org/freedesktop/portal/desktop"
#define PORTAL_ACCESS_DIALOG_GRANTED      0
#define PORTAL_ACCESS_DIALOG_DENIED       1
#define PORTAL_ACCESS_DIALOG_ALREADY_OPEN 2


/**
 * PhoshPortalAccessManager:
 *
 * Implements org.freedesktop.impl.portal
 */

typedef struct _PhoshPortalAccessManager {
  PhoshDBusImplPortalAccessSkeleton access_portal;
  PhoshPortalRequest               *request;
  int                               dbus_name_id;
  PhoshAppAuthPrompt               *app_auth_prompt;
  GDBusMethodInvocation            *invocation;
  GVariant                         *choices;
} PhoshPortalAccessManager;

static void
phosh_portal_access_manager_access_iface_init (PhoshDBusImplPortalAccessIface *iface);

G_DEFINE_TYPE_WITH_CODE (PhoshPortalAccessManager,
                         phosh_portal_access_manager,
                         PHOSH_DBUS_TYPE_IMPL_PORTAL_ACCESS_SKELETON,
                         G_IMPLEMENT_INTERFACE (
                           PHOSH_DBUS_TYPE_IMPL_PORTAL_ACCESS,
                           phosh_portal_access_manager_access_iface_init));

static void
on_access_dialog_closed (PhoshPortalAccessManager *self)
{
  g_autoptr (GVariantDict) results = g_variant_dict_new (NULL);
  GVariant *choices = NULL;
  gboolean granted = phosh_app_auth_prompt_get_grant_access (GTK_WIDGET (self->app_auth_prompt));
  guint response = PORTAL_ACCESS_DIALOG_DENIED;

  g_clear_object (&self->request);

  if (granted)
    response = PORTAL_ACCESS_DIALOG_GRANTED;

  choices = phosh_app_auth_prompt_get_selected_choices (GTK_WIDGET (self->app_auth_prompt));
  g_variant_dict_insert_value (results, "choices", choices);
  phosh_dbus_impl_portal_access_complete_access_dialog (PHOSH_DBUS_IMPL_PORTAL_ACCESS (self),
                                                        self->invocation,
                                                        response,
                                                        g_variant_dict_end (results));
  self->invocation = NULL;

  if (self->app_auth_prompt != NULL)
    gtk_widget_hide (GTK_WIDGET (self->app_auth_prompt));
  g_clear_pointer (&self->app_auth_prompt, phosh_cp_widget_destroy);
}


static gboolean
handle_access_dialog (PhoshDBusImplPortalAccess *object,
                      GDBusMethodInvocation     *invocation,
                      const char                *arg_handle,
                      const char                *arg_app_id,
                      const char                *arg_parent_window,
                      const char                *arg_title,
                      const char                *arg_subtitle,
                      const char                *arg_body,
                      GVariant                  *arg_options)
{
  const char *sender;
  const char *grant_label = NULL;
  const char *deny_label = NULL;
  g_autoptr (GIcon) icon = NULL;
  GVariant *choices = NULL;
  GVariant *icon_variant = g_variant_lookup_value (arg_options, "icon", G_VARIANT_TYPE_STRING);
  PhoshPortalAccessManager *self = PHOSH_PORTAL_ACCESS_MANAGER (object);
  g_autoptr (PhoshPortalRequest) request = NULL;

  sender = g_dbus_method_invocation_get_sender (invocation);
  request = phosh_portal_request_new (sender, arg_app_id, arg_handle);

  if (self->app_auth_prompt != NULL) {
    return FALSE;
  }
  if (icon_variant != NULL) {
    const char *icon_name_str = g_variant_get_string (icon_variant, NULL);
    icon = g_themed_icon_new (icon_name_str);
  }
  g_variant_lookup (arg_options, "grant_label", "&s", grant_label);
  g_variant_lookup (arg_options, "deny_label",  "&s", deny_label);
  choices = g_variant_lookup_value (arg_options, "choices",
                                    G_VARIANT_TYPE (PHOSH_APP_AUTH_PROMPT_CHOICES_FORMAT));

  self->invocation = invocation;
  self->choices = choices;
  self->request = g_steal_pointer (&request);
  self->app_auth_prompt = PHOSH_APP_AUTH_PROMPT (
    phosh_app_auth_prompt_new (icon,
                               arg_title,
                               arg_subtitle,
                               arg_body,
                               grant_label,
                               deny_label,
                               FALSE,
                               choices));

  g_signal_connect_object (self->app_auth_prompt,
                           "closed",
                           G_CALLBACK (on_access_dialog_closed),
                           self,
                           G_CONNECT_SWAPPED);

  /* Show widget when not locked and keep that in sync */
  g_object_bind_property (phosh_shell_get_default (), "locked",
                          self->app_auth_prompt, "visible",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  phosh_portal_request_export (self->request, g_dbus_method_invocation_get_connection (invocation));
  return TRUE;
}

static void
phosh_portal_access_manager_access_iface_init (PhoshDBusImplPortalAccessIface *iface)
{
  iface->handle_access_dialog = handle_access_dialog;
}

static void
on_name_acquired (GDBusConnection *connection,
                  const char      *name,
                  gpointer         user_data)
{
  g_debug ("Acquired name %s", name);
}

static void
on_name_lost (GDBusConnection *connection,
              const char      *name,
              gpointer         user_data)
{
  g_debug ("Lost or failed to acquire name %s", name);
}

static void
on_bus_acquired (GDBusConnection *connection,
                 const char      *name,
                 gpointer         user_data)
{
  PhoshPortalAccessManager *self = user_data;
  g_autoptr (GError) err = NULL;

  if (g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                        connection,
                                        PORTAL_OBJECT_PATH,
                                        &err)) {
    g_debug ("Access portal exported");
  } else {
    g_warning ("Failed to export on %s: %s", PORTAL_DBUS_NAME, err->message);
  }
}

static void
phosh_portal_access_manager_constructed (GObject *object)
{
  PhoshPortalAccessManager *self = PHOSH_PORTAL_ACCESS_MANAGER (object);

  G_OBJECT_CLASS (phosh_portal_access_manager_parent_class)->constructed (object);

  self->dbus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                       PORTAL_DBUS_NAME,
                                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                                       G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                       on_bus_acquired,
                                       on_name_acquired,
                                       on_name_lost,
                                       self,
                                       NULL);
}

static void
phosh_portal_access_manager_dispose (GObject *object)
{
  PhoshPortalAccessManager *self = PHOSH_PORTAL_ACCESS_MANAGER (object);

  g_clear_pointer (&self->app_auth_prompt, phosh_cp_widget_destroy);
  g_clear_object (&self->request);

  if (g_dbus_interface_skeleton_get_object_path (G_DBUS_INTERFACE_SKELETON (self)))
    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (self));

  g_clear_handle_id (&self->dbus_name_id, g_bus_unown_name);

  G_OBJECT_CLASS (phosh_portal_access_manager_parent_class)->dispose (object);
}

static void
phosh_portal_access_manager_class_init (PhoshPortalAccessManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_portal_access_manager_constructed;
  object_class->dispose = phosh_portal_access_manager_dispose;
}

static void
phosh_portal_access_manager_init (PhoshPortalAccessManager *self)
{
}

PhoshPortalAccessManager *
phosh_portal_access_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_PORTAL_ACCESS_MANAGER, NULL);
}
