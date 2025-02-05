/*
 * Copyright (C) 2018 Purism SPC
 *               2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-wwan-mm"

#include "phosh-config.h"

#include "phosh-wwan-iface.h"
#include "phosh-wwan-mm.h"
#include "util.h"

#include <libmm-glib.h>
#include <ModemManager.h>

#define BUS_NAME "org.freedesktop.ModemManager1"

/**
 * PhoshWWanMM:
 *
 * Implementation of the [iface@Phosh.WWan] interface for ModemManager
 *
 * Since: 0.0.1
 */

enum {
  PROP_0,
  PROP_SIGNAL_QUALITY,
  PROP_ACCESS_TEC,
  PROP_UNLOCKED,
  PROP_SIM,
  PROP_PRESENT,
  PROP_ENABLED,
  PROP_OPERATOR,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

typedef struct _PhoshWWanMM {
  PhoshWWanManager                parent;

  MMObject                       *object;
  MMModem                        *modem;
  MMModem3gpp                    *modem_3gpp;
#ifdef PHOSH_HAVE_MM_CBM
  MMModemCellBroadcast           *cellbroadcast;
#endif

  MMManager                      *manager;
  GCancellable                   *cancel;

  guint                           signal_quality;
  const char                     *access_tec;
  gboolean                        unlocked;
  gboolean                        sim;
  gboolean                        present;
  gboolean                        enabled;
  char                           *operator;

  GListStore                     *cbms;
} PhoshWWanMM;


static void phosh_wwan_mm_interface_init (PhoshWWanInterface *iface);
G_DEFINE_TYPE_WITH_CODE (PhoshWWanMM, phosh_wwan_mm, PHOSH_TYPE_WWAN_MANAGER,
                         G_IMPLEMENT_INTERFACE (PHOSH_TYPE_WWAN,
                                                phosh_wwan_mm_interface_init))

static void
phosh_wwan_mm_update_signal_quality (PhoshWWanMM *self)
{
  g_return_if_fail (self);
  g_return_if_fail (self->modem);

  self->signal_quality = mm_modem_get_signal_quality (self->modem, NULL);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SIGNAL_QUALITY]);
}


static const char *
phosh_wwan_mm_user_friendly_access_tec (guint access_tec)
{
  if (access_tec & MM_MODEM_ACCESS_TECHNOLOGY_5GNR) {
    return "5G";
  } else if (access_tec & MM_MODEM_ACCESS_TECHNOLOGY_LTE) {
    return "4G";
  } else if (access_tec & MM_MODEM_ACCESS_TECHNOLOGY_HSPA_PLUS) {
    return "3.75G";
  } else if (access_tec & MM_MODEM_ACCESS_TECHNOLOGY_HSPA) {
    return "3.5G";
  } else if (access_tec & (MM_MODEM_ACCESS_TECHNOLOGY_UMTS |
                           MM_MODEM_ACCESS_TECHNOLOGY_HSDPA |
                           MM_MODEM_ACCESS_TECHNOLOGY_HSUPA)) {
    return "3G";
  } else if (access_tec & MM_MODEM_ACCESS_TECHNOLOGY_EDGE) {
    return "2.75G";
  } else if (access_tec & MM_MODEM_ACCESS_TECHNOLOGY_GPRS) {
    return "2.5G";
  } else if (access_tec & (MM_MODEM_ACCESS_TECHNOLOGY_GSM |
                           MM_MODEM_ACCESS_TECHNOLOGY_GSM_COMPACT)) {
    return "2G";
  }

  return NULL;
}


static void
phosh_wwan_mm_update_access_tec (PhoshWWanMM *self)
{
  guint access_tec;

  g_return_if_fail (self);
  g_return_if_fail (self->modem);

  access_tec = mm_modem_get_access_technologies (self->modem);
  self->access_tec = phosh_wwan_mm_user_friendly_access_tec (access_tec);
  g_debug ("Access tec is %s", self->access_tec);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ACCESS_TEC]);
}


