/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-session-manager"

#include "phosh-config.h"
#include "end-session-dialog.h"
#include "session-manager.h"
#include "shell.h"
#include "util.h"

#include "dbus/gnome-session-dbus.h"
#include "dbus/gnome-session-client-private-dbus.h"

#define BUS_NAME "org.gnome.SessionManager"
#define OBJECT_PATH "/org/gnome/SessionManager"

#define END_SESSION_DIALOG_OBJECT_PATH "/org/gnome/SessionManager/EndSessionDialog"

#define SESSION_SHUTDOWN_TIMEOUT 15

/**
 * PhoshSessionManager:
 *
 * Session interaction
 *
 * The #PhoshSessionManager is responsible for
 * managing attributes of the session.
 */

enum {
  PHOSH_SESSION_MANAGER_PROP_0,
  PHOSH_SESSION_MANAGER_PROP_ACTIVE,
  PHOSH_SESSION_MANAGER_PROP_LAST_PROP,
};
static GParamSpec *props[PHOSH_SESSION_MANAGER_PROP_LAST_PROP];

static void phosh_session_manager_end_session_dialog_iface_init (
  PhoshDBusEndSessionDialogIface *iface);

typedef struct _PhoshSessionManager {
  PhoshDBusEndSessionDialogSkeleton           parent;
  gboolean                                    active;

  PhoshDBusSessionManager                    *proxy;
  GCancellable                               *cancel;
  PhoshSessionClientPrivateDBusClientPrivate *priv_proxy;

  PhoshEndSessionDialog *dialog;
} PhoshSessionManager;


G_DEFINE_TYPE_WITH_CODE (PhoshSessionManager,
                         phosh_session_manager,
                         PHOSH_DBUS_TYPE_END_SESSION_DIALOG_SKELETON,
                         G_IMPLEMENT_INTERFACE (
                           PHOSH_DBUS_TYPE_END_SESSION_DIALOG,
                           phosh_session_manager_end_session_dialog_iface_init))


static void
on_end_session_dialog_closed (PhoshSessionManager *self, PhoshEndSessionDialog *dialog)
{
  gint action;
  gboolean confirmed;
  PhoshDBusEndSessionDialog *object;

  g_return_if_fail (PHOSH_IS_SESSION_MANAGER (self));
  g_return_if_fail (PHOSH_IS_END_SESSION_DIALOG (dialog));

  object = PHOSH_DBUS_END_SESSION_DIALOG (self);

  confirmed = phosh_end_session_dialog_get_action_confirmed (dialog);
  action = phosh_end_session_dialog_get_action (dialog);
  g_clear_pointer ((PhoshSystemModalDialog**)&self->dialog, phosh_system_modal_dialog_close);

  g_debug ("Action %d confirmed: %d", action, confirmed);

  if (!confirmed) {
    phosh_dbus_end_session_dialog_emit_canceled (object);
    return;
  }

  switch (action) {
  case PHOSH_END_SESSION_ACTION_LOGOUT:
    phosh_dbus_end_session_dialog_emit_confirmed_logout (object);
    break;
  case PHOSH_END_SESSION_ACTION_SHUTDOWN:
    phosh_dbus_end_session_dialog_emit_confirmed_shutdown (object);
    break;
  case PHOSH_END_SESSION_ACTION_REBOOT:
    phosh_dbus_end_session_dialog_emit_confirmed_reboot (object);
    break;
  /* not used by gnome-session */
  case PHOSH_END_SESSION_ACTION_HIBERNATE:
  case PHOSH_END_SESSION_ACTION_SUSPEND:
  case PHOSH_END_SESSION_ACTION_HYBRID_SLEEP:
  default:
    g_return_if_reached ();
  }
}


