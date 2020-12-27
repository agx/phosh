/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 *         Evangelos Ribeiro Tzaras <devrtz@fortysixandtwo.eu>
 */

#define G_LOG_DOMAIN "phosh-gnome-shell-manager"

#include "../config.h"

#include "gnome-shell-manager.h"
#include "shell.h"

/**
 * SECTION:gnome-shell-manager
 * @short_description: Provides the org.gnome.Shell DBus interface
 * @Title: PhoshGnomeShellManager
 *
 */

#define NOTIFY_DBUS_NAME "org.gnome.Shell"


static void phosh_gnome_shell_manager_shell_iface_init (PhoshGnomeShellDBusShellIface *iface);

typedef struct _PhoshGnomeShellManager {
  PhoshGnomeShellDBusShellSkeleton parent;

  GHashTable                      *info_by_action;
  guint                            last_action_id;
  int                              dbus_name_id;
} PhoshGnomeShellManager;

G_DEFINE_TYPE_WITH_CODE (PhoshGnomeShellManager,
                         phosh_gnome_shell_manager,
                         PHOSH_GNOME_SHELL_DBUS_TYPE_SHELL_SKELETON,
                         G_IMPLEMENT_INTERFACE (
                           PHOSH_GNOME_SHELL_DBUS_TYPE_SHELL,
                           phosh_gnome_shell_manager_shell_iface_init));

static void accelerator_activated_action (GSimpleAction *action, GVariant *param, gpointer data);

typedef struct _AcceleratorInfo {
  guint                            action_id;
  gchar                           *accelerator;
  gchar                           *sender;
  guint                            mode_flags;
  guint                            grab_flags;
} AcceleratorInfo;

static void
remove_action_entries (const gchar *accelerator)
{
  const GActionEntry action_entries[] = {
    { accelerator, accelerator_activated_action, },
  };

  phosh_shell_remove_global_keyboard_action_entries (phosh_shell_get_default (),
                                                     action_entries,
                                                     G_N_ELEMENTS (action_entries));
}

static void
free_accelerator_info_from_hash_table (gpointer data)
{
  AcceleratorInfo *info = (AcceleratorInfo *) data;
  g_return_if_fail (info != NULL);

  remove_action_entries (info->accelerator);
  g_free (info->accelerator);
  g_free (info->sender);
  g_free (info);
}

/* DBus handlers */
static gboolean
handle_show_monitor_labels (PhoshGnomeShellDBusShell *skeleton,
                            GDBusMethodInvocation    *invocation,
                            GVariant                 *arg_params)
{
  PhoshGnomeShellManager *self = PHOSH_GNOME_SHELL_MANAGER (skeleton);

  g_return_val_if_fail (PHOSH_IS_GNOME_SHELL_MANAGER (self), FALSE);
  g_debug ("DBus show monitor labels");

  phosh_gnome_shell_dbus_shell_complete_show_monitor_labels (
    skeleton, invocation);

  return TRUE;
}

static gboolean
handle_hide_monitor_labels (PhoshGnomeShellDBusShell *skeleton,
                            GDBusMethodInvocation    *invocation)
{
  PhoshGnomeShellManager *self = PHOSH_GNOME_SHELL_MANAGER (skeleton);

  g_return_val_if_fail (PHOSH_IS_GNOME_SHELL_MANAGER (self), FALSE);
  g_debug ("DBus hide monitor labels");

  phosh_gnome_shell_dbus_shell_complete_hide_monitor_labels (
    skeleton, invocation);

  return TRUE;
}


static gboolean
grab_single_accelerator (PhoshGnomeShellManager *self,
                         const gchar            *accelerator,
                         guint                   mode_flags,
                         guint                   grab_flags,
                         AcceleratorInfo        *info,
                         const gchar            *sender,
                         GError                **error)
{
  const GActionEntry action_entries[] = {
    { accelerator, accelerator_activated_action, },
  };

  g_assert (PHOSH_IS_GNOME_SHELL_MANAGER (self));

  /* this should never happen */
  if (g_hash_table_contains (self->info_by_action, GUINT_TO_POINTER (self->last_action_id + 1))) {
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "action id %d already taken", self->last_action_id + 1);
    return FALSE;
  }
  info->accelerator = g_strdup (accelerator);
  info->action_id = ++(self->last_action_id);
  info->sender = g_strdup (sender);
  info->mode_flags = mode_flags;
  info->grab_flags = grab_flags;

  g_debug ("Using action id %d for accelerator %s", info->action_id, info->accelerator);

  g_hash_table_insert (self->info_by_action, GUINT_TO_POINTER (info->action_id), info);

  phosh_shell_add_global_keyboard_action_entries (phosh_shell_get_default (),
                                                  action_entries,
                                                  G_N_ELEMENTS (action_entries),
                                                  info);
  return TRUE;
}