static void
phosh_wwan_mm_update_operator (PhoshWWanMM *self)
{
  const char *operator;

  g_return_if_fail (self);
  g_return_if_fail (self->modem_3gpp);
  operator = mm_modem_3gpp_get_operator_name (self->modem_3gpp);

  if (g_strcmp0 (operator, self->operator)) {
    g_debug("Operator is '%s'", operator);
    g_free (self->operator);
    self->operator = g_strdup (operator);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_OPERATOR]);
  }
}


static void
phosh_wwan_mm_update_lock_status (PhoshWWanMM *self)
{
  guint unlock_required;
  int state;

  g_return_if_fail (self);
  g_return_if_fail (self->modem);
  /* Whether any kind of PIN is required */
  unlock_required = mm_modem_get_unlock_required (self->modem);
  /* Whether the sim card is currently locked */
  state = mm_modem_get_state (self->modem);
  self->unlocked = !!(unlock_required == MM_MODEM_LOCK_NONE ||
                      (state != MM_MODEM_STATE_LOCKED &&
                       state != MM_MODEM_STATE_FAILED));
  g_debug ("SIM is %slocked: (%d %d)", self->unlocked ? "un" : "", state, unlock_required);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_UNLOCKED]);
}


static void
phosh_wwan_mm_update_sim_status (PhoshWWanMM *self)
{
  const char *sim;

  g_return_if_fail (self);
  g_return_if_fail (self->modem);
  sim = mm_modem_get_sim_path (self->modem);
  g_debug ("SIM path %s", sim);
  self->sim = !!g_strcmp0 (sim, "/");
  g_debug ("SIM is %spresent", self->sim ? "" : "not ");
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SIM]);
}


static void
phosh_wwan_mm_update_present (PhoshWWanMM *self, gboolean present)
{
  g_return_if_fail (self);

  if (self->present != present) {
    g_debug ("Modem is %spresent", present ? "" : "not ");
    self->present = present;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PRESENT]);
  }
}


static void
phosh_wwan_mm_update_enabled (PhoshWWanMM *self)
{
  MMModemState state;
  gboolean enabled;

  g_return_if_fail (self);

  state = mm_modem_get_state (self->modem);

  enabled = (state > MM_MODEM_STATE_ENABLING) ? TRUE : FALSE;
  g_debug ("Modem is %senabled, state: %d", enabled ? "" : "not ", state);
  if (self->enabled != enabled) {
    self->enabled = enabled;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);
  }
}


static void
on_modem_props_changed (MmGdbusModem *proxy,
                        GVariant     *changed_properties,
                        GStrv         invaliated,
                        PhoshWWanMM  *self)
{
  char *property;
  GVariantIter i;

  g_variant_iter_init (&i, changed_properties);
  while (g_variant_iter_next (&i, "{&sv}", &property, NULL)) {
    g_debug ("WWAN property %s changed", property);
    if (g_strcmp0 (property, "AccessTechnologies") == 0) {
      phosh_wwan_mm_update_access_tec (self);
    } else if (g_strcmp0 (property, "SignalQuality") == 0) {
      phosh_wwan_mm_update_signal_quality (self);
    } else if (g_strcmp0 (property, "UnlockRequired") == 0) {
      phosh_wwan_mm_update_lock_status (self);
    } else if (g_strcmp0 (property, "State") == 0) {
      phosh_wwan_mm_update_lock_status (self);
      phosh_wwan_mm_update_enabled (self);
    } else if (g_strcmp0 (property, "Sim") == 0) {
      phosh_wwan_mm_update_sim_status (self);
    }
  }
}

static void
on_3gpp_props_changed (MmGdbusModem *proxy,
                       GVariant     *changed_properties,
                       GStrv         invaliated,
                       PhoshWWanMM  *self)
{
  char *property;
  GVariantIter i;

  g_variant_iter_init (&i, changed_properties);
  while (g_variant_iter_next (&i, "{&sv}", &property, NULL)) {
    g_debug ("WWAN 3gpp property %s changed", property);
    if (g_strcmp0 (property, "OperatorName") == 0) {
      phosh_wwan_mm_update_operator (self);
    }
  }
}