static gboolean
handle_end_session_open (PhoshDBusEndSessionDialog *object,
                         GDBusMethodInvocation     *invocation,
                         guint                      arg_type,
                         guint                      arg_timestamp,
                         guint                      arg_seconds_to_stay_open,
                         const gchar *const        *arg_inhibitor_object_paths)
{
  PhoshSessionManager *self = PHOSH_SESSION_MANAGER (object);

  g_debug ("DBus call %s, type: %d, seconds %d",
           __func__, arg_type, arg_seconds_to_stay_open);

  if (self->dialog != NULL) {
      g_object_set (self->dialog,
                    "inhibitor-paths", arg_inhibitor_object_paths,
                    NULL);
      gtk_widget_show (GTK_WIDGET (self->dialog));
      phosh_dbus_end_session_dialog_complete_open (
        object, invocation);
      return TRUE;
  }

  self->dialog = PHOSH_END_SESSION_DIALOG (phosh_end_session_dialog_new (arg_type,
                                                                         arg_seconds_to_stay_open,
                                                                         arg_inhibitor_object_paths));
  g_signal_connect_swapped (self->dialog, "closed",
                            G_CALLBACK (on_end_session_dialog_closed), self);

  gtk_widget_show (GTK_WIDGET (self->dialog));
  phosh_dbus_end_session_dialog_complete_open (object, invocation);

  return TRUE;
}


static void
phosh_session_manager_end_session_dialog_iface_init (PhoshDBusEndSessionDialogIface *iface)
{
  iface->handle_open = handle_end_session_open;
}


static void
phosh_session_manager_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  PhoshSessionManager *self = PHOSH_SESSION_MANAGER (object);

  switch (property_id) {
  case PHOSH_SESSION_MANAGER_PROP_ACTIVE:
    g_value_set_boolean (value, self->active);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
on_session_active_changed (PhoshSessionManager     *self,
                           GParamSpec              *pspec,
                           PhoshDBusSessionManager *proxy)
{
  gboolean active;

  g_return_if_fail (PHOSH_IS_SESSION_MANAGER (self));
  g_return_if_fail (PHOSH_DBUS_IS_SESSION_MANAGER_PROXY (proxy));

  active = phosh_dbus_session_manager_get_session_is_active (proxy);

  if (self->active == active)
    return;

  self->active = active;
  g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_SESSION_MANAGER_PROP_ACTIVE]);
  g_debug ("Session is now %sactive", self->active ? "" : "in");
}


static void
on_end_session_response_finish (GObject             *source_object,
                                GAsyncResult        *res,
                                PhoshSessionManager *self)
{
  g_autoptr (GError) err = NULL;

  g_return_if_fail (PHOSH_IS_SESSION_MANAGER (self));

  if (!phosh_session_client_private_dbus_client_private_call_end_session_response_finish (
        self->priv_proxy, res, &err)) {
    g_warning ("Failed end session response: %s", err->message);
    goto out;
  }
out:
  g_object_unref (self);
}


static void
respond_to_end_session (PhoshSessionManager *self, gboolean shutdown)
{
  if (shutdown)
    phosh_shell_fade_out (phosh_shell_get_default (), SESSION_SHUTDOWN_TIMEOUT);

  phosh_session_client_private_dbus_client_private_call_end_session_response (
    self->priv_proxy,
    TRUE,
    "",
    NULL,
    (GAsyncReadyCallback)on_end_session_response_finish,
    g_object_ref (self));
}


static void
on_query_end_session (PhoshSessionManager                        *self,
                      guint                                       flags,
                      PhoshSessionClientPrivateDBusClientPrivate *object)
{
  respond_to_end_session (self, FALSE);
}


static void
on_end_session (PhoshSessionManager                        *self,
                guint                                       flags,
                PhoshSessionClientPrivateDBusClientPrivate *object)
{
  respond_to_end_session (self, TRUE);
}


static void
on_stop (PhoshSessionManager                        *self,
         PhoshSessionClientPrivateDBusClientPrivate *object)
{
  gtk_main_quit ();
}


static void
on_client_private_proxy_new_for_bus_finish (GObject             *source_object,
                                            GAsyncResult        *res,
                                            PhoshSessionManager *self)
{
  g_autoptr (GError) err = NULL;

  g_return_if_fail (PHOSH_IS_SESSION_MANAGER (self));

  self->priv_proxy = phosh_session_client_private_dbus_client_private_proxy_new_for_bus_finish (
    res, &err);
  if (!self->priv_proxy) {
    g_warning ("Failed to get client private proxy: %s", err->message);
    goto out;
  }

  g_debug ("Private client initialized");

  g_object_connect (self->priv_proxy,
                    "swapped-signal::query-end-session", on_query_end_session, self,
                    "swapped-signal::end-session", on_end_session, self,
                    "swapped-signal::stop", on_stop, self,
                    NULL);
out:
  g_object_unref (self);
}


