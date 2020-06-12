/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-sensor-proxy-manager"

#include "config.h"
#include "sensor-proxy-manager.h"

#define IIO_SENSOR_PROXY_DBUS_NAME       "net.hadess.SensorProxy"
#define IIO_SENSOR_PROXY_DBUS_IFACE_NAME "net.hadess.SensorProxy"
#define IIO_SENSOR_PROXY_DBUS_OBJECT     "/net/hadess/SensorProxy"

/**
 * SECTION:sensor-proxy-manager
 * @short_description: Interface with iio-sensor-proxy
 * @Title: PhoshSensorProxyManager
 *
 * @descroption: The #PhoshSensorProxyManager is responsible for
 * getting events from iio-sensor-proxy.
 *
 * This is just a minimal wrapper so we don't have to provide the
 * object path, names and bus names in several places.
 */


typedef struct _PhoshSensorProxyManager
{
  PhoshDbusSensorProxyProxy parent;

} PhoshSensorProxyManager;


G_DEFINE_TYPE (PhoshSensorProxyManager, phosh_sensor_proxy_manager,
               PHOSH_DBUS_TYPE_SENSOR_PROXY_PROXY)


static void
phosh_sensor_proxy_manager_class_init (PhoshSensorProxyManagerClass *klass)
{
}


static void
phosh_sensor_proxy_manager_init (PhoshSensorProxyManager *self)
{
}


PhoshSensorProxyManager *
phosh_sensor_proxy_manager_get_default_failable (void)
{
  static PhoshSensorProxyManager *instance;
  GError *err = NULL;
  GInitable *ret;

  if (instance == NULL) {
    ret = g_initable_new (PHOSH_TYPE_SENSOR_PROXY_MANAGER, NULL, &err,
                          "g-flags", G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                          "g-name", IIO_SENSOR_PROXY_DBUS_NAME,
                          "g-bus-type", G_BUS_TYPE_SYSTEM,
                          "g-object-path", IIO_SENSOR_PROXY_DBUS_OBJECT,
                          "g-interface-name", IIO_SENSOR_PROXY_DBUS_IFACE_NAME,
                          NULL);
    if (ret != NULL) {
      instance = PHOSH_SENSOR_PROXY_MANAGER (ret);
    } else {
      g_warning ("Can't connect to iio-sensor-sensor proxy:  %s", err->message);
      return NULL;
    }
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
  }
  return instance;
}