static void
phosh_wwan_mm_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  PhoshWWanMM *self = PHOSH_WWAN_MM (object);

  switch (property_id) {
  case PROP_SIGNAL_QUALITY:
    g_value_set_uint (value, self->signal_quality);
    break;
  case PROP_ACCESS_TEC:
    g_value_set_string (value, self->access_tec);
    break;
  case PROP_UNLOCKED:
    g_value_set_boolean (value, self->unlocked);
    break;
  case PROP_SIM:
    g_value_set_boolean (value, self->sim);
    break;
  case PROP_PRESENT:
    g_value_set_boolean (value, self->present);
    break;
  case PROP_ENABLED:
    g_value_set_boolean (value, self->enabled);
    break;
  case PROP_OPERATOR:
    g_value_set_string (value, self->operator);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_wwan_mm_destroy_modem (PhoshWWanMM *self)
{
#ifdef PHOSH_HAVE_MM_CBM
  if (self->cellbroadcast)
    g_signal_handlers_disconnect_by_data (self->cellbroadcast, self);
  g_clear_object (&self->cellbroadcast);
#endif
  if (self->modem_3gpp)
    g_signal_handlers_disconnect_by_data (self->modem_3gpp, self);
  g_clear_object (&self->modem_3gpp);

  if (self->modem)
    g_signal_handlers_disconnect_by_data (self->modem, self);
  g_clear_object (&self->modem);

  if (self->object)
    g_signal_handlers_disconnect_by_data (self->object, self);
  g_clear_object (&self->object);

  g_clear_object (&self->cbms);

  phosh_wwan_mm_update_present (self, FALSE);

  self->enabled = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);

  self->signal_quality = 0;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SIGNAL_QUALITY]);

  self->access_tec = NULL;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ACCESS_TEC]);

  self->unlocked = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_UNLOCKED]);

  self->sim = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SIM]);

  g_clear_pointer (&self->operator, g_free);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_OPERATOR]);
}


static void
modem_init_3gpp (PhoshWWanMM *self, MMObject *object)
{
  self->modem_3gpp = mm_object_get_modem_3gpp (object);
  g_return_if_fail (self->modem_3gpp);

  g_signal_connect (self->modem_3gpp,
                    "g-properties-changed",
                    G_CALLBACK (on_3gpp_props_changed),
                    self);
  phosh_wwan_mm_update_operator (self);
}


static void
modem_init_modem (PhoshWWanMM *self, MMObject *object)
{
  self->modem = mm_object_get_modem (object);
  g_return_if_fail (self->modem);

  g_signal_connect (self->modem,
                    "g-properties-changed",
                    G_CALLBACK (on_modem_props_changed),
                    self);
  phosh_wwan_mm_update_signal_quality (self);
  phosh_wwan_mm_update_access_tec (self);
  phosh_wwan_mm_update_lock_status (self);
  phosh_wwan_mm_update_sim_status (self);
  phosh_wwan_mm_update_present (self, TRUE);
  phosh_wwan_mm_update_enabled (self);
}

#ifdef PHOSH_HAVE_MM_CBM
static void
emit_new_cbm_received (PhoshWWanMM *self, MMCbm *cbm)
{
  g_signal_emit_by_name (self, "new-cbm", mm_cbm_get_text (cbm), mm_cbm_get_channel (cbm));
}


static void
on_cbm_state_updated (PhoshWWanMM *self, GParamSpec *pspec, MMCbm *cbm)
{
  guint pos;

  if (mm_cbm_get_state (cbm) != MM_CBM_STATE_RECEIVED)
    return;

  g_debug ("Received cbm %u: %s", mm_cbm_get_channel (cbm), mm_cbm_get_text (cbm));
  emit_new_cbm_received (self, cbm);

  /* Once notified we can drop the CBM from the store */
  g_signal_handlers_disconnect_by_data (cbm, self);
  g_return_if_fail (g_list_store_find (self->cbms, cbm, &pos));
  g_list_store_remove (self->cbms, pos);
}


