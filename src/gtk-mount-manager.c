/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-gtk-mount-manager"

#include "gtk-mount-manager.h"
#include "gtk-mount-prompt.h"
#include "shell.h"
#include "util.h"

#include <gio/gio.h>

/**
 * PhoshGtkMountManager:
 *
 * Provides the org.Gtk.GtkMountOperationHandler DBus interface
 *
 * The interface is responsible to handle the ui parts of a #GtkMountOperation.
 */

#define GTK_MOUNT_DBUS_NAME "org.gtk.MountOperationHandler"

enum {
  NEW_PROMPT,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

static void phosh_gtk_mount_manager_gtk_mount_iface_init (
  PhoshDBusMountOperationHandlerIface *iface);

typedef struct _PhoshGtkMountManager {
  PhoshDBusMountOperationHandlerSkeleton parent;

  int                                    dbus_name_id;
  PhoshGtkMountPrompt                   *prompt;
  char                                  *object_id;

  GDBusMethodInvocation                 *invocation;
} PhoshGtkMountManager;

G_DEFINE_TYPE_WITH_CODE (PhoshGtkMountManager,
                         phosh_gtk_mount_manager,
                         PHOSH_DBUS_TYPE_MOUNT_OPERATION_HANDLER_SKELETON,
                         G_IMPLEMENT_INTERFACE (
                           PHOSH_DBUS_TYPE_MOUNT_OPERATION_HANDLER,
                           phosh_gtk_mount_manager_gtk_mount_iface_init));


static void
on_prompt_done (PhoshGtkMountManager *self, PhoshGtkMountPrompt *prompt)
{
  gboolean cancelled;
  GMountOperationResult response = G_MOUNT_OPERATION_ABORTED;
  g_autoptr (GVariantDict) response_details = g_variant_dict_new (NULL);

  g_return_if_fail (PHOSH_IS_GTK_MOUNT_MANAGER (self));
  g_return_if_fail (PHOSH_IS_GTK_MOUNT_PROMPT (prompt));

  cancelled = phosh_gtk_mount_prompt_get_cancelled (prompt);
  g_debug ("Prompt done, cancelled: %d", cancelled);

  if (phosh_gtk_mount_prompt_get_choices (prompt)) { /* AskQuestion / ShowProcesses */
    int choice = phosh_gtk_mount_prompt_get_choice (prompt);

    if (!cancelled) {
      response = G_MOUNT_OPERATION_HANDLED;
      g_variant_dict_insert (response_details, "choice", "i", choice);
    }
  } else { /* AskPassword / Default buttons */
    if (!cancelled) {
      const char *password;

      response = G_MOUNT_OPERATION_HANDLED;
      password = phosh_gtk_mount_prompt_get_password (prompt);
      g_variant_dict_insert (response_details, "password", "s", password ?: "", NULL);
      /*
       * GTKs gtkmountoperation:call_password_proxy_cb only cares about
       * 'password', 'password_save', 'hidden_volume', 'system_volume' and 'pim'
       * so no need to bother with usernames and domains here
       */
      /* TODO: 'password_save' */
    }
  }

  if (self->invocation) {
    /*
     * phosh_dbus_mount_operation_handler_complete_ask_question has the same
     * signature and params so we can use either one
     */
    phosh_dbus_mount_operation_handler_complete_ask_password (
      PHOSH_DBUS_MOUNT_OPERATION_HANDLER (self),
      self->invocation, response,
      g_variant_dict_end (response_details));
    self->invocation = NULL;
  } else {
    g_warning ("No invocation!");
  }
}


static void
end_ask_invocation (PhoshGtkMountManager *self)
{
  g_autoptr (GVariantDict) response_details = g_variant_dict_new (NULL);

  if (!self->invocation)
    return;

  /*
   * phosh_dbus_mount_operation_handler_complete_* all have the same
   * signature and params so we can any of them:
   */
  phosh_dbus_mount_operation_handler_complete_ask_password (
    PHOSH_DBUS_MOUNT_OPERATION_HANDLER (self),
    self->invocation, G_MOUNT_OPERATION_UNHANDLED,
    g_variant_dict_end (response_details));
  self->invocation = NULL;
}


static void
new_prompt (PhoshGtkMountManager *self,
            const char           *message,
            const char           *icon_name,
            const char           *default_user,
            const char           *default_domain,
            GVariant             *pids,
            const char *const    *choices,
            GAskPasswordFlags     ask_flags)
{
  g_debug ("New prompt for '%s'", message);

  g_clear_pointer ((PhoshSystemModalDialog**)&self->prompt, phosh_system_modal_dialog_close);

  self->prompt = PHOSH_GTK_MOUNT_PROMPT (phosh_gtk_mount_prompt_new (
                                           message,
                                           icon_name,
                                           default_user,
                                           default_domain,
                                           pids,
                                           choices,
                                           ask_flags));
  g_signal_connect_swapped (self->prompt,
                            "closed",
                            G_CALLBACK (on_prompt_done),
                            self);

  gtk_widget_show (GTK_WIDGET (self->prompt));

  g_signal_emit (self, signals[NEW_PROMPT], 0);
}


static gboolean
handle_ask_password (PhoshDBusMountOperationHandler *object,
                     GDBusMethodInvocation          *invocation,
                     const char                     *arg_object_id,
                     const char                     *arg_message,
                     const char                     *arg_icon_name,
                     const char                     *arg_default_user,
                     const char                     *arg_default_domain,
                     guint                           arg_flags)
{
  PhoshGtkMountManager *self = PHOSH_GTK_MOUNT_MANAGER (object);

  g_debug ("DBus call AskPassword for '%s'", arg_object_id);

  if (self->invocation)
    end_ask_invocation (self);
  self->invocation = invocation;

  new_prompt (self,
              arg_message,
              arg_icon_name,
              arg_default_user,
              arg_default_domain,
              NULL,
              NULL,
              arg_flags);

  return TRUE;
}

static gboolean
handle_ask_question ( PhoshDBusMountOperationHandler *object,
                      GDBusMethodInvocation          *invocation,
                      const char                     *arg_object_id,
                      const char                     *arg_message,
                      const char                     *arg_icon_name,
                      const char *const              *arg_choices)
{
  PhoshGtkMountManager *self = PHOSH_GTK_MOUNT_MANAGER (object);

  g_debug ("DBus call AskQuestion: %s", arg_object_id);

  /* Cancel an ongoing dialog */
  if (self->invocation)
    end_ask_invocation (self);
  self->invocation = invocation;

  new_prompt (self,
              arg_message,
              arg_icon_name,
              NULL,
              NULL,
              NULL,
              arg_choices,
              0);

  return TRUE;
}

static gboolean
handle_show_processes (PhoshDBusMountOperationHandler *object,
                       GDBusMethodInvocation          *invocation,
                       const char                     *arg_object_id,
                       const char                     *arg_message,
                       const char                     *arg_icon_name,
                       GVariant                       *arg_application_pids,
                       const char *const              *arg_choices)
{
  PhoshGtkMountManager *self = PHOSH_GTK_MOUNT_MANAGER (object);

  g_debug ("DBus call ShowProcesses: %s", arg_object_id);

  if (arg_object_id == NULL) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_ACCESS_DENIED,
                                           "No object id");
    invocation = NULL;
    return TRUE;
  }

  if (!g_strcmp0 (arg_object_id, self->object_id) && self->prompt) {
    g_debug ("Updating dialog %s", self->object_id);

    phosh_gtk_mount_prompt_set_pids (self->prompt, arg_application_pids);
    self->invocation = invocation;
    return TRUE;
  }

  g_clear_pointer (&self->object_id, g_free);
  self->object_id = g_strdup (arg_object_id);

  if (self->invocation)
    end_ask_invocation (self);
  self->invocation = invocation;

  new_prompt (self,
              arg_message,
              arg_icon_name,
              NULL,
              NULL,
              arg_application_pids,
              arg_choices,
              0);

  return TRUE;
}

