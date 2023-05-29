/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-hks-manager"

#include "phosh-config.h"

#include "hks-manager.h"
#include "shell.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/rfkill.h>
#include <sys/stat.h>
#include <sys/types.h>


#ifdef HAVE_RFKILL_EVENT_EXT
typedef struct rfkill_event_ext RfKillEvent;
#else
typedef struct rfkill_event RfKillEvent;
#endif
G_DEFINE_AUTOPTR_CLEANUP_FUNC (RfKillEvent, g_free);

/* rfkill types not yet upstream */
#define RFKILL_TYPE_CAMERA_ 9
#define RFKILL_TYPE_MIC_ 10

/**
 * PhoshHksManager:
 *
 * Tracks hardware kill switch state
 *
 * Monitor hardware kill switch state. This will be submitted to gnome-settings-daemon
 * once we figured out the kernel interfaces.
 */

typedef enum {
  PROP_0,
  /* Each hks must be in the order of present, blocked, icon_name */
  PROP_MIC_PRESENT,
  PROP_MIC_BLOCKED,
  PROP_MIC_ICON_NAME,
  PROP_CAMERA_PRESENT,
  PROP_CAMERA_BLOCKED,
  PROP_CAMERA_ICON_NAME,
  PROP_LAST_PROP
} PhoshHksManagerProps;
static GParamSpec *props[PROP_LAST_PROP];


enum {
  HKS_STATE_BLOCKED = 0,
  HKS_STATE_UNBLOCKED = 1,
};


typedef struct {
  gboolean present;
  gboolean blocked;
  gchar   *blocked_icon_name;
  gchar   *unblocked_icon_name;
  GHashTable *killswitches;
} Hks;


struct _PhoshHksManager {
  GObject     parent;

  GIOChannel *channel;
  int         watch_id;

  Hks         mic;
  Hks         camera;
};
G_DEFINE_TYPE (PhoshHksManager, phosh_hks_manager, G_TYPE_OBJECT);


static void
phosh_hks_manager_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PhoshHksManager *self = PHOSH_HKS_MANAGER (object);
  const gchar *icon_name;

  switch (property_id) {
  case PROP_MIC_PRESENT:
    g_value_set_boolean (value, self->mic.present);
    break;
  case PROP_MIC_BLOCKED:
    g_value_set_boolean (value, self->mic.blocked);
    break;
  case PROP_MIC_ICON_NAME:
    icon_name = self->mic.blocked ?
      self->mic.blocked_icon_name : self->mic.unblocked_icon_name;
    g_value_set_string (value, icon_name);
    break;
  case PROP_CAMERA_PRESENT:
    g_value_set_boolean (value, self->camera.present);
    break;
  case PROP_CAMERA_BLOCKED:
    g_value_set_boolean (value, self->camera.blocked);
    break;
  case PROP_CAMERA_ICON_NAME:
    icon_name = self->camera.blocked ?
      self->camera.blocked_icon_name : self->camera.unblocked_icon_name;
    g_value_set_string (value, icon_name);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static const char *
type_to_string (unsigned int type)
{
  switch (type) {
  case RFKILL_TYPE_ALL:
    return "ALL";
  case RFKILL_TYPE_WLAN:
    return "WLAN";
  case RFKILL_TYPE_BLUETOOTH:
    return "BLUETOOTH";
  case RFKILL_TYPE_UWB:
    return "UWB";
  case RFKILL_TYPE_WIMAX:
    return "WIMAX";
  case RFKILL_TYPE_WWAN:
    return "WWAN";
  case RFKILL_TYPE_CAMERA_:
    return "CAMERA";
  case RFKILL_TYPE_MIC_:
    return "MIC";
  default:
    return "UNKNOWN";
  }
}


static const char *
op_to_string (unsigned int op)
{
  switch (op) {
  case RFKILL_OP_ADD:
    return "ADD";
  case RFKILL_OP_DEL:
    return "DEL";
  case RFKILL_OP_CHANGE:
    return "CHANGE";
  case RFKILL_OP_CHANGE_ALL:
    return "CHANGE_ALL";
  default:
    return "Unknown";
  }
}


static void
print_event (RfKillEvent *event)
{
  g_debug ("RFKILL event: idx %u type %u (%s) op %u (%s) soft %u hard %u",
           event->idx,
           event->type, type_to_string (event->type),
           event->op, op_to_string (event->op),
           event->soft, event->hard);
}


static void
update_props (PhoshHksManager *self, Hks *hks, PhoshHksManagerProps prop)
{
  GHashTableIter iter;
  gpointer key, value;
  gboolean blocked = FALSE;

  if (g_hash_table_size (hks->killswitches) == 0) {
    if (!hks->present)
      return;

    if (hks->blocked) {
      hks->blocked = FALSE;
      g_object_notify_by_pspec (G_OBJECT (self), props[prop+1]);
      g_object_notify_by_pspec (G_OBJECT (self), props[prop+2]);
    }

    hks->present = FALSE;
    g_object_notify_by_pspec (G_OBJECT (self), props[prop]);
    return;
  }

  g_hash_table_iter_init (&iter, hks->killswitches);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    int state;

    state = GPOINTER_TO_INT (value);
    /* A single blocked switch is enough  */
    if (state == HKS_STATE_BLOCKED) {
      blocked = TRUE;
      break;
    }
  }

  if (!hks->present) {
    hks->present = TRUE;
    g_object_notify_by_pspec (G_OBJECT (self), props[prop]);
  }

  if (blocked == hks->blocked)
    return;
  hks->blocked = blocked;

  /* blocked property */
  g_object_notify_by_pspec (G_OBJECT (self), props[prop + 1]);
  /* icon_name property */
  g_object_notify_by_pspec (G_OBJECT (self), props[prop + 2]);
}