static void
track_cbm (PhoshWWanMM *self, MMCbm *cbm)
{
  g_debug ("New cbm at %s", mm_cbm_get_path (cbm));
  g_list_store_insert (self->cbms, 0, cbm);
  g_signal_connect_object (cbm,
                           "notify::state",
                           G_CALLBACK (on_cbm_state_updated),
                           self,
                           G_CONNECT_SWAPPED);
  on_cbm_state_updated (self, NULL, cbm);
}


typedef struct {
  PhoshWWanMM *self;
  char        *cbm_path;
} PhoshWWanMMCbmListData;


static void
cbm_list_data_free (PhoshWWanMMCbmListData *data)
{
  g_free (data->cbm_path);
  g_free (data);
}


static void
on_cbms_listed (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  MMModemCellBroadcast *cell_broadcast = MM_MODEM_CELL_BROADCAST (source_object);
  PhoshWWanMMCbmListData *data = user_data;
  PhoshWWanMM *self = data->self;
  g_autolist (MMObject) cbm_list = NULL;
  g_autoptr (GError) err = NULL;
  MMCbm *cbm = NULL;

  cbm_list = mm_modem_cell_broadcast_list_finish (cell_broadcast, res, &err);
  if (!cbm_list) {
    phosh_async_error_warn (err, "Failed to fetch cell broadcast messages");
    return;
  }

  for (GList *l = cbm_list; l; l = l->next) {
    cbm = MM_CBM (l->data);

    if (data->cbm_path == NULL || g_strcmp0 (mm_cbm_get_path (cbm), data->cbm_path) == 0) {
      track_cbm (self, cbm);
      if (data->cbm_path)
        break;
    }
  }
  if (!cbm && data->cbm_path) {
    g_warning ("Failed to find CBM at %s", data->cbm_path);
  }

  cbm_list_data_free (data);
}


static void
on_cbm_added (PhoshWWanMM *self, const char *cbm_path, MMModemCellBroadcast *modem_cb)
{
  g_autolist (MMObject) cbm_list = NULL;
  PhoshWWanMMCbmListData *data = g_new0 (PhoshWWanMMCbmListData, 1);

  g_return_if_fail (PHOSH_IS_WWAN_MANAGER (self));

  *data = (PhoshWWanMMCbmListData){
    .self = self,
    .cbm_path = g_strdup (cbm_path),
  };

  mm_modem_cell_broadcast_list (self->cellbroadcast,
                                self->cancel,
                                on_cbms_listed,
                                data);
}


static void
modem_init_cellbroadcast (PhoshWWanMM *self, MMObject *object)
{
  PhoshWWanMMCbmListData *data = g_new0 (PhoshWWanMMCbmListData, 1);

  self->cellbroadcast = mm_object_get_modem_cell_broadcast (object);
  g_assert (self->cellbroadcast);

  self->cbms = g_list_store_new (MM_TYPE_CBM);

  g_debug ("Enabling cell broadcast interface");
  g_signal_connect_object (self->cellbroadcast,
                           "added",
                           G_CALLBACK (on_cbm_added),
                           self,
                           G_CONNECT_SWAPPED);

  /* Cold plug existing CBMs */
  *data = (PhoshWWanMMCbmListData){
    .self = self,
    .cbm_path = NULL,
  };
  mm_modem_cell_broadcast_list (self->cellbroadcast,
                                self->cancel,
                                on_cbms_listed,
                                data);
}
#endif


static void
on_mm_object_interface_added (PhoshWWanMM *self, GDBusInterface* interface, MMObject *object)
{
  g_return_if_fail (PHOSH_IS_WWAN_MM (self));

  if (self->object != object)
    return;

  if (MM_IS_MODEM_3GPP (interface)) {
    modem_init_3gpp (self, object);
    return;
#ifdef PHOSH_HAVE_MM_CBM
  } else if (MM_IS_MODEM_CELL_BROADCAST (interface)) {
    modem_init_cellbroadcast (self, object);
    return;
#endif
  }
}