static gboolean
handle_close (PhoshDBusMountOperationHandler *object,
              GDBusMethodInvocation          *invocation)
{
  PhoshGtkMountManager *self = PHOSH_GTK_MOUNT_MANAGER (object);

  g_debug ("DBus call Close");

  end_ask_invocation (self);

  g_clear_pointer ((PhoshSystemModalDialog**)&self->prompt, phosh_system_modal_dialog_close);
  g_clear_pointer (&self->object_id, g_free);

  phosh_dbus_mount_operation_handler_complete_close (
    object, invocation);

  return TRUE;
}

static void
phosh_gtk_mount_manager_gtk_mount_iface_init (PhoshDBusMountOperationHandlerIface *iface)
{
  iface->handle_ask_password = handle_ask_password;
  iface->handle_ask_question = handle_ask_question;
  iface->handle_close = handle_close;
  iface->handle_show_processes = handle_show_processes;
}


static void
on_name_acquired (GDBusConnection *connection,
                  const char      *name,
                  gpointer         user_data)
{
  PhoshGtkMountManager *self = PHOSH_GTK_MOUNT_MANAGER (user_data);

  g_debug ("Acquired name %s", name);
  g_return_if_fail (PHOSH_IS_GTK_MOUNT_MANAGER (self));
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
  PhoshGtkMountManager *self = user_data;

  g_autoptr (GError) err = NULL;

  if (g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                        connection,
                                        "/org/gtk/MountOperationHandler",
                                        &err)) {
    g_debug ("Mount operation handler exported");
  } else {
    g_warning ("Failed to export on %s: %s", GTK_MOUNT_DBUS_NAME, err->message);
  }
}