static void
on_client_registered (PhoshDBusSessionManager *proxy,
                      GAsyncResult            *res,
                      PhoshSessionManager     *self)
{
  g_autofree gchar *client_id = NULL;
  g_autoptr (GError) err = NULL;

  if (!phosh_dbus_session_manager_call_register_client_finish (proxy, &client_id, res, &err)) {
    phosh_async_error_warn (err, "Failed to register client");
    return;
  }
  g_debug ("Registered client at '%s'", client_id);

  phosh_session_client_private_dbus_client_private_proxy_new_for_bus (
    G_BUS_TYPE_SESSION,
    G_DBUS_PROXY_FLAGS_NONE,
    BUS_NAME,
    client_id,
    NULL,
    (GAsyncReadyCallback)on_client_private_proxy_new_for_bus_finish,
    g_object_ref (self));
}


static void
on_logout_finished (PhoshDBusSessionManager *proxy,
                    GAsyncResult            *res,
                    PhoshSessionManager     *self)
{
  g_autoptr (GError) err = NULL;

  if (!phosh_dbus_session_manager_call_logout_finish (proxy, res, &err))
    g_warning ("Failed to logout: %s", err->message);
  g_object_unref (self);
}


static void
on_shutdown_finished (PhoshDBusSessionManager *proxy,
                      GAsyncResult            *res,
                      PhoshSessionManager     *self)
{
  g_autoptr (GError) err = NULL;

  if (!phosh_dbus_session_manager_call_shutdown_finish (proxy, res, &err))
    g_warning ("Failed to shutdown: %s", err->message);
  g_object_unref (self);
}


static void
on_reboot_finished (PhoshDBusSessionManager *proxy,
                    GAsyncResult            *res,
                    PhoshSessionManager     *self)
{
  g_autoptr (GError) err = NULL;

  if (!phosh_dbus_session_manager_call_reboot_finish (proxy, res, &err))
    g_warning ("Failed to reboot: %s", err->message);
  g_object_unref (self);
}


static void
phosh_session_manager_constructed (GObject *object)
{
  PhoshSessionManager *self = PHOSH_SESSION_MANAGER (object);

  g_autoptr (GError) err = NULL;

  /* Sync call since this happens early in startup and we need it right away */
  self->proxy = phosh_dbus_session_manager_proxy_new_for_bus_sync (
    G_BUS_TYPE_SESSION,
    G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION,
    BUS_NAME,
    OBJECT_PATH,
    NULL,
    &err);

  if (!self->proxy) {
    g_warning ("Failed to get session proxy %s", err->message);
  } else {
    /* Don't use a property binding so 'active' can be a r/o property */
    g_signal_connect_swapped (self->proxy,
                              "notify::session-is-active",
                              G_CALLBACK (on_session_active_changed),
                              self);
    on_session_active_changed (self, NULL, self->proxy);
  }

  G_OBJECT_CLASS (phosh_session_manager_parent_class)->constructed (object);
}


static void
phosh_session_manager_dispose (GObject *object)
{
  PhoshSessionManager *self = PHOSH_SESSION_MANAGER (object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  if (g_dbus_interface_skeleton_get_object_path (G_DBUS_INTERFACE_SKELETON (self)))
    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (self));

  g_clear_pointer (&self->dialog, phosh_cp_widget_destroy);
  g_clear_object (&self->priv_proxy);
  g_clear_object (&self->proxy);

  G_OBJECT_CLASS (phosh_session_manager_parent_class)->dispose (object);
}


static void
phosh_session_manager_class_init (PhoshSessionManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_session_manager_constructed;
  object_class->dispose = phosh_session_manager_dispose;
  object_class->get_property = phosh_session_manager_get_property;

  /**
   * PhoshSessionManager:active:
   *
   * Whether this phosh instance runs in the currently active session.
   */
  props[PHOSH_SESSION_MANAGER_PROP_ACTIVE] =
    g_param_spec_boolean ("active",
                          "Active",
                          "Active session",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PHOSH_SESSION_MANAGER_PROP_LAST_PROP, props);
}


