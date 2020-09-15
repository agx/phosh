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
 * SECTION:keyboard-events
 * @short_description: Grabs and manages special keyboard events
 * @Title: PhoshKeyboardEvents
 */


enum {
  SIGNAL_ACCELERATOR_ACTIVATED,
  SIGNAL_ACCELERATOR_GRABBED,
  N_SIGNALS,
};
static guint signals[N_SIGNALS] = { 0 };

struct _PhoshKeyboardEvents {
  GObject parent;

  struct phosh_private_keyboard_event *kbevent;

};

static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (PhoshKeyboardEvents, phosh_keyboard_events, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE(G_TYPE_INITABLE, initable_iface_init));


static void
handle_accelerator_activated_event (void *data,
                                    struct phosh_private_keyboard_event *kbevent,
                                    uint32_t action_id,
                                    uint32_t timestamp)
{
  PhoshKeyboardEvents *self = PHOSH_KEYBOARD_EVENTS (data);
  g_debug ("incoming action! %d", action_id);
  /** Emitting a signal allows other "modules" to connect to a signal
   *  and take appropriate action (f.e. change the volume)
   */

  g_signal_emit (self,
                 signals[SIGNAL_ACCELERATOR_ACTIVATED],
                 0,
                 action_id,
                 timestamp);
}


static void
handle_grab_failed_event (void *data,
                          struct phosh_private_keyboard_event *kbevent,
                          const char *accelerator,
                          uint32_t error)
{
  switch ((enum phosh_private_keyboard_event_error) error) {
  case PHOSH_PRIVATE_KEYBOARD_EVENT_ERROR_ALREADY_SUBSCRIBED:
    g_warning ("Already subscribed to accelerator %s\n", accelerator);
    break;
  case PHOSH_PRIVATE_KEYBOARD_EVENT_ERROR_INVALID_KEYSYM:
    g_warning ("Accelerator %s not subscribeable\n", accelerator);
    break;
  case PHOSH_PRIVATE_KEYBOARD_EVENT_ERROR_MISC_ERROR:
  case PHOSH_PRIVATE_KEYBOARD_EVENT_ERROR_INVALID_ARGUMENT:
  default:
    g_warning ("Unknown error %d trying to subscribe accelerator %s\n", error, accelerator);
  }
}


static void
handle_grab_success_event (void *data,
                           struct phosh_private_keyboard_event *kbevent,
                           const char *accelerator,
                           uint32_t action_id)
{
  PhoshKeyboardEvents *self = PHOSH_KEYBOARD_EVENTS (data);
  g_signal_emit (self,
                 signals[SIGNAL_ACCELERATOR_GRABBED],
                 0,
                 accelerator,
                 action_id);
}

static const struct phosh_private_keyboard_event_listener keyboard_event_listener = {
  .accelerator_activated_event = handle_accelerator_activated_event,
  .grab_failed_event = handle_grab_failed_event,
  .grab_success_event = handle_grab_success_event,
};


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

  if (phosh_private_get_version (phosh_private) < 5) {
    g_warning ("Skipping grab manager due to mismatch of phosh_private protocol version");
    g_set_error (error,
                 G_IO_ERROR, G_IO_ERROR_FAILED,
                 "Protocol version mismatch. Need 5, got %d",
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
  g_clear_pointer(&self->kbevent, phosh_private_keyboard_event_destroy);

  G_OBJECT_CLASS (phosh_keyboard_events_parent_class)->dispose (object);
}


static void
phosh_keyboard_events_class_init (PhoshKeyboardEventsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = phosh_keyboard_events_dispose;

  /**
   * PhoshKeyboardEvents::keypress:
   * @kbevent: The #PhoshKeyboardEvents emitting the signal.
   * @action_id: The id of the forwarded action
   * @timestamp: The timestamp when the key has been pressed
   *
   * Emitted whenever a subscribed accelerator/action has been received
   */
  signals[SIGNAL_ACCELERATOR_ACTIVATED] = g_signal_new (
    "accelerator-activated",
    G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
    NULL, G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);

  /**
   * PhoshKeyboardEvents::accelerator-grabbed:
   * @kbevent: The #PhoshKeyboardEvents emitting the signal.
   * @accelerator: The accelerator which has been grabbed
   * @action_id: The assigned id of the accelerator
   *
   * Emitted whenever an accelerator subscription has been successfull
   */

  signals[SIGNAL_ACCELERATOR_GRABBED] = g_signal_new (
    "accelerator-grabbed",
    G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
    NULL, G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_UINT);

}


void
phosh_keyboard_events_register_keys (PhoshKeyboardEvents *self,
                                     char               **accelerators,
                                     size_t               len)
{
  g_return_if_fail (self->kbevent);

  for (size_t i = 0; i < len; ++i) {
    phosh_private_keyboard_event_grab_accelerator_request (self->kbevent, accelerators[i]);
  }
}


static void
phosh_keyboard_events_init (PhoshKeyboardEvents *self)
{
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