static void
process_events (PhoshHksManager *self, GList *events)
{
  GList *l;
  int value;

  for (l = events; l != NULL; l = l->next) {
    RfKillEvent *event = l->data;

    switch (event->op) {
    case RFKILL_OP_ADD:
    case RFKILL_OP_CHANGE:
      value = event->hard ? HKS_STATE_BLOCKED : HKS_STATE_UNBLOCKED;

      if (event->type == RFKILL_TYPE_MIC_) {
        g_hash_table_insert (self->mic.killswitches,
                             GINT_TO_POINTER (event->idx),
                             GINT_TO_POINTER (value));
      } else if (event->type == RFKILL_TYPE_CAMERA_) {
        g_hash_table_insert (self->camera.killswitches,
                             GINT_TO_POINTER (event->idx),
                             GINT_TO_POINTER (value));
      }
      g_debug ("%s rfkill type %d, ID %d",
               event->op == RFKILL_OP_ADD ? "Added" : "Changed",
               event->type, event->idx);
      break;
    case RFKILL_OP_DEL:
      if (event->type == RFKILL_TYPE_MIC_) {
        g_hash_table_remove (self->mic.killswitches,
                             GINT_TO_POINTER (event->idx));
      } else if (event->type == RFKILL_TYPE_CAMERA_) {
        g_hash_table_remove (self->camera.killswitches,
                             GINT_TO_POINTER (event->idx));
      }
      g_debug ("Removed rfkill type %d, ID %d",
               event->type, event->idx);
      break;
    default:
      /* Nothing to do */
      break;
    }
  }

  g_object_freeze_notify (G_OBJECT (self));
  update_props (self, &self->mic, PROP_MIC_PRESENT);
  update_props (self, &self->camera, PROP_CAMERA_PRESENT);
  g_object_thaw_notify (G_OBJECT (self));


}


static gboolean
rfkill_event_cb (GIOChannel      *source,
                 GIOCondition     condition,
                 PhoshHksManager *self)
{
  g_autolist (RfKillEvent) events = NULL;

  if (condition & G_IO_IN) {
    GIOStatus status;
    RfKillEvent event = { 0 };
    RfKillEvent *event_ptr;
    gsize read;

    status = g_io_channel_read_chars (source,
                                      (char *) &event,
                                      sizeof(event),
                                      &read,
                                      NULL);

    while (status == G_IO_STATUS_NORMAL && read >= RFKILL_EVENT_SIZE_V1) {
      print_event (&event);
      event_ptr = g_memdup2 (&event, sizeof(event));
      events = g_list_prepend (events, event_ptr);

      status = g_io_channel_read_chars (source,
                                        (char *) &event,
                                        sizeof(event),
                                        &read,
                                        NULL);
    }
    events = g_list_reverse (events);
  } else {
    g_debug ("Something unexpected happened on rfkill fd");
    return FALSE;
  }

  process_events (self, events);
  return TRUE;
}