static void
phosh_session_manager_init (PhoshSessionManager *self)
{
  self->cancel = g_cancellable_new ();
}


PhoshSessionManager *
phosh_session_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_SESSION_MANAGER, NULL);
}


gboolean
phosh_session_manager_is_active (PhoshSessionManager *self)
{
  g_return_val_if_fail (PHOSH_IS_SESSION_MANAGER (self), FALSE);

  return self->active;
}

void
phosh_session_manager_register (PhoshSessionManager *self,
                                const gchar         *app_id,
                                const gchar         *startup_id)
{
  g_return_if_fail (PHOSH_IS_SESSION_MANAGER (self));
  g_return_if_fail (PHOSH_DBUS_IS_SESSION_MANAGER_PROXY (self->proxy));
  g_return_if_fail (app_id != NULL);

  phosh_dbus_session_manager_call_register_client (self->proxy,
                                                   app_id,
                                                   startup_id ? startup_id : "",
                                                   self->cancel,
                                                   (GAsyncReadyCallback) on_client_registered,
                                                   self);
}

void
phosh_session_manager_logout (PhoshSessionManager *self)
{
  g_return_if_fail (PHOSH_IS_SESSION_MANAGER (self));
  g_return_if_fail (PHOSH_DBUS_IS_SESSION_MANAGER_PROXY (self->proxy));

  phosh_dbus_session_manager_call_logout (self->proxy,
                                          1 /* no dialog */,
                                          NULL,
                                          (GAsyncReadyCallback)on_logout_finished,
                                          g_object_ref (self));
}


void
phosh_session_manager_shutdown (PhoshSessionManager *self)
{
  g_return_if_fail (PHOSH_IS_SESSION_MANAGER (self));
  g_return_if_fail (PHOSH_DBUS_IS_SESSION_MANAGER_PROXY (self->proxy));

  phosh_dbus_session_manager_call_shutdown (self->proxy,
                                            NULL,
                                            (GAsyncReadyCallback)on_shutdown_finished,
                                            g_object_ref (self));
}


void
phosh_session_manager_reboot (PhoshSessionManager *self)
{
  g_return_if_fail (PHOSH_IS_SESSION_MANAGER (self));
  g_return_if_fail (PHOSH_DBUS_IS_SESSION_MANAGER_PROXY (self->proxy));

  phosh_dbus_session_manager_call_reboot (self->proxy,
                                          NULL,
                                          (GAsyncReadyCallback)on_reboot_finished,
                                          g_object_ref (self));
}

void
phosh_session_manager_export_end_session (PhoshSessionManager *self,
                                          GDBusConnection     *connection)
{
  g_return_if_fail (PHOSH_IS_SESSION_MANAGER (self));

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                    connection,
                                    END_SESSION_DIALOG_OBJECT_PATH,
                                    NULL);
}

guint
phosh_session_manager_inhibit (PhoshSessionManager      *self,
                               PhoshSessionManagerFlags  what,
                               const char               *reason)
{
  g_autoptr (GError) err = NULL;
  gboolean success;
  guint cookie;

  success = phosh_dbus_session_manager_call_inhibit_sync (self->proxy,
                                                          PHOSH_APP_ID,
                                                          0,
                                                          reason,
                                                          what,
                                                          &cookie,
                                                          self->cancel,
                                                          &err);
  if (!success) {
    g_warning ("Failed to inhibit %d: %s", what, err->message);
    return 0;
  }

  return cookie;
}


void
phosh_session_manager_uninhibit (PhoshSessionManager *self, guint cookie)
{
  g_autoptr (GError) err = NULL;
  gboolean success;

  success = phosh_dbus_session_manager_call_uninhibit_sync (self->proxy,
                                                            cookie,
                                                            self->cancel,
                                                            &err);
  if (!success)
    g_warning ("Failed to uninhibit %u: %s", cookie, err->message);
}