static void
on_mm_object_added (PhoshWWanMM *self, GDBusObject *object, MMManager *manager)
{
  g_debug ("Modem added at path: %s", g_dbus_object_get_object_path (object));

  if (!self->object) {
    g_debug ("Tracking modem at: %s", g_dbus_object_get_object_path (object));

    self->object = g_object_ref (MM_OBJECT (object));
    /* Modem interface is always present */
    modem_init_modem (self, MM_OBJECT (object));

    g_signal_connect_swapped (object,
                              "interface-added",
                              G_CALLBACK (on_mm_object_interface_added),
                              self);

    /* Coldplug interfaces */
    if (mm_object_peek_modem_3gpp (MM_OBJECT (object)))
      modem_init_3gpp (self, MM_OBJECT (object));
#ifdef PHOSH_HAVE_MM_CBM
    if (mm_object_peek_modem_cell_broadcast (MM_OBJECT (object)))
      modem_init_cellbroadcast (self, MM_OBJECT (object));
#endif
  }
}


static void
on_mm_object_removed (PhoshWWanMM *self, GDBusObject *object, MMManager *manager)
{
  g_debug ("Modem removed at path: %s", g_dbus_object_get_object_path (object));

  if (self->object == MM_OBJECT (object)) {
    g_debug ("Dropping modem at: %s", g_dbus_object_get_object_path (object));
    phosh_wwan_mm_destroy_modem (self);
  }
}


static void
on_mm_manager_ready (GObject *source_object, GAsyncResult *res, PhoshWWanMM  *self)
{
  g_autoptr (GError) err = NULL;
  g_autolist (GDBusObject) modems = NULL;
  MMManager *manager;

  manager = mm_manager_new_finish (res, &err);
  if (!manager) {
    g_message ("Failed to connect modem manager: %s", err->message);
    return;
  }

  self->manager = manager;

  g_signal_connect_swapped (self->manager,
                            "object-added",
                            G_CALLBACK (on_mm_object_added),
                            self);

  g_signal_connect_swapped (self->manager,
                            "object-removed",
                            G_CALLBACK (on_mm_object_removed),
                            self);

  modems = g_dbus_object_manager_get_objects (G_DBUS_OBJECT_MANAGER (self->manager));
  if (modems) {
    /* Cold plug first modem */
    on_mm_object_added (self, modems->data, self->manager);
  } else {
    g_debug ("No modem found");
  }
}


