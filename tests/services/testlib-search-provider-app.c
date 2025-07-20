/*
 * Copyright (C) 2025 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Gotam Gorabh <gautamy672@gmail.com>
 */

#include <glib/gi18n.h>
#include <stdlib.h>

#include <gio/gio.h>

#include "testlib-search-provider.h"
#include "testlib-search-provider-app.h"

struct _PhoshSearchProviderMockApp
{
  GApplication parent;

  PhoshSearchProviderMock *search_provider;
};

G_DEFINE_TYPE (PhoshSearchProviderMockApp, phosh_search_provider_mock_app, G_TYPE_APPLICATION);

#define INACTIVITY_TIMEOUT 60 * 1000 /* One minute, in milliseconds */


static gboolean
phosh_search_provider_mock_app_dbus_register (GApplication    *application,
                                              GDBusConnection *connection,
                                              const gchar     *object_path,
                                              GError         **error)
{
  PhoshSearchProviderMockApp *self;

  g_assert_true (G_APPLICATION_CLASS (phosh_search_provider_mock_app_parent_class)->dbus_register (
                                                                                    application,
                                                                                    connection,
                                                                                    object_path,
                                                                                    error));

  self = PHOSH_SEARCH_PROVIDER_MOCK_APP (application);

  return phosh_search_provider_mock_dbus_register (self->search_provider,
                                                   connection,
                                                   object_path,
                                                   error);
}


static void
phosh_search_provider_mock_app_dbus_unregister (GApplication    *application,
                                                GDBusConnection *connection,
                                                const gchar     *object_path)
{
  PhoshSearchProviderMockApp *self = PHOSH_SEARCH_PROVIDER_MOCK_APP (application);

  if (self->search_provider)
    phosh_search_provider_mock_dbus_unregister (self->search_provider, connection, object_path);

  G_APPLICATION_CLASS (phosh_search_provider_mock_app_parent_class)->dbus_unregister (application,
                                                                                      connection,
                                                                                      object_path);
}


static void
phosh_search_provider_mock_app_dispose (GObject *object)
{
  PhoshSearchProviderMockApp *self = PHOSH_SEARCH_PROVIDER_MOCK_APP (object);

  g_clear_object (&self->search_provider);

  G_OBJECT_CLASS (phosh_search_provider_mock_app_parent_class)->dispose (object);
}


static void
phosh_search_provider_mock_app_init (PhoshSearchProviderMockApp *self)
{
  self->search_provider = phosh_search_provider_mock_new ();
  g_application_set_inactivity_timeout (G_APPLICATION (self),
                                        INACTIVITY_TIMEOUT);

  /* HACK: get the inactivity timeout started */
  g_application_hold (G_APPLICATION (self));
  g_application_release (G_APPLICATION (self));
}


static void
phosh_search_provider_mock_app_class_init (PhoshSearchProviderMockAppClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  object_class->dispose = phosh_search_provider_mock_app_dispose;

  app_class->dbus_register = phosh_search_provider_mock_app_dbus_register;
  app_class->dbus_unregister = phosh_search_provider_mock_app_dbus_unregister;
}


int main (int argc, char **argv)
{
  g_autoptr (GApplication) app = NULL;

  app = g_object_new (PHOSH_TYPE_SEARCH_PROVIDER_MOCK_APP,
                      "application-id", "org.gnome.Phosh.MockSearchProvider",
                      "flags", G_APPLICATION_IS_SERVICE,
                      NULL);
  return g_application_run (app, argc, argv);
}