static gboolean
handle_grab_accelerator (PhoshGnomeShellDBusShell *skeleton,
                         GDBusMethodInvocation    *invocation,
                         const gchar              *arg_accelerator,
                         guint                     arg_modeFlags,
                         guint                     arg_grabFlags)
{
  PhoshGnomeShellManager *self = PHOSH_GNOME_SHELL_MANAGER (skeleton);
  AcceleratorInfo *info;
  g_autoptr (GError) error = NULL;
  const gchar * sender;

  g_return_val_if_fail (PHOSH_IS_GNOME_SHELL_MANAGER (self), FALSE);
  g_debug ("DBus grab accelerator %s", arg_accelerator);

  info = g_new0 (AcceleratorInfo, 1);
  if (!info) {
    g_error ("Error allocating memory for AcceleratorInfo");
    return FALSE;
  }
  sender = g_dbus_method_invocation_get_sender (invocation);

  if (!grab_single_accelerator (self,
                                arg_accelerator,
                                arg_modeFlags,
                                arg_grabFlags,
                                info,
                                sender,
                                &error)) {
    g_warning ("Error trying to grab accelerator %s: %s", arg_accelerator, error->message);
    g_dbus_method_invocation_return_error (invocation,
                                           error->domain,
                                           error->code,
                                           "%s",
                                           error->message);
    return TRUE;
  }

  phosh_gnome_shell_dbus_shell_complete_grab_accelerator (
    skeleton, invocation, info->action_id);

  return TRUE;
}

static gboolean
handle_grab_accelerators (PhoshGnomeShellDBusShell *skeleton,
                          GDBusMethodInvocation    *invocation,
                          GVariant                 *arg_accelerators)
{
  PhoshGnomeShellManager *self = PHOSH_GNOME_SHELL_MANAGER (skeleton);
  g_autoptr (GVariantBuilder) builder = NULL;
  GVariantIter *arg_iter;
  gchar *accelerator_name;
  guint accelerator_mode_flags;
  guint accelerator_grab_flags;
  AcceleratorInfo *info;
  g_autoptr (GError) error = NULL;
  const gchar *sender;
  gboolean conflict = FALSE;

  g_return_val_if_fail (PHOSH_IS_GNOME_SHELL_MANAGER (self), FALSE);
  g_debug ("DBus grab accelerators");

  sender = g_dbus_method_invocation_get_sender (invocation);

  builder = g_variant_builder_new (G_VARIANT_TYPE ("au"));
  g_variant_get (arg_accelerators, "a(suu)", &arg_iter);

  while (g_variant_iter_loop (arg_iter, "(&suu)",
                              &accelerator_name,
                              &accelerator_mode_flags,
                              &accelerator_grab_flags)) {
    info = g_new0 (AcceleratorInfo, 1);
    if (!info) {
      g_error ("Error allocating memory for AcceleratorInfo");
      conflict = TRUE;
      break;
    }
    if (!grab_single_accelerator (self,
                                  accelerator_name,
                                  accelerator_mode_flags,
                                  accelerator_grab_flags,
                                  info,
                                  sender,
                                  &error)) {
      g_warning ("Error trying to grab accelerator %s: %s", accelerator_name, error->message);
      g_dbus_method_invocation_return_error (invocation,
                                             error->domain,
                                             error->code,
                                             "%s",
                                             error->message);
      conflict = TRUE;
      break;
    }

    g_variant_builder_add (builder, "u", info->action_id);
  }

  if (conflict) { /* clean up existing bindings */
    g_autoptr (GVariant) unroll_ids = g_variant_builder_end (builder);
    gsize n = g_variant_n_children (unroll_ids);

    for (gsize i = 0; i < n; ++i) {
      guint action_id;

      g_variant_get_child (unroll_ids, i, "u", &action_id);
      g_hash_table_remove (self->info_by_action, GUINT_TO_POINTER (action_id));
    }
  } else { /* success */
    phosh_gnome_shell_dbus_shell_complete_grab_accelerators (
      skeleton, invocation, g_variant_builder_end (builder));
  }

  return TRUE;
}