static void
phosh_wwan_mm_dispose (GObject *object)
{
  PhoshWWanMM *self = PHOSH_WWAN_MM (object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  phosh_wwan_mm_destroy_modem (self);

  if (self->manager) {
    g_signal_handlers_disconnect_by_data (self->manager, self);
    g_clear_object (&self->manager);
  }

  G_OBJECT_CLASS (phosh_wwan_mm_parent_class)->dispose (object);
}


static void
phosh_wwan_mm_class_init (PhoshWWanMMClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = phosh_wwan_mm_dispose;
  object_class->get_property = phosh_wwan_mm_get_property;

  g_object_class_override_property (object_class, PROP_SIGNAL_QUALITY, "signal-quality");
  props[PROP_SIGNAL_QUALITY] = g_object_class_find_property (object_class, "signal-quality");

  g_object_class_override_property (object_class, PROP_ACCESS_TEC, "access-tec");
  props[PROP_ACCESS_TEC] = g_object_class_find_property (object_class, "access-tec");

  g_object_class_override_property (object_class, PROP_UNLOCKED, "unlocked");
  props[PROP_UNLOCKED] = g_object_class_find_property (object_class, "unlocked");

  g_object_class_override_property (object_class, PROP_SIM, "sim");
  props[PROP_SIM] = g_object_class_find_property (object_class, "sim");

  g_object_class_override_property (object_class, PROP_PRESENT, "present");
  props[PROP_PRESENT] = g_object_class_find_property (object_class, "present");

  g_object_class_override_property (object_class, PROP_ENABLED, "enabled");
  props[PROP_ENABLED] = g_object_class_find_property (object_class, "enabled");

  g_object_class_override_property (object_class, PROP_OPERATOR, "operator");
  props[PROP_OPERATOR] = g_object_class_find_property (object_class, "operator");
}


static guint
phosh_wwan_mm_get_signal_quality (PhoshWWan *phosh_wwan)
{
  PhoshWWanMM *self;

  g_return_val_if_fail (PHOSH_IS_WWAN_MM (phosh_wwan), 0);

  self = PHOSH_WWAN_MM (phosh_wwan);

  return self->signal_quality;
}


static const char *
phosh_wwan_mm_get_access_tec (PhoshWWan *phosh_wwan)
{
  PhoshWWanMM *self;

  g_return_val_if_fail (PHOSH_IS_WWAN_MM (phosh_wwan), NULL);

  self = PHOSH_WWAN_MM (phosh_wwan);

  return self->access_tec;
}


static gboolean
phosh_wwan_mm_is_unlocked (PhoshWWan *phosh_wwan)
{
  PhoshWWanMM *self;

  g_return_val_if_fail (PHOSH_IS_WWAN_MM (phosh_wwan), FALSE);

  self = PHOSH_WWAN_MM (phosh_wwan);

  return self->unlocked;
}


static gboolean
phosh_wwan_mm_has_sim (PhoshWWan *phosh_wwan)
{
  PhoshWWanMM *self;

  g_return_val_if_fail (PHOSH_IS_WWAN_MM (phosh_wwan), FALSE);

  self = PHOSH_WWAN_MM (phosh_wwan);

  return self->sim;
}


static gboolean
phosh_wwan_mm_is_present (PhoshWWan *phosh_wwan)
{
  PhoshWWanMM *self;

  g_return_val_if_fail (PHOSH_IS_WWAN_MM (phosh_wwan), FALSE);

  self = PHOSH_WWAN_MM (phosh_wwan);

  return self->present;
}


static gboolean
phosh_wwan_mm_is_enabled (PhoshWWan *phosh_wwan)
{
  PhoshWWanMM *self;

  g_return_val_if_fail (PHOSH_IS_WWAN_MM (phosh_wwan), FALSE);

  self = PHOSH_WWAN_MM (phosh_wwan);

  return self->enabled;
}


static const char *
phosh_wwan_mm_get_operator (PhoshWWan *phosh_wwan)
{
  PhoshWWanMM *self;

  g_return_val_if_fail (PHOSH_IS_WWAN_MM (phosh_wwan), NULL);

  self = PHOSH_WWAN_MM (phosh_wwan);

  return self->operator;
}


static void
phosh_wwan_mm_interface_init (PhoshWWanInterface *iface)
{
  iface->get_signal_quality = phosh_wwan_mm_get_signal_quality;
  iface->get_access_tec = phosh_wwan_mm_get_access_tec;
  iface->is_unlocked = phosh_wwan_mm_is_unlocked;
  iface->has_sim = phosh_wwan_mm_has_sim;
  iface->is_present = phosh_wwan_mm_is_present;
  iface->is_enabled = phosh_wwan_mm_is_enabled;
  iface->get_operator = phosh_wwan_mm_get_operator;
}


static void
on_bus_get_ready (GObject *source_object, GAsyncResult *res, PhoshWWanMM *self)
{
  g_autoptr (GError) err = NULL;
  GDBusConnection *connection;

  connection = g_bus_get_finish (res, &err);
  if (!connection) {
    phosh_async_error_warn (err, "Failed to attach to system bus");
    return;
  }

  mm_manager_new (connection,
                  G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_DO_NOT_AUTO_START,
                  self->cancel,
                  (GAsyncReadyCallback)on_mm_manager_ready,
                  self);
}


static void
phosh_wwan_mm_init (PhoshWWanMM *self)
{
  self->cancel = g_cancellable_new ();

  g_bus_get (G_BUS_TYPE_SYSTEM,
             self->cancel,
             (GAsyncReadyCallback)on_bus_get_ready,
             self);
}


PhoshWWanMM *
phosh_wwan_mm_new (void)
{
  return g_object_new (PHOSH_TYPE_WWAN_MM, NULL);
}
