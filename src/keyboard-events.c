/*
 * Copyright (C) 2020 Evangelos Ribeiro Tzaras
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Evangelos Ribeiro Tzaras <devrtz@fortysixandtwo.eu>
 */

#define G_LOG_DOMAIN "phosh-keyboard-events"

#include "keyboard-events.h"
#include "wlr-screencopy-unstable-v1-client-protocol.h"
#include "phosh-private-client-protocol.h"
#include "phosh-wayland.h"

/**
 * PhoshKeyboardEvents:
 *
 * Grabs and manages special keyboard events
 */

struct _PhoshKeyboardEvents {
  GSimpleActionGroup                   parent;

  struct phosh_private_keyboard_event *kbevent;
  GHashTable                          *accelerators;
};

static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (PhoshKeyboardEvents, phosh_keyboard_events, G_TYPE_SIMPLE_ACTION_GROUP,
                         G_IMPLEMENT_INTERFACE(G_TYPE_INITABLE, initable_iface_init));

static void
handle_accelerator_activated_event (void *data,
                                    struct phosh_private_keyboard_event *kbevent,
                                    uint32_t action_id,
                                    uint32_t timestamp)
{
  PhoshKeyboardEvents *self = PHOSH_KEYBOARD_EVENTS (data);
  const gchar *action;
  GVariant *pressed = NULL;

  action = g_hash_table_lookup (self->accelerators, GUINT_TO_POINTER (action_id));
  g_return_if_fail (action);

  g_debug ("Accelerator %d activated: %s", action_id, action);

  g_return_if_fail (g_action_group_has_action (G_ACTION_GROUP (self), action));

  if (g_action_group_get_action_parameter_type (G_ACTION_GROUP (self), action))
    pressed = g_variant_new_boolean (TRUE);

  g_action_group_activate_action (G_ACTION_GROUP (self), action, pressed);

  /*
   * Emulate key released when running against older phoc, can be
   * removed once we require phoc 0.26.0
   */
  if ((phosh_private_keyboard_event_get_version (kbevent) <
       PHOSH_PRIVATE_KEYBOARD_EVENT_ACCELERATOR_RELEASED_EVENT_SINCE_VERSION) &&
      g_action_group_get_action_parameter_type (G_ACTION_GROUP (self), action)) {
    g_warning_once ("Emulating accelerator up. Please upgrade phoc");
    g_action_group_activate_action (G_ACTION_GROUP (self), action, g_variant_new_boolean (FALSE));
  }
}


static void
handle_accelerator_released_event (void *data,
                                   struct phosh_private_keyboard_event *kbevent,
                                   uint32_t action_id,
                                   uint32_t timestamp)
{
  PhoshKeyboardEvents *self = PHOSH_KEYBOARD_EVENTS (data);
  const gchar *action;

  action = g_hash_table_lookup (self->accelerators, GUINT_TO_POINTER (action_id));
  g_return_if_fail (action);

  g_debug ("Accelerator %d released: %s", action_id, action);

  g_return_if_fail (g_action_group_has_action (G_ACTION_GROUP (self), action));

  /* Action doesn't have a parameter so we only notify press */
  if (g_action_group_get_action_parameter_type (G_ACTION_GROUP (self), action) == NULL)
    return;

  g_action_group_activate_action (G_ACTION_GROUP (self), action, g_variant_new_boolean (FALSE));
}


static void
handle_grab_failed_event (void *data,
                          struct phosh_private_keyboard_event *kbevent,
                          const char *accelerator,
                          uint32_t error)
{
  switch ((enum phosh_private_keyboard_event_error) error) {
  case PHOSH_PRIVATE_KEYBOARD_EVENT_ERROR_ALREADY_SUBSCRIBED:
    g_warning ("Already subscribed to accelerator %s", accelerator);
    break;
  case PHOSH_PRIVATE_KEYBOARD_EVENT_ERROR_INVALID_KEYSYM:
    g_warning ("Accelerator %s not subscribeable", accelerator);
    break;
  case PHOSH_PRIVATE_KEYBOARD_EVENT_ERROR_MISC_ERROR:
  case PHOSH_PRIVATE_KEYBOARD_EVENT_ERROR_INVALID_ARGUMENT:
  default:
    g_warning ("Unknown error %d trying to subscribe accelerator %s", error, accelerator);
  }
}


static void
handle_grab_success_event (void *data,
                           struct phosh_private_keyboard_event *kbevent,
                           const char *accelerator,
                           uint32_t action_id)
{
  PhoshKeyboardEvents *self = PHOSH_KEYBOARD_EVENTS (data);

  g_hash_table_insert (self->accelerators, GUINT_TO_POINTER (action_id), g_strdup (accelerator));
}


static void
handle_ungrab_success_event (void *data,
                             struct phosh_private_keyboard_event *kbevent,
                             uint32_t action_id)
{
  PhoshKeyboardEvents *self = PHOSH_KEYBOARD_EVENTS (data);

  g_return_if_fail (PHOSH_IS_KEYBOARD_EVENTS (data));
  g_debug ("Ungrab of %d successful", action_id);
  g_hash_table_remove (self->accelerators, GUINT_TO_POINTER (action_id));
}


