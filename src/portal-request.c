/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Florian Loers <florianloers@mailbox.org>
 */

#include "portal-request.h"
#include "dbus/portal-dbus.h"

/**
 * PhoshPortalRequest:
 *
 * Shared request api for all portal backend interfaces.
 *
 * The PhoshPortalRequest implements the Request backend interface.
 * See the portal documentation: https://flatpak.github.io/xdg-desktop-portal/#gdbus-org.freedesktop.impl.portal.Request.
 *
 * The request object will alive as long as it is exported. Users of the request must ensure to export/unexport properly.
 */

static void
phosh_portal_request_iface_init (PhoshDBusImplPortalRequestIface *iface);

typedef struct _PhoshPortalRequest {
  PhoshDBusImplPortalRequestSkeleton parent_instance;

  gboolean                           exported;
  char                              *id;
  char                              *sender;
  char                              *app_id;

} PhoshPortalRequest;

G_DEFINE_TYPE_WITH_CODE (PhoshPortalRequest,
                         phosh_portal_request,
                         PHOSH_DBUS_TYPE_IMPL_PORTAL_REQUEST_SKELETON,
                         G_IMPLEMENT_INTERFACE (
                           PHOSH_DBUS_TYPE_IMPL_PORTAL_REQUEST,
                           phosh_portal_request_iface_init));

static gboolean
handle_close (PhoshDBusImplPortalRequest *object, GDBusMethodInvocation *invocation)
{
  PhoshPortalRequest *self = (PhoshPortalRequest *)object;

  if (self->exported)
    phosh_portal_request_unexport (self);

  phosh_dbus_impl_portal_request_complete_close (object, invocation);

  return TRUE;
}

static void
phosh_portal_request_iface_init (PhoshDBusImplPortalRequestIface *iface)
{
  iface->handle_close = handle_close;
}

static void
phosh_portal_request_finalize (GObject *object)
{
  PhoshPortalRequest *self = PHOSH_PORTAL_REQUEST (object);

  phosh_portal_request_unexport (self);
  g_free (self->sender);
  g_free (self->app_id);
  g_free (self->id);

  G_OBJECT_CLASS (phosh_portal_request_parent_class)->finalize (object);
}

static void
phosh_portal_request_class_init (PhoshPortalRequestClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = phosh_portal_request_finalize;
}

static void
phosh_portal_request_init (PhoshPortalRequest *request)
{
}

PhoshPortalRequest *
phosh_portal_request_new (const char *sender, const char *app_id, const char *id)
{
  PhoshPortalRequest *request;

  request = g_object_new (PHOSH_TYPE_PORTAL_REQUEST, NULL);
  request->sender = g_strdup (sender);
  request->app_id = g_strdup (app_id);
  request->id = g_strdup (id);

  return request;
}

gboolean
phosh_portal_request_exported (PhoshPortalRequest *self)
{
  g_return_val_if_fail (PHOSH_IS_PORTAL_REQUEST (self), FALSE);

  return self->exported;
}

void
phosh_portal_request_export (PhoshPortalRequest *self, GDBusConnection *connection)
{
  g_autoptr (GError) error = NULL;

  g_return_if_fail (PHOSH_IS_PORTAL_REQUEST (self));
  g_return_if_fail (G_IS_DBUS_CONNECTION (connection));

  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                         connection, self->id,
                                         &error)) {
    g_warning ("Error exporting request: %s", error->message);
    self->exported = TRUE;
  }
}

void
phosh_portal_request_unexport (PhoshPortalRequest *self)
{
  g_return_if_fail (PHOSH_IS_PORTAL_REQUEST (self));

  if (self->exported) {
    self->exported = FALSE;
    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (self));
  }
}
