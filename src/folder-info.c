/*
 * Copyright (C) 2024 Tether Operations Limited
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Arun Mani J <arunmani@peartree.to>
 */

#define G_LOG_DOMAIN "phosh-folder-info"

#include "folder-info.h"
#include "util.h"

#include "favorite-list-model.h"
#include "gtk-list-models/gtkfilterlistmodel.h"

#include <gmobile.h>

#include <gio/gdesktopappinfo.h>

#define FOLDER_SCHEMA_ID "org.gnome.desktop.app-folders.folder"
#define FOLDERS_PREFIX "/org/gnome/desktop/app-folders/folders"

/**
 * PhoshFolderInfo:
 *
 * An object that represents a list of applications belonging to a folder.
 */

enum {
  PROP_0,
  PROP_PATH,
  PROP_NAME,
  PROP_APP_INFOS,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

enum {
  APPS_CHANGED,
  N_SIGNALS,
};
static guint signals[N_SIGNALS];

struct _PhoshFolderInfo {
  GObject                 parent;

  char                   *path;
  char                   *name;

  /* Contains all apps belonging to the folder */
  GListStore             *app_infos;
  /* Filters the above to show only required apps,
   * like non-favorite etc. */
  GtkFilterListModel     *filtered_app_infos;

  PhoshFavoriteListModel *favorites;
  GSettings              *settings;

  /* The current search term (only valid during refilter) */
  const char             *search;
};

static void folder_info_iface_init (GAppInfoIface *iface);

G_DEFINE_TYPE_WITH_CODE (PhoshFolderInfo, phosh_folder_info, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_APP_INFO, folder_info_iface_init))