static gboolean
setup_rfkill (PhoshHksManager *self)
{
  int fd, ret;
  g_autolist (RfKillEvent) events = NULL;

  fd = open ("/dev/rfkill", O_RDONLY);
  if (fd < 0)
    return FALSE;

  ret = fcntl (fd, F_SETFL, O_NONBLOCK);
  if (ret < 0) {
    g_warning ("Can't set RFKILL control device to non-blocking: %s", strerror (errno));
    close (fd);
    return FALSE;
  }

  while (1) {
    RfKillEvent event;
    RfKillEvent *event_ptr;
    ssize_t len;

    len = read (fd, &event, sizeof(event));
    if (len < 0) {
      if (errno == EAGAIN)
        break;
      g_debug ("Reading of RFKILL events failed");
      break;
    }

    if (len < RFKILL_EVENT_SIZE_V1) {
      g_warning ("Wrong size of RFKILL event\n");
      continue;
    }

    if (event.op != RFKILL_OP_ADD)
      continue;

    g_debug ("Read killswitch of type '%s' (idx=%d): soft %d hard %d",
             type_to_string (event.type),
             event.idx, event.soft, event.hard);

    event_ptr = g_memdup2 (&event, sizeof(event));
    events = g_list_prepend (events, event_ptr);
  }

  /* Setup monitoring */
  self->channel = g_io_channel_unix_new (fd);
  g_io_channel_set_encoding (self->channel, NULL, NULL);
  g_io_channel_set_buffered (self->channel, FALSE);
  self->watch_id = g_io_add_watch (self->channel,
                                   G_IO_IN | G_IO_HUP | G_IO_ERR,
                                   (GIOFunc) rfkill_event_cb,
                                   self);

  if (events) {
    events = g_list_reverse (events);
    process_events (self, events);
  } else {
    g_debug ("No rfkill device available on startup");
  }

  return TRUE;
}


static void
phosh_hks_manager_constructed (GObject *object)
{
  PhoshHksManager *self = PHOSH_HKS_MANAGER (object);

  self->mic.killswitches = g_hash_table_new (g_direct_hash, g_direct_equal);
  self->mic.blocked_icon_name = "microphone-hardware-disabled-symbolic";
  self->mic.unblocked_icon_name = "microphone-sensitivity-high-symbolic";

  self->camera.killswitches = g_hash_table_new (g_direct_hash, g_direct_equal);
  self->camera.blocked_icon_name = "camera-hardware-disabled-symbolic";
  self->camera.unblocked_icon_name = "camera-photo-symbolic";

  setup_rfkill (self);

  G_OBJECT_CLASS (phosh_hks_manager_parent_class)->constructed (object);
}


static void
phosh_hks_manager_dispose (GObject *object)
{
  PhoshHksManager *self = PHOSH_HKS_MANAGER (object);

  if (self->watch_id > 0) {
    g_source_remove (self->watch_id);
    self->watch_id = 0;
    g_io_channel_shutdown (self->channel, FALSE, NULL);
    g_io_channel_unref (self->channel);
  }

  G_OBJECT_CLASS (phosh_hks_manager_parent_class)->dispose (object);
}


static void
phosh_hks_manager_finalize (GObject *object)
{
  PhoshHksManager *self = PHOSH_HKS_MANAGER (object);

  g_clear_pointer (&self->mic.killswitches, g_hash_table_destroy);
  g_clear_pointer (&self->camera.killswitches, g_hash_table_destroy);

  G_OBJECT_CLASS (phosh_hks_manager_parent_class)->finalize (object);
}


static void
phosh_hks_manager_class_init (PhoshHksManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_hks_manager_constructed;
  object_class->dispose = phosh_hks_manager_dispose;
  object_class->finalize = phosh_hks_manager_finalize;
  object_class->get_property = phosh_hks_manager_get_property;

  props[PROP_MIC_PRESENT] =
    g_param_spec_boolean ("mic-present",
                          "Mic present",
                          "HKS capable microphone present",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS);

  props[PROP_MIC_BLOCKED] =
    g_param_spec_boolean ("mic-blocked",
                          "Mic blocked",
                          "Microphone blocked via hks",
                          TRUE,
                          G_PARAM_READABLE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS);

  props[PROP_MIC_ICON_NAME] =
    g_param_spec_string ("mic-icon-name",
                         "Mic Icon Name",
                         "Icon for microphone hks",
                         "",
                         G_PARAM_READABLE |
                         G_PARAM_EXPLICIT_NOTIFY |
                         G_PARAM_STATIC_STRINGS);

  props[PROP_CAMERA_PRESENT] =
    g_param_spec_boolean ("camera-present",
                          "Camera present",
                          "HKS capable camera present",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS);

  props[PROP_CAMERA_BLOCKED] =
    g_param_spec_boolean ("camera-blocked",
                          "Camera blocked",
                          "Camera blocked via hks",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS);

  props[PROP_CAMERA_ICON_NAME] =
    g_param_spec_string ("camera-icon-name",
                         "Camera Icon Name",
                         "Icon for camera hks",
                         "",
                         G_PARAM_READABLE |
                         G_PARAM_EXPLICIT_NOTIFY |
                         G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_hks_manager_init (PhoshHksManager *self)
{
}


PhoshHksManager *
phosh_hks_manager_new (void)
{
  return PHOSH_HKS_MANAGER (g_object_new (PHOSH_TYPE_HKS_MANAGER, NULL));
}
