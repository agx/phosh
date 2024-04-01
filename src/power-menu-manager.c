/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "phosh-power-menu-manager"

#include "power-menu-manager.h"
#include "power-menu.h"
#include "shell.h"
#include "util.h"

/**
 * PhoshPowerMenuManager:
 *
 * Handles the power button menu
 *
 * The interface is responsible to handle the non-ui parts of a
 * #PhoshPowerDialog.
 */

typedef struct _PhoshPowerMenuManager {
  GObject                parent;

  PhoshPowerMenu        *dialog;

  GSimpleActionGroup    *menu_actions;
} PhoshPowerMenuManager;

G_DEFINE_TYPE (PhoshPowerMenuManager, phosh_power_menu_manager, G_TYPE_OBJECT)


static void
close_menu (PhoshPowerMenuManager *self)
{
  g_clear_pointer (&self->dialog, phosh_cp_widget_destroy);
}


static void
on_power_menu_done (PhoshPowerMenuManager *self)
{
  g_return_if_fail (PHOSH_IS_POWER_MENU_MANAGER (self));

  close_menu (self);
}


static void
on_power_off_activated (GSimpleAction *action, GVariant *param, gpointer data)
{
  PhoshPowerMenuManager *self = PHOSH_POWER_MENU_MANAGER (data);
  PhoshSessionManager *sm = phosh_shell_get_session_manager (phosh_shell_get_default ());

  g_return_if_fail (PHOSH_IS_POWER_MENU_MANAGER (self));

  close_menu (self);
  phosh_session_manager_shutdown (sm);
}


static void
on_screen_lock_activated (GSimpleAction *action, GVariant *param, gpointer data)
{
  PhoshPowerMenuManager *self = PHOSH_POWER_MENU_MANAGER (data);
  PhoshShell *shell = phosh_shell_get_default ();

  g_return_if_fail (PHOSH_IS_POWER_MENU_MANAGER (self));

  close_menu (self);
  phosh_shell_set_locked (shell, TRUE);
}


static void
on_screenshot_activated (GSimpleAction *action, GVariant *param, gpointer data)

{
  PhoshPowerMenuManager *self = PHOSH_POWER_MENU_MANAGER (data);
  PhoshScreenshotManager *manager =
    phosh_shell_get_screenshot_manager (phosh_shell_get_default ());

  g_return_if_fail (PHOSH_IS_POWER_MENU_MANAGER (self));
  g_return_if_fail (PHOSH_IS_SCREENSHOT_MANAGER (manager));

  close_menu (self);
  phosh_screenshot_manager_do_screenshot (manager, NULL, NULL, FALSE);
}


static void
on_emergency_call_activated (GSimpleAction *action, GVariant *param, gpointer data)
{
  PhoshPowerMenuManager *self = PHOSH_POWER_MENU_MANAGER (data);

  g_return_if_fail (PHOSH_IS_POWER_MENU_MANAGER (self));

  close_menu (self);
  g_action_group_activate_action (G_ACTION_GROUP (phosh_shell_get_default ()),
                                  "emergency.toggle-menu", NULL);
}


static void
on_power_menu_activated (GSimpleAction *action, GVariant *param, gpointer data)
{
  PhoshPowerMenuManager *self = PHOSH_POWER_MENU_MANAGER (data);

  if (self->dialog) {
    on_power_menu_done (self);
    return;
  }

  self->dialog = phosh_power_menu_new (NULL);
  gtk_widget_insert_action_group (GTK_WIDGET (self->dialog), "power-menu",
                                  G_ACTION_GROUP (self->menu_actions));
  g_signal_connect_swapped (self->dialog, "done",
                            G_CALLBACK (on_power_menu_done), self);

  gtk_widget_show (GTK_WIDGET (self->dialog));
}


static void
on_shell_state_changed (PhoshPowerMenuManager *self, GParamSpec *pspec, PhoshShell *shell)
{
  GAction *action;
  gboolean enable = TRUE;
  PhoshShellStateFlags state;

  g_return_if_fail (PHOSH_IS_POWER_MENU_MANAGER (self));
  g_return_if_fail (PHOSH_IS_SHELL (shell));

  state = phosh_shell_get_state (shell);
  if (state & PHOSH_STATE_LOCKED)
    enable = FALSE;

  /* TODO: Make them work on the lock screen too */
  action = g_action_map_lookup_action (G_ACTION_MAP (self->menu_actions), "screen-lock");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enable);
  action = g_action_map_lookup_action (G_ACTION_MAP (self->menu_actions), "poweroff");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enable);
}


static void
phosh_power_menu_manager_finalize (GObject *object)
{
  PhoshPowerMenuManager *self = PHOSH_POWER_MENU_MANAGER (object);

  g_action_map_remove_action (G_ACTION_MAP (phosh_shell_get_default ()), "power.toggle-menu");

  g_clear_object (&self->menu_actions);
  g_clear_pointer (&self->dialog, phosh_cp_widget_destroy);

  G_OBJECT_CLASS (phosh_power_menu_manager_parent_class)->finalize (object);
}


static void
phosh_power_menu_manager_class_init (PhoshPowerMenuManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phosh_power_menu_manager_finalize;
}


static GActionEntry menu_entries[] = {
  { .name = "poweroff", .activate = on_power_off_activated },
  { .name = "screen-lock", .activate = on_screen_lock_activated },
  { .name = "screenshot", .activate = on_screenshot_activated },
  { .name = "emergency-call", .activate = on_emergency_call_activated },
};


static GActionEntry entries[] = {
  { .name = "power.toggle-menu", .activate = on_power_menu_activated },
};


static void
phosh_power_menu_manager_init (PhoshPowerMenuManager *self)
{
  GAction *src_action, *dst_action;

  g_action_map_add_action_entries (G_ACTION_MAP (phosh_shell_get_default ()),
                                   entries,
                                   G_N_ELEMENTS (entries),
                                   self);

  self->menu_actions = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (self->menu_actions),
                                   menu_entries, G_N_ELEMENTS (menu_entries),
                                   self);

  /* Only enable emergency call button when the emergency-calls manager says it's o.k. */
  src_action = g_action_map_lookup_action (G_ACTION_MAP (phosh_shell_get_default ()),
                                           "emergency.toggle-menu");
  dst_action = g_action_map_lookup_action (G_ACTION_MAP (self->menu_actions), "emergency-call");
  g_object_bind_property (src_action, "enabled", dst_action, "enabled", G_BINDING_SYNC_CREATE);

  g_signal_connect_object (phosh_shell_get_default (),
                           "notify::shell-state",
                           G_CALLBACK (on_shell_state_changed),
                           self,
                           G_CONNECT_SWAPPED);
  on_shell_state_changed (self, NULL, phosh_shell_get_default ());
}


PhoshPowerMenuManager *
phosh_power_menu_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_POWER_MENU_MANAGER, NULL);
}