static void
phosh_gtk_mount_manager_dispose (GObject *object)
{
  PhoshGtkMountManager *self = PHOSH_GTK_MOUNT_MANAGER (object);

  end_ask_invocation (self);
  g_clear_pointer (&self->prompt, phosh_cp_widget_destroy);
  if (g_dbus_interface_skeleton_get_object_path (G_DBUS_INTERFACE_SKELETON (self)))
    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (self));

  g_clear_handle_id (&self->dbus_name_id, g_bus_unown_name);

  G_OBJECT_CLASS (phosh_gtk_mount_manager_parent_class)->dispose (object);
}


static void
phosh_gtk_mount_manager_finalize (GObject *object)
{
  PhoshGtkMountManager *self = PHOSH_GTK_MOUNT_MANAGER (object);

  g_clear_pointer (&self->object_id, g_free);

  G_OBJECT_CLASS (phosh_gtk_mount_manager_parent_class)->finalize (object);
}


static void
phosh_gtk_mount_manager_constructed (GObject *object)
{
  PhoshGtkMountManager *self = PHOSH_GTK_MOUNT_MANAGER (object);

  G_OBJECT_CLASS (phosh_gtk_mount_manager_parent_class)->constructed (object);
  self->dbus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                       GTK_MOUNT_DBUS_NAME,
                                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                                       G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                       on_bus_acquired,
                                       on_name_acquired,
                                       on_name_lost,
                                       self,
                                       NULL);
}


static void
phosh_gtk_mount_manager_class_init (PhoshGtkMountManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_gtk_mount_manager_constructed;
  object_class->dispose = phosh_gtk_mount_manager_dispose;
  object_class->finalize = phosh_gtk_mount_manager_finalize;

  /**
   * PhoshGtkMountManager::new-prompt
   * @self: The #PhoshGtkMountManager emitting this signal
   *
   * Emitted when a new prompt is shown
   */
  signals[NEW_PROMPT] = g_signal_new (
    "new-prompt",
    G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
    NULL, G_TYPE_NONE, 0);
}


static void
phosh_gtk_mount_manager_init (PhoshGtkMountManager *self)
{
}


PhoshGtkMountManager *
phosh_gtk_mount_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_GTK_MOUNT_MANAGER, NULL);
}