static void
phosh_folder_info_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  PhoshFolderInfo *self = PHOSH_FOLDER_INFO (object);

  switch (property_id) {
  case PROP_PATH:
    self->path = g_value_dup_string (value);
    break;
  case PROP_NAME:
    phosh_folder_info_set_name (self, g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_folder_info_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PhoshFolderInfo *self = PHOSH_FOLDER_INFO (object);

  switch (property_id) {
  case PROP_PATH:
    g_value_set_string (value, self->path);
    break;
  case PROP_NAME:
    g_value_set_string (value, self->name);
    break;
  case PROP_APP_INFOS:
    g_value_set_object (value, self->filtered_app_infos);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static gboolean
equal (GAppInfo *app_info1, GAppInfo *app_info2)
{
  PhoshFolderInfo *self = PHOSH_FOLDER_INFO (app_info1);
  PhoshFolderInfo *other;
  gboolean is_equal;

  if (!PHOSH_IS_FOLDER_INFO (app_info2))
    return FALSE;

  other = PHOSH_FOLDER_INFO (app_info2);
  is_equal = g_str_equal (self->path, other->path);

  return is_equal;
}


static const char *
get_name (GAppInfo *app_info)
{
  PhoshFolderInfo *self = PHOSH_FOLDER_INFO (app_info);

  return self->name;
}


static gboolean
should_show (GAppInfo *app_info)
{
  PhoshFolderInfo *self = PHOSH_FOLDER_INFO (app_info);
  g_autoptr (GAppInfo) item = NULL;

  item = g_list_model_get_item (G_LIST_MODEL (self->filtered_app_infos), 0);
  return item != NULL;
}


static char *
load_from_file (char *filename, const char *data_dir)
{
  g_autofree char *path = g_build_path ("/", data_dir, "desktop-directories", filename, NULL);
  g_autoptr (GKeyFile) keyfile = g_key_file_new ();
  g_autoptr (GError) error = NULL;
  char *dirname;

  g_key_file_load_from_file (keyfile, path, G_KEY_FILE_NONE, &error);
  if (error != NULL) {
    g_debug ("Unable to load desktop directory %s: %s", path, error->message);
    return NULL;
  }

  dirname = g_key_file_get_locale_string (keyfile, "Desktop Entry", "Name", NULL, &error);
  if (!dirname) {
    g_debug ("Unable to load name from %s: %s", path, error->message);
    return NULL;
  }

  return dirname;
}


static void
on_settings_name_changed (PhoshFolderInfo *self, GSettings *settings, char *key)
{
  g_autofree char *name = g_settings_get_string (self->settings, "name");

  /* Load name from .directory file if available */
  if (g_str_has_suffix (name, ".directory")) {
    const char *const *data_dirs = g_get_system_data_dirs ();
    for (int i = 0; data_dirs[i] != NULL; i++) {
      g_autofree char *dirname = load_from_file (name, data_dirs[i]);
      if (dirname) {
        g_free (name);
        name = g_steal_pointer (&dirname);
        break;
      }
    }
  }

  if (g_strcmp0 (name, self->name) == 0)
    return;

  g_clear_pointer (&self->name, g_free);
  self->name = g_steal_pointer (&name);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_NAME]);
}


static void
load_apps (PhoshFolderInfo *self)
{
  g_auto (GStrv) apps;

  apps = g_settings_get_strv (self->settings, "apps");

  for (int i = 0; apps[i]; i++) {
    g_autoptr (GDesktopAppInfo) app_info = g_desktop_app_info_new (apps[i]);

    if (app_info == NULL)
      g_debug ("Unable to load app-info for %s", apps[i]);
    else if (!g_app_info_should_show (G_APP_INFO (app_info)))
      continue;
    else
      g_list_store_append (self->app_infos, app_info);
  }
}


static void
on_settings_apps_changed (PhoshFolderInfo *self, GSettings *settings, char *key)
{
  g_signal_emit (self, signals[APPS_CHANGED], 0);
  g_list_store_remove_all (self->app_infos);
  load_apps (self);
}


static gboolean
filter_app (gpointer item, gpointer data)
{
  GAppInfo *app_info = G_APP_INFO (item);
  PhoshFolderInfo *self = PHOSH_FOLDER_INFO (data);
  gboolean show;

  /*
   * If nothing is being searched, then show the app only if it is non-favorite.
   * Else check if the app matches the search term.
   */
  if (gm_str_is_null_or_empty (self->search))
    show = !phosh_favorite_list_model_app_is_favorite (self->favorites, app_info);
  else
    show = phosh_util_matches_app_info (app_info, self->search);

  return show;
}


static void
phosh_folder_info_dispose (GObject *object)
{
  PhoshFolderInfo *self = PHOSH_FOLDER_INFO (object);

  g_clear_pointer (&self->path, g_free);
  g_clear_pointer (&self->name, g_free);
  g_clear_object (&self->filtered_app_infos);
  g_clear_object (&self->app_infos);
  g_clear_object (&self->settings);

  G_OBJECT_CLASS (phosh_folder_info_parent_class)->dispose (object);
}


static void
phosh_folder_info_constructed (GObject *object)
{
  PhoshFolderInfo *self = PHOSH_FOLDER_INFO (object);
  g_autofree char *path = NULL;

  G_OBJECT_CLASS (phosh_folder_info_parent_class)->constructed (object);

  self->app_infos = g_list_store_new (G_TYPE_APP_INFO);
  self->favorites = phosh_favorite_list_model_get_default ();
  self->filtered_app_infos = gtk_filter_list_model_new (G_LIST_MODEL (self->app_infos),
                                                        filter_app,
                                                        self,
                                                        NULL);
  path = g_strconcat (FOLDERS_PREFIX, "/", self->path, "/", NULL);
  self->settings = g_settings_new_with_path (FOLDER_SCHEMA_ID, path);

  g_signal_connect_object (self->settings, "changed::name",
                           G_CALLBACK (on_settings_name_changed), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->settings, "changed::apps",
                           G_CALLBACK (on_settings_apps_changed), self, G_CONNECT_SWAPPED);

  on_settings_name_changed (self, NULL, NULL);
  load_apps (self);
  gtk_filter_list_model_refilter (self->filtered_app_infos);
}

static void
folder_info_iface_init (GAppInfoIface *iface)
{
  iface->equal = equal;
  iface->get_name = get_name;
  iface->should_show = should_show;
}


static void
phosh_folder_info_class_init (PhoshFolderInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = phosh_folder_info_dispose;
  object_class->constructed = phosh_folder_info_constructed;
  object_class->set_property = phosh_folder_info_set_property;
  object_class->get_property = phosh_folder_info_get_property;

  /**
   * PhoshFolderInfo:path:
   *
   * Relative GSettings path to folder
   */
  props[PROP_PATH] =
    g_param_spec_string ("path", "", "",
                         NULL,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  /**
   * PhoshFolderInfo:name:
   *
   * Name of the folder
   */
  props[PROP_NAME] =
    g_param_spec_string ("name", "", "",
                         NULL,
                         G_PARAM_READWRITE |
                         G_PARAM_EXPLICIT_NOTIFY |
                         G_PARAM_STATIC_STRINGS);

  /**
   * PhoshFolderInfo:app-infos:
   *
   * A list model of (filtered) app-infos belonging to the folder.
   */
  props[PROP_APP_INFOS] =
    g_param_spec_object ("app-infos", "", "",
                         G_TYPE_LIST_MODEL,
                         G_PARAM_READABLE |
                         G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  signals[APPS_CHANGED] = g_signal_new ("apps-changed",
                                        G_OBJECT_CLASS_TYPE (object_class),
                                        G_SIGNAL_RUN_LAST,
                                        0, NULL, NULL, NULL,
                                        G_TYPE_NONE, 0);
}


static void
phosh_folder_info_init (PhoshFolderInfo *self)
{
}


PhoshFolderInfo *
phosh_folder_info_new_from_folder_path (char *path)
{
  return g_object_new (PHOSH_TYPE_FOLDER_INFO, "path", path, NULL);
}


char *
phosh_folder_info_get_name (PhoshFolderInfo *self)
{
  g_return_val_if_fail (PHOSH_IS_FOLDER_INFO (self), 0);

  return self->name;
}

void
phosh_folder_info_set_name (PhoshFolderInfo *self, const char *name)
{
  g_return_if_fail (PHOSH_IS_FOLDER_INFO (self));

  g_settings_set_string (self->settings, "name", name);
}

/**
 * phosh_folder_info_get_app_infos:
 * @self: A folder info
 *
 * Get the list model of the folder info.
 *
 * Returns:(transfer none): The folder info
 */
GListModel *
phosh_folder_info_get_app_infos (PhoshFolderInfo *self)
{
  g_return_val_if_fail (PHOSH_IS_FOLDER_INFO (self), 0);

  return G_LIST_MODEL (self->filtered_app_infos);
}


gboolean
phosh_folder_info_contains (PhoshFolderInfo *self, GAppInfo *app_info)
{
  gboolean found = FALSE;
  g_return_val_if_fail (PHOSH_IS_FOLDER_INFO (self), FALSE);

  found = g_list_store_find_with_equal_func (self->app_infos, app_info,
                                             (GEqualFunc) g_app_info_equal, NULL);

  return found;
}


gboolean
phosh_folder_info_refilter (PhoshFolderInfo *self, const char *search)
{
  g_autoptr (GAppInfo) item = NULL;
  g_return_val_if_fail (PHOSH_IS_FOLDER_INFO (self), FALSE);

  self->search = search;
  gtk_filter_list_model_refilter (self->filtered_app_infos);
  self->search = NULL;

  item = g_list_model_get_item (G_LIST_MODEL (self->filtered_app_infos), 0);
  return item != NULL;
}


void
phosh_folder_info_add_app_info (PhoshFolderInfo *self, GAppInfo *app_info)
{
  const char *app_id;
  g_auto (GStrv) apps = NULL;
  g_auto (GStrv) new_apps = NULL;

  g_return_if_fail (PHOSH_IS_FOLDER_INFO (self));

  app_id = g_app_info_get_id (app_info);

  if (app_id == NULL) {
    g_debug ("Unable to get application ID");
    return;
  }

  apps = g_settings_get_strv (self->settings, "apps");
  new_apps = phosh_util_append_to_strv (apps, app_id);

  g_settings_set_strv (self->settings, "apps", (const char *const *) new_apps);
}


gboolean
phosh_folder_info_remove_app_info (PhoshFolderInfo *self, GAppInfo *app_info)
{
  const char *app_id;
  g_auto (GStrv) apps = NULL;
  g_auto (GStrv) new_apps = NULL;

  g_return_val_if_fail (PHOSH_IS_FOLDER_INFO (self), FALSE);

  app_id = g_app_info_get_id (app_info);

  if (app_id == NULL) {
    g_debug ("Unable to get application ID");
    return FALSE;
  }

  apps = g_settings_get_strv (self->settings, "apps");
  new_apps = phosh_util_remove_from_strv (apps, app_id);

  g_settings_set_strv (self->settings, "apps", (const char *const *) new_apps);
  return new_apps[0] != NULL;
}