static gboolean
handle_ungrab_accelerator (PhoshGnomeShellDBusShell *skeleton,
                           GDBusMethodInvocation    *invocation,
                           guint                     arg_action)
{
  PhoshGnomeShellManager *self = PHOSH_GNOME_SHELL_MANAGER (skeleton);
  AcceleratorInfo *info;
  gboolean success = FALSE;
  const gchar *sender;

  g_return_val_if_fail (PHOSH_IS_GNOME_SHELL_MANAGER (self), FALSE);
  g_debug ("DBus ungrab accelerator (id %u)", arg_action);

  sender = g_dbus_method_invocation_get_sender (invocation);
  info = g_hash_table_lookup (self->info_by_action, GUINT_TO_POINTER (arg_action));

  if (info != NULL) {
    if (g_strcmp0 (info->sender, sender) == 0) {
      g_hash_table_remove (self->info_by_action, GUINT_TO_POINTER (info->action_id));
      success = TRUE;
    } else {
      g_debug ("Ungrab not allowed: Sender %s not allowed to ungrab (grabbed by %s)",
                 sender, info->sender);
    }
  }

  phosh_gnome_shell_dbus_shell_complete_ungrab_accelerator (
    skeleton, invocation, success);

  return TRUE;
}

static gboolean
handle_ungrab_accelerators (PhoshGnomeShellDBusShell *skeleton,
                            GDBusMethodInvocation    *invocation,
                            GVariant                 *arg_actions)
{
  gsize n;
  PhoshGnomeShellManager *self = PHOSH_GNOME_SHELL_MANAGER (skeleton);
  AcceleratorInfo *info;
  gboolean success = TRUE;
  const gchar *sender;
  g_return_val_if_fail (PHOSH_IS_GNOME_SHELL_MANAGER (self), FALSE);

  sender = g_dbus_method_invocation_get_sender (invocation);
  n = g_variant_n_children (arg_actions);
  g_debug ("DBus ungrab %" G_GSIZE_FORMAT " accelerators", n);

  for (gsize i = 0; i < n; i++) {
    guint arg_action;
    g_variant_get_child (arg_actions, i, "u", &arg_action);
    info = g_hash_table_lookup (self->info_by_action, GUINT_TO_POINTER (arg_action));
    if (info == NULL) {
      g_warning ("Can't ungrab: No accelerator (id %u) found", arg_action);
      success = FALSE;
      continue;
    }

    if (g_strcmp0 (info->sender, sender) != 0) {
      g_warning ("Ungrab (id %u) not allowed: Sender %s not allowed to ungrab (grabbed by %s)",
                 arg_action, sender, info->sender);
      success = FALSE;
      continue;
    }
    g_hash_table_remove (self->info_by_action, GUINT_TO_POINTER (info->action_id));
  }
  phosh_gnome_shell_dbus_shell_complete_ungrab_accelerators (
    skeleton, invocation, success);

  return TRUE;
}

static void
accelerator_activated (PhoshGnomeShellDBusShell *skeleton,
                       guint                     arg_action,
                       GVariant                 *arg_parameters)
{
  PhoshGnomeShellManager *self = PHOSH_GNOME_SHELL_MANAGER (skeleton);

  g_return_if_fail (PHOSH_IS_GNOME_SHELL_MANAGER (self));
  g_debug ("DBus emitting accelerator activated for action %u", arg_action);

  phosh_gnome_shell_dbus_shell_emit_accelerator_activated (
    skeleton, arg_action, arg_parameters);

}

static void
phosh_gnome_shell_manager_shell_iface_init (PhoshGnomeShellDBusShellIface *iface)
{
  iface->handle_show_monitor_labels = handle_show_monitor_labels;
  iface->handle_hide_monitor_labels = handle_hide_monitor_labels;
  iface->handle_grab_accelerator = handle_grab_accelerator;
  iface->handle_grab_accelerators = handle_grab_accelerators;
  iface->handle_ungrab_accelerator = handle_ungrab_accelerator;
  iface->handle_ungrab_accelerators = handle_ungrab_accelerators;
}

static ShellActionMode
get_action_mode (void)
{
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshShellStateFlags state = phosh_shell_get_state (shell);

  if (state & PHOSH_STATE_LOCKED) {
    /* SHELL_ACTION_MODE_LOCK_SCREEN or SHELL_ACTION_MODE_UNLOCK_SCREEN */
    return SHELL_ACTION_MODE_LOCK_SCREEN | SHELL_ACTION_MODE_UNLOCK_SCREEN;
  }

  if (state & PHOSH_STATE_MODAL_SYSTEM_PROMPT)
    return SHELL_ACTION_MODE_SYSTEM_MODAL;

  if (state & PHOSH_STATE_OVERVIEW)
    return SHELL_ACTION_MODE_OVERVIEW;

  return SHELL_ACTION_MODE_NORMAL;
}