static void
handle_ungrab_failed_event (void *data,
                            struct phosh_private_keyboard_event *kbevent,
                            uint32_t action_id,
                            uint32_t error)
{
  g_warning ("Ungrab of %d failed: %d", action_id, error);
}


static const struct phosh_private_keyboard_event_listener keyboard_event_listener = {
  .accelerator_activated_event = handle_accelerator_activated_event,
  .accelerator_released_event = handle_accelerator_released_event,
  .grab_failed_event = handle_grab_failed_event,
  .grab_success_event = handle_grab_success_event,
  .ungrab_failed_event = handle_ungrab_failed_event,
  .ungrab_success_event = handle_ungrab_success_event,
};


static void
on_action_added (PhoshKeyboardEvents *self,
                 gchar               *action_name,
                 GActionGroup        *action_group)
{
  g_debug ("Grabbing accelerator %s", action_name);
  phosh_private_keyboard_event_grab_accelerator_request (self->kbevent, action_name);
}


static void
on_action_removed (PhoshKeyboardEvents *self,
                   gchar               *action_name,
                   GActionGroup        *action_group)
{
  GHashTableIter iter;
  gpointer key, value;

  g_debug ("Ungrabbing accelerator %s", action_name);

  g_hash_table_iter_init (&iter, self->accelerators);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    if (!g_strcmp0 (action_name, value)) {
      phosh_private_keyboard_event_ungrab_accelerator_request (self->kbevent,
                                                               GPOINTER_TO_UINT (key));
    }
  }
}


static gboolean
initable_init (GInitable    *initable,
               GCancellable *cancelable,
               GError      **error)
{
  struct phosh_private *phosh_private;
  PhoshKeyboardEvents *self = PHOSH_KEYBOARD_EVENTS (initable);

  phosh_private = phosh_wayland_get_phosh_private (
    phosh_wayland_get_default ());

  if (!phosh_private) {
    g_warning ("Skipping grab manager due to missing phosh_private protocol extension");
    g_set_error (error,
                 G_IO_ERROR, G_IO_ERROR_FAILED,
                 "Missing phosh_private protocol extension!");
    return FALSE;
  }

  if (phosh_private_get_version (phosh_private) < PHOSH_PRIVATE_GET_KEYBOARD_EVENT_SINCE_VERSION) {
    g_warning ("Skipping grab manager due to mismatch of phosh_private protocol version");
    g_set_error (error,
                 G_IO_ERROR, G_IO_ERROR_FAILED,
                 "Protocol version mismatch. Need %d, got %d",
                 PHOSH_PRIVATE_GET_KEYBOARD_EVENT_SINCE_VERSION,
                 phosh_private_get_version (phosh_private));
    return FALSE;
  }

  if ((self->kbevent = phosh_private_get_keyboard_event (phosh_private)) == NULL) {
    g_warning ("Skipping grab manager because of an unknown phosh_private protocol error");
    g_set_error (error,
                 G_IO_ERROR, G_IO_ERROR_FAILED,
                 "Unknown protocol error (Running out of memory?)");
    return FALSE;
  }

  phosh_private_keyboard_event_add_listener (self->kbevent, &keyboard_event_listener, self);

  g_signal_connect (self,
                    "action-added",
                    G_CALLBACK (on_action_added),
                    NULL);

  g_signal_connect (self,
                    "action-removed",
                    G_CALLBACK (on_action_removed),
                    NULL);

  return TRUE;
}


static void
initable_iface_init (GInitableIface *iface)
{
  iface->init = initable_init;
}


static void
phosh_keyboard_events_dispose (GObject *object)
{
  PhoshKeyboardEvents *self = PHOSH_KEYBOARD_EVENTS (object);

  g_clear_pointer (&self->kbevent, phosh_private_keyboard_event_destroy);

  G_OBJECT_CLASS (phosh_keyboard_events_parent_class)->dispose (object);
}


static void
phosh_keyboard_events_finalize (GObject *object)
{
  PhoshKeyboardEvents *self = PHOSH_KEYBOARD_EVENTS (object);

  g_clear_pointer (&self->accelerators, g_hash_table_unref);

  G_OBJECT_CLASS (phosh_keyboard_events_parent_class)->finalize (object);
}



static void
phosh_keyboard_events_class_init (PhoshKeyboardEventsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = phosh_keyboard_events_dispose;
  object_class->finalize = phosh_keyboard_events_finalize;
}


static void
phosh_keyboard_events_init (PhoshKeyboardEvents *self)
{
  self->accelerators = g_hash_table_new_full (g_direct_hash,
                                              g_direct_equal,
                                              NULL,
                                              g_free);
}


PhoshKeyboardEvents *
phosh_keyboard_events_new (void)
{
  g_autoptr (GError) err = NULL;
  return g_initable_new (PHOSH_TYPE_KEYBOARD_EVENTS,
                         NULL,
                         &err,
                         NULL);
}
