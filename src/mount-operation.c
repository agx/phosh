/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-mount-operation"

#include "phosh-config.h"

#include "gtk-mount-prompt.h"
#include "mount-operation.h"
#include "util.h"

/**
 * PhoshMountOperation:
 *
 * #GMountOperation using UI
 *
 * A #GMountOperation that uses system modal dialogs for input.
 */

struct _PhoshMountOperation {
  GMountOperation      parent;

  PhoshGtkMountPrompt *prompt;
};
G_DEFINE_TYPE (PhoshMountOperation, phosh_mount_operation, G_TYPE_MOUNT_OPERATION)


static void
on_prompt_done (PhoshMountOperation *self, PhoshGtkMountPrompt *prompt)
{
  gboolean cancelled;
  GMountOperationResult result = G_MOUNT_OPERATION_ABORTED;

  g_return_if_fail (PHOSH_IS_MOUNT_OPERATION (self));
  g_return_if_fail (PHOSH_IS_GTK_MOUNT_PROMPT (prompt));

  cancelled = phosh_gtk_mount_prompt_get_cancelled (prompt);
  g_debug ("Prompt done, cancelled: %d", cancelled);

  if (!cancelled) {
    GAskPasswordFlags flags = phosh_gtk_mount_prompt_get_ask_flags (prompt);

    if (flags & G_ASK_PASSWORD_NEED_PASSWORD) {
      const char *password;

      password = phosh_gtk_mount_prompt_get_password (prompt);
      g_mount_operation_set_password (G_MOUNT_OPERATION (self), password);
      result = G_MOUNT_OPERATION_HANDLED;
    }
  }

  g_mount_operation_reply (G_MOUNT_OPERATION (self), result);
  g_clear_pointer (&self->prompt, phosh_cp_widget_destroy);
}


static void
new_prompt (PhoshMountOperation *self,
            const char          *message,
            const char          *icon_name,
            const char          *default_user,
            const char          *default_domain,
            GVariant            *pids,
            const char *const   *choices,
            GAskPasswordFlags    ask_flags)
{
  g_debug ("New prompt for '%s'", message);

  g_clear_pointer (&self->prompt, phosh_cp_widget_destroy);

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
}


static void
phosh_mount_operation_ask_password (GMountOperation  *op,
                                    const char       *message,
                                    const char       *default_user,
                                    const char       *default_domain,
                                    GAskPasswordFlags flags)
{
  PhoshMountOperation *self = PHOSH_MOUNT_OPERATION (op);

  new_prompt (self, message,
              NULL,             /* icon */
              default_user,
              default_domain,
              NULL,             /* default_user */
              NULL,             /* choices */
              flags);
}


static void
phosh_mount_operation_dispose (GObject *object)
{
  PhoshMountOperation *self = PHOSH_MOUNT_OPERATION (object);

  g_clear_pointer (&self->prompt, phosh_cp_widget_destroy);

  G_OBJECT_CLASS (phosh_mount_operation_parent_class)->dispose (object);
}


static void
phosh_mount_operation_class_init (PhoshMountOperationClass *klass)
{
  GMountOperationClass *mount_op_class = G_MOUNT_OPERATION_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = phosh_mount_operation_dispose;

  mount_op_class->ask_password = phosh_mount_operation_ask_password;
}


static void
phosh_mount_operation_init (PhoshMountOperation *self)
{
}


PhoshMountOperation *
phosh_mount_operation_new (void)
{
  return PHOSH_MOUNT_OPERATION (g_object_new (PHOSH_TYPE_MOUNT_OPERATION, NULL));
}