static void
accelerator_activated_action (GSimpleAction *action,
                              GVariant      *param,
                              gpointer       data)
{
  AcceleratorInfo *info = (AcceleratorInfo *) data;
  PhoshGnomeShellManager *self = phosh_gnome_shell_manager_get_default ();
  g_autoptr (GVariantBuilder) builder = NULL;
  GVariant *parameters;
  uint32_t action_id;
  uint32_t action_mode;

  action_id = info->action_id;
  action_mode = get_action_mode ();
  g_debug ("accelerator action activated for id %u", action_id);

  if ((info->mode_flags & action_mode) == 0) {
    g_debug ("Accelerator registered for mode %u, but shell is currently in %u",
             info->mode_flags,
             action_mode);
    return;
  }

  builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
  /*
   * Fill some dummy values
   */
  g_variant_builder_add (builder, "{sv}", "device-id", g_variant_new_uint32 (0));
  g_variant_builder_add (builder, "{sv}", "timestamp", g_variant_new_uint32 (GDK_CURRENT_TIME));
  g_variant_builder_add (builder, "{sv}", "action-mode", g_variant_new_uint32 (0));
  g_variant_builder_add (builder, "{sv}", "device-id", g_variant_new_string ("/dev/input/event0"));
  parameters = g_variant_builder_end (builder);

  accelerator_activated (PHOSH_GNOME_SHELL_DBUS_SHELL (self),
                         action_id,
                         parameters);

}


static void
on_name_acquired (GDBusConnection *connection,
                  const char      *name,
                  gpointer         user_data)
{
  PhoshGnomeShellManager *self = PHOSH_GNOME_SHELL_MANAGER (user_data);

  g_debug ("Acquired name %s", name);
  g_return_if_fail (PHOSH_IS_GNOME_SHELL_MANAGER (self));
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
  PhoshGnomeShellManager *self = user_data;

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                    connection,
                                    "/org/gnome/Shell",
                                    NULL);
}


static void
phosh_gnome_shell_manager_dispose (GObject *object)
{
  PhoshGnomeShellManager *self = PHOSH_GNOME_SHELL_MANAGER (object);
  GList *grabbed;
  guint n = 0;

  for (grabbed = g_hash_table_get_keys (self->info_by_action); grabbed != NULL; grabbed = grabbed->next) {
    AcceleratorInfo *info = grabbed->data;

    g_hash_table_remove (self->info_by_action, GUINT_TO_POINTER (info->action_id));
    ++n;
  }
  g_hash_table_unref (self->info_by_action);

  g_debug ("%d accelerators needed to be cleaned up!", n);

  G_OBJECT_CLASS (phosh_gnome_shell_manager_parent_class)->dispose (object);
}


static void
phosh_gnome_shell_manager_constructed (GObject *object)
{
  PhoshGnomeShellManager *self = PHOSH_GNOME_SHELL_MANAGER (object);

  G_OBJECT_CLASS (phosh_gnome_shell_manager_parent_class)->constructed (object);
  self->dbus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                       NOTIFY_DBUS_NAME,
                                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                                       G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                       on_bus_acquired,
                                       on_name_acquired,
                                       on_name_lost,
                                       g_object_ref (self),
                                       g_object_unref);
}


static void
phosh_gnome_shell_manager_class_init (PhoshGnomeShellManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_gnome_shell_manager_constructed;
  object_class->dispose = phosh_gnome_shell_manager_dispose;
}


static void
phosh_gnome_shell_manager_init (PhoshGnomeShellManager *self)
{
  self->info_by_action = g_hash_table_new_full (g_direct_hash,
                                                g_direct_equal,
                                                NULL,
                                                free_accelerator_info_from_hash_table);
  self->last_action_id = 0;
}


PhoshGnomeShellManager *
phosh_gnome_shell_manager_get_default (void)
{
  static PhoshGnomeShellManager *instance;

  if (instance == NULL) {
    g_debug ("Creating gnome shell DBus manager");
    instance = g_object_new (PHOSH_TYPE_GNOME_SHELL_MANAGER, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *) &instance);
  }
  return instance;
}
