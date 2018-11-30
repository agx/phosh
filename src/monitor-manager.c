/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Somewhat based on mutter's src/backends/meta-monitor-manager.c
 */

#define G_LOG_DOMAIN "phosh-monitor-manager"

#include "monitor-manager.h"
#include "monitor/monitor.h"

#include "gamma-control-client-protocol.h"
#include "phosh-wayland.h"
#include "shell.h"

#include <gdk/gdkwayland.h>


static void phosh_monitor_manager_display_config_init (
  PhoshDisplayDbusDisplayConfigIface *iface);

typedef struct _PhoshMonitorManager
{
  PhoshDisplayDbusDisplayConfigSkeleton parent;

  GPtrArray *monitors;   /* Currently known monitors */

  int dbus_name_id;
  int serial;
} PhoshMonitorManager;

G_DEFINE_TYPE_WITH_CODE (PhoshMonitorManager,
                         phosh_monitor_manager,
                         PHOSH_DISPLAY_DBUS_TYPE_DISPLAY_CONFIG_SKELETON,
                         G_IMPLEMENT_INTERFACE (
                           PHOSH_DISPLAY_DBUS_TYPE_DISPLAY_CONFIG,
                           phosh_monitor_manager_display_config_init));


static char*
get_display_name (PhoshMonitor *monitor)
{
  if (phosh_monitor_is_builtin (monitor))
    return g_strdup (_("Built-in display"));
  else if (monitor->name)
    return g_strdup (monitor->name);
  else /* Translators: An unknown monitor type */
    return g_strdup (_("Unknown"));
}


static gboolean
phosh_monitor_manager_handle_get_resources (
  PhoshDisplayDbusDisplayConfig *skeleton,
  GDBusMethodInvocation *invocation)
{
  PhoshMonitorManager *self = PHOSH_MONITOR_MANAGER (skeleton);
  GVariantBuilder crtc_builder, output_builder, mode_builder;

  g_return_val_if_fail (self->monitors->len, FALSE);
  g_debug ("DBus %s", __func__);

  g_variant_builder_init (&crtc_builder, G_VARIANT_TYPE ("a(uxiiiiiuaua{sv})"));
  g_variant_builder_init (&output_builder, G_VARIANT_TYPE ("a(uxiausauaua{sv})"));
  g_variant_builder_init (&mode_builder, G_VARIANT_TYPE ("a(uxuudu)"));

  /* CRTCs */
  for (int i = 0; i < self->monitors->len; i++) {
    PhoshMonitor *monitor = g_ptr_array_index (self->monitors, i);
    GVariantBuilder transforms;

    if (!phosh_monitor_is_configured(monitor))
      continue;

    /* TODO: add transforms */
    g_variant_builder_init (&transforms, G_VARIANT_TYPE ("au"));
    g_variant_builder_add (&transforms, "u", 0);

    g_variant_builder_add (&crtc_builder, "(uxiiiiiuaua{sv})",
                           i,           /* ID */
                           i,           /* (gint64)crtc->crtc_id, */
                           monitor->x,
                           monitor->y,
                           monitor->width,
                           monitor->height,
                           0,           /* current_mode_index, */
                           0,           /* (guint32)crtc->transform, */
                           &transforms,
                           NULL         /* properties */);
  }

  /* outputs */
  for (int i = 0; i < self->monitors->len; i++) {
    PhoshMonitor *monitor = g_ptr_array_index (self->monitors, i);
    GVariantBuilder crtcs, modes, clones, properties;
    gboolean is_primary;

    if (!phosh_monitor_is_configured(monitor))
      continue;

    g_variant_builder_init (&crtcs, G_VARIANT_TYPE ("au"));
    g_variant_builder_add (&crtcs, "u", i /* possible_crtc_index */);
    g_variant_builder_init (&modes, G_VARIANT_TYPE ("au"));
    g_variant_builder_add (&modes, "u", 0 /* mode_index */);
    g_variant_builder_init (&clones, G_VARIANT_TYPE ("au"));
    g_variant_builder_add (&clones, "u", -1 /* possible_clone_index */);
    g_variant_builder_init (&properties, G_VARIANT_TYPE ("a{sv}"));
    g_variant_builder_add (&properties, "{sv}", "vendor",
                           g_variant_new_string (monitor->vendor));
    g_variant_builder_add (&properties, "{sv}", "product",
                           g_variant_new_string (monitor->product));
    g_variant_builder_add (&properties, "{sv}", "width-mm",
                           g_variant_new_int32 (monitor->width_mm));
    g_variant_builder_add (&properties, "{sv}", "height-mm",
                           g_variant_new_int32 (monitor->height_mm));
    is_primary = (monitor == phosh_shell_get_primary_monitor (phosh_shell_get_default()));
    g_variant_builder_add (&properties, "{sv}", "primary",
                           g_variant_new_boolean (is_primary));

    g_variant_builder_add (&output_builder, "(uxiausauaua{sv})",
                           i, /* ID */
                           i, /* (gint64)output->winsys_id, */
                           i, /* crtc_index, */
                           &crtcs,
                           monitor->name, /* output->name */
                           &modes,
                           &clones,
                           &properties);
  }

  /* modes */
  for (int i = 0; i < self->monitors->len; i++) {
    PhoshMonitor *monitor = g_ptr_array_index (self->monitors, i);
    GArray *modes = monitor->modes;

    if (!phosh_monitor_is_configured(monitor))
      continue;

    for (int k = 0; k < modes->len; k++) {
      PhoshMonitorMode *mode = &g_array_index (modes, PhoshMonitorMode, k);
      g_variant_builder_add (&mode_builder, "(uxuudu)",
                             k,            /* ID */
                             k,            /* (gint64)mode->mode_id, */
                             mode->width,  /* (guint32)mode->width, */
                             mode->height, /* (guint32)mode->height, */
                             (double)mode->refresh / 1000.0, /* (double)mode->refresh_rate, */
                             0             /* (guint32)mode->flags); */
      );
    }
  }

  phosh_display_dbus_display_config_complete_get_resources (
    skeleton,
    invocation,
    self->serial,
    g_variant_builder_end (&crtc_builder),
    g_variant_builder_end (&output_builder),
    g_variant_builder_end (&mode_builder),
    65535,  /* max_screen_width */
    65535   /* max_screen_height */
    );

  return TRUE;
}


static gboolean
phosh_monitor_manager_handle_change_backlight (
  PhoshDisplayDbusDisplayConfig *skeleton,
  GDBusMethodInvocation *invocation,
  guint                  serial,
  guint                  output_index,
  gint                   value)
{
  g_debug ("Unimplemented DBus call %s", __func__);
  g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                         G_DBUS_ERROR_NOT_SUPPORTED,
                                         "Changing backlight not supported");
  return TRUE;
}


struct get_wl_gamma_callback_data {
  PhoshDisplayDbusDisplayConfig *skeleton;
  GDBusMethodInvocation *invocation;
};


static void handle_wl_gamma_size(void *data, struct gamma_control *gamma_control,
                                 uint32_t size) {
  struct get_wl_gamma_callback_data *gamma_callback_data = data;
  GBytes *red_bytes, *green_bytes, *blue_bytes;
  GVariant *red_v, *green_v, *blue_v;
  /* All known clients using libgnome-desktop's
     gnome_rr_crtc_get_gamma only do so to get the size of the gamma
     table. So don't bother getting the real table since this is not
     supported by wlroots: https://github.com/swaywm/wlroots/pull/1059.
     Return an empty table instead.
  */
  size *= sizeof(unsigned short);
  red_bytes = g_bytes_new_take (g_malloc0 (size), size);
  green_bytes = g_bytes_new_take (g_malloc0 (size), size);
  blue_bytes = g_bytes_new_take (g_malloc0 (size), size);

  red_v = g_variant_new_from_bytes (G_VARIANT_TYPE ("aq"), red_bytes, TRUE);
  green_v = g_variant_new_from_bytes (G_VARIANT_TYPE ("aq"), green_bytes, TRUE);
  blue_v = g_variant_new_from_bytes (G_VARIANT_TYPE ("aq"), blue_bytes, TRUE);

  phosh_display_dbus_display_config_complete_get_crtc_gamma (
    gamma_callback_data->skeleton,
    gamma_callback_data->invocation,
    red_v, green_v, blue_v);

  g_bytes_unref (red_bytes);
  g_bytes_unref (green_bytes);
  g_bytes_unref (blue_bytes);

  g_free (gamma_callback_data);
  gamma_control_destroy (gamma_control);
}


static const struct
gamma_control_listener gamma_control_listener = {
	.gamma_size = handle_wl_gamma_size,
};


static gboolean
phosh_monitor_manager_handle_get_crtc_gamma (
  PhoshDisplayDbusDisplayConfig *skeleton,
  GDBusMethodInvocation *invocation,
  guint                  serial,
  guint                  crtc_id)
{
  PhoshMonitorManager *self = PHOSH_MONITOR_MANAGER (skeleton);
  PhoshMonitor *monitor;
  struct gamma_control *gamma_control;
  struct gamma_control_manager *gamma_control_manager;
  struct get_wl_gamma_callback_data *data;;

  g_debug ("DBus call %s for crtc %d, serial %d", __func__, crtc_id, serial);

  if (serial != self->serial) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_ACCESS_DENIED,
                                           "The requested configuration is based on stale information");
    return TRUE;
  }

  if (crtc_id >= self->monitors->len) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_INVALID_ARGS,
                                           "Invalid crtc id %d", crtc_id);
    return TRUE;
  }

  gamma_control_manager = phosh_wayland_get_gamma_control_manager (
    phosh_wayland_get_default ());
  if (gamma_control_manager == NULL) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_NOT_SUPPORTED,
                                           "gamma control not supported");
    return TRUE;
  }

  data = g_new0 (struct get_wl_gamma_callback_data, 1);
  data->skeleton = skeleton;
  data->invocation = invocation;

  monitor = g_ptr_array_index (self->monitors, crtc_id);
  gamma_control = gamma_control_manager_get_gamma_control (
    gamma_control_manager,
    monitor->wl_output);
  gamma_control_add_listener (gamma_control, &gamma_control_listener, data);

  return TRUE;
}


static gboolean
phosh_monitor_manager_handle_set_crtc_gamma (
  PhoshDisplayDbusDisplayConfig *skeleton,
  GDBusMethodInvocation *invocation,
  guint                  serial,
  guint                  crtc_id,
  GVariant              *red_v,
  GVariant              *green_v,
  GVariant              *blue_v)
{
  PhoshMonitorManager *self = PHOSH_MONITOR_MANAGER (skeleton);
  PhoshMonitor *monitor;
  unsigned short *red, *green, *blue;
  GBytes *red_bytes, *green_bytes, *blue_bytes;
  gsize size, dummy;
  struct gamma_control_manager *gamma_control_manager;
  struct gamma_control *gamma_control;
  struct wl_array wl_red, wl_green, wl_blue;

  g_debug ("DBus call %s for crtc %d, serial %d", __func__, crtc_id, serial);
  if (serial != self->serial) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_ACCESS_DENIED,
                                           "The requested configuration is based on stale information");
      return TRUE;
  }

  if (crtc_id >= self->monitors->len) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_INVALID_ARGS,
                                           "Invalid crtc id");
    return TRUE;
  }

  gamma_control_manager = phosh_wayland_get_gamma_control_manager (
    phosh_wayland_get_default ());
  if (!gamma_control_manager) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_NOT_SUPPORTED,
                                           "gamma control not supported");
    return TRUE;
  }

  monitor = g_ptr_array_index (self->monitors, crtc_id);

  red_bytes = g_variant_get_data_as_bytes (red_v);
  green_bytes = g_variant_get_data_as_bytes (green_v);
  blue_bytes = g_variant_get_data_as_bytes (blue_v);

  size = g_bytes_get_size (red_bytes);
  if (size != g_bytes_get_size (blue_bytes) || size != g_bytes_get_size (green_bytes)) {
        g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_NOT_SUPPORTED,
                                           "gamma for each color must have same size");
    goto err;
  }

  red = (unsigned short*) g_bytes_get_data (red_bytes, &dummy);
  green = (unsigned short*) g_bytes_get_data (green_bytes, &dummy);
  blue = (unsigned short*) g_bytes_get_data (blue_bytes, &dummy);

  wl_array_init (&wl_red);
  wl_array_init (&wl_green);
  wl_array_init (&wl_blue);

  wl_array_add (&wl_red, size);
  wl_array_add (&wl_green, size);
  wl_array_add (&wl_blue, size);

  memcpy(wl_red.data, red, size);
  memcpy(wl_green.data, green, size);
  memcpy(wl_blue.data, blue, size);

  gamma_control = gamma_control_manager_get_gamma_control (
    gamma_control_manager,
    monitor->wl_output);
  gamma_control_set_gamma(gamma_control, &wl_red, &wl_green, &wl_blue);
  gamma_control_destroy (gamma_control);

  phosh_display_dbus_display_config_complete_set_crtc_gamma (
      skeleton,
      invocation);

  wl_array_release (&wl_red);
  wl_array_release (&wl_green);
  wl_array_release (&wl_blue);

 err:
  g_bytes_unref (red_bytes);
  g_bytes_unref (green_bytes);
  g_bytes_unref (blue_bytes);
  return TRUE;
}

#define MODE_FORMAT "(siiddada{sv})"
#define MODES_FORMAT "a" MODE_FORMAT
#define MONITOR_SPEC_FORMAT "(ssss)"
#define MONITOR_FORMAT "(" MONITOR_SPEC_FORMAT MODES_FORMAT "a{sv})"
#define MONITORS_FORMAT "a" MONITOR_FORMAT

#define LOGICAL_MONITOR_MONITORS_FORMAT "a" MONITOR_SPEC_FORMAT
#define LOGICAL_MONITOR_FORMAT "(iidub" LOGICAL_MONITOR_MONITORS_FORMAT "a{sv})"
#define LOGICAL_MONITORS_FORMAT "a" LOGICAL_MONITOR_FORMAT

static gboolean
phosh_monitor_manager_handle_get_current_state (
  PhoshDisplayDbusDisplayConfig *skeleton,
  GDBusMethodInvocation *invocation)
{
  PhoshMonitorManager *self = PHOSH_MONITOR_MANAGER (skeleton);
  GVariantBuilder monitors_builder, logical_monitors_builder, properties_builder;

  g_debug ("DBus call %s", __func__);
  g_variant_builder_init (&monitors_builder,
                          G_VARIANT_TYPE (MONITORS_FORMAT));
  g_variant_builder_init (&logical_monitors_builder,
                          G_VARIANT_TYPE (LOGICAL_MONITORS_FORMAT));

  for (int i = 0; i < self->monitors->len; i++) {
    double scale = 1.0;
    PhoshMonitor *monitor = g_ptr_array_index (self->monitors, i);
    GVariantBuilder modes_builder, supported_scales_builder, mode_properties_builder,
      monitor_properties_builder;
    g_autofree gchar *serial = NULL;
    gchar *display_name;
    gboolean is_builtin;

    if (!phosh_monitor_is_configured(monitor))
      continue;

    g_variant_builder_init (&modes_builder, G_VARIANT_TYPE (MODES_FORMAT));

    for (int k = 0; k < monitor->modes->len; k++) {
      PhoshMonitorMode *mode = &g_array_index (monitor->modes, PhoshMonitorMode, k);
      g_autofree gchar *mode_name = NULL;

      g_variant_builder_init (&supported_scales_builder,
                              G_VARIANT_TYPE ("ad"));
      g_variant_builder_add (&supported_scales_builder, "d",
                             (double)monitor->scale);

      g_variant_builder_init (&mode_properties_builder,
                              G_VARIANT_TYPE ("a{sv}"));
      g_variant_builder_add (&mode_properties_builder, "{sv}",
                             "is-current",
                             g_variant_new_boolean (k == monitor->current_mode));
      g_variant_builder_add (&mode_properties_builder, "{sv}",
                             "is-preferred",
                             g_variant_new_boolean (k == monitor->preferred_mode));

      mode_name = g_strdup_printf ("%dx%d@%.0f", mode->width, mode->height,
                                   mode->refresh / 1000.0);

      g_variant_builder_add (&modes_builder, MODE_FORMAT,
                             mode_name, /* mode_id */
                             mode->width,
                             mode->height,
                             (double) mode->refresh / 1000.0,
                             scale,  /* (double) preferred_scale, */
                             &supported_scales_builder,
                             &mode_properties_builder);
    }

    g_variant_builder_init (&monitor_properties_builder,
                            G_VARIANT_TYPE ("a{sv}"));

    is_builtin = phosh_monitor_is_builtin (monitor);
    g_variant_builder_add (&monitor_properties_builder, "{sv}",
                           "is-builtin",
                           g_variant_new_boolean (is_builtin));

    display_name = get_display_name (monitor);
    g_variant_builder_add (&monitor_properties_builder, "{sv}",
                             "display-name",
                             g_variant_new_take_string (display_name));

    serial = g_strdup_printf ("00%d", i);
    g_variant_builder_add (&monitors_builder, MONITOR_FORMAT,
                           monitor->name,                       /* monitor_spec->connector */
                           monitor->vendor,                     /* monitor_spec->vendor, */
                           monitor->product,                    /* monitor_spec->product, */
                           serial,                              /* monitor_spec->serial, */
                           &modes_builder,
                           &monitor_properties_builder);
  }


  for (int i = 0; i < self->monitors->len; i++) {
    PhoshMonitor *monitor = g_ptr_array_index (self->monitors, i);
    GVariantBuilder logical_monitor_monitors_builder;
    g_autofree gchar *serial = NULL;
    gboolean is_primary;

    if (!phosh_monitor_is_configured(monitor))
      continue;

    serial = g_strdup_printf ("00%d", i);
    g_variant_builder_init (&logical_monitor_monitors_builder,
                            G_VARIANT_TYPE (LOGICAL_MONITOR_MONITORS_FORMAT));
    g_variant_builder_add (&logical_monitor_monitors_builder,
                           MONITOR_SPEC_FORMAT,
                           monitor->name,                       /* monitor_spec->connector, */
                           monitor->vendor,                     /* monitor_spec->vendor, */
                           monitor->product,                    /* monitor_spec->product, */
                           serial                               /* monitor_spec->serial, */
      );

    is_primary = (monitor == phosh_shell_get_primary_monitor (phosh_shell_get_default()));
    g_variant_builder_add (&logical_monitors_builder,
                           LOGICAL_MONITOR_FORMAT,
                           monitor->x,           /* logical_monitor->rect.x */
                           monitor->y,          /* logical_monitor->rect.y */
                           /* (double) logical_monitor->scale */
                           (double)monitor->scale,
                           0,                    /* logical_monitor->transform */
                           is_primary,           /* logical_monitor->is_primary */
                           &logical_monitor_monitors_builder,
                           NULL);
  }

  g_variant_builder_init (&properties_builder, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_add (&properties_builder, "{sv}",
                         "supports-mirroring",
                         g_variant_new_boolean (FALSE));

  g_variant_builder_add (&properties_builder, "{sv}",
                         "layout-mode",
                         g_variant_new_uint32 (0));

  phosh_display_dbus_display_config_complete_get_current_state (
    skeleton,
    invocation,
    self->serial,
    g_variant_builder_end (&monitors_builder),
    g_variant_builder_end (&logical_monitors_builder),
    g_variant_builder_end (&properties_builder));

  return TRUE;
}
#undef LOGICAL_MONITORS_FORMAT
#undef LOGICAL_MONITOR_FORMAT
#undef LOGICAL_MONITOR_MONITORS_FORMAT
#undef MODES_FORMAT
#undef MODE_FORMAT
#undef MONITORS_FORMAT
#undef MONITOR_FORMAT


#define MONITOR_CONFIG_FORMAT "(ssa{sv})"
#define MONITOR_CONFIGS_FORMAT "a" MONITOR_CONFIG_FORMAT


/* TODO: This can later become get_monitor_config_from_variant */
static PhoshMonitor *
find_monitor_from_variant(PhoshMonitorManager *self,
                                 GVariant     *monitor_config_variant,
                                 GError      **err)
{
  g_autofree char *connector = NULL;
  g_autofree char *mode_id = NULL;
  g_autoptr (GVariant) properties_variant = NULL;
  PhoshMonitor *monitor;

  g_variant_get (monitor_config_variant, "(ss@a{sv})",
                 &connector,
                 &mode_id,
                 &properties_variant);

  /* Look for the monitor associated with this connector
     TODO: We don't do any actual configuration yet.
   */
  monitor = phosh_monitor_manager_find_monitor (self, connector);
  if (monitor == NULL) {
    g_set_error (err, G_IO_ERROR, G_IO_ERROR_FAILED,
                 "Could not find monitor for connector '%s'", connector);
  }
  return monitor;
}

#define LOGICAL_MONITOR_CONFIG_FORMAT "(iidub" MONITOR_CONFIGS_FORMAT ")"

/* TODO: this can later become get-logical_monitor_config_from_variant */
static PhoshMonitor *
check_primary_monitor_from_variant (PhoshMonitorManager *self,
                                    GVariant            *logical_monitor_config_variant,
                                    GError             **err)
{
  int x, y;
  unsigned int transform;
  double scale;
  gboolean is_primary;
  GVariantIter *monitor_configs_iter;
  PhoshMonitor *monitor = NULL;

  g_variant_get (logical_monitor_config_variant, LOGICAL_MONITOR_CONFIG_FORMAT,
                 &x,
                 &y,
                 &scale,
                 &transform,
                 &is_primary,
                 &monitor_configs_iter);

  if (!is_primary)
    return NULL;

  while (TRUE) {
    GVariant *monitor_config_variant =
      g_variant_iter_next_value (monitor_configs_iter);

    if (!monitor_config_variant)
      break;

    monitor =
      find_monitor_from_variant (self,
                                 monitor_config_variant, err);
    g_variant_unref (monitor_config_variant);

    if (monitor == NULL)
      break;
  }
  g_variant_iter_free (monitor_configs_iter);
  return monitor;
}
#undef LOGICAL_MONITOR_CONFIG_FORMAT
#undef MONITOR_CONFIGS_FORMAT
#undef MONITOR_CONFIG_FORMAT

static gboolean
phosh_monitor_manager_handle_apply_monitors_config (
  PhoshDisplayDbusDisplayConfig *skeleton,
  GDBusMethodInvocation *invocation,
  guint                  serial,
  guint                  method,
  GVariant              *logical_monitor_configs_variant,
  GVariant              *properties_variant)
{
  PhoshMonitorManager *self = PHOSH_MONITOR_MANAGER (skeleton);
  GVariantIter logical_monitor_configs_iter;
  GError *err = NULL;
  PhoshMonitor *primary_monitor = NULL;
  PhoshShell *shell = phosh_shell_get_default();

  g_debug ("Mostly Stubbed DBus call %s", __func__);

  if (serial != self->serial) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_ACCESS_DENIED,
                                             "The requested configuration is based on stale information");
    return TRUE;
  }

  g_variant_iter_init (&logical_monitor_configs_iter,
                       logical_monitor_configs_variant);

  while (TRUE) {
    GVariant *logical_monitor_config_variant =
      g_variant_iter_next_value (&logical_monitor_configs_iter);

    if (!logical_monitor_config_variant)
      break;

    g_variant_unref (logical_monitor_config_variant);

    primary_monitor =
      check_primary_monitor_from_variant (self,
                                          logical_monitor_config_variant,
                                          &err);
    if (primary_monitor == NULL && err) {
      g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                             G_DBUS_ERROR_ACCESS_DENIED,
                                             "%s", err->message);
      return TRUE;
    }

    if (primary_monitor)
      break;
  }

  if (primary_monitor == NULL) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_ACCESS_DENIED,
                                           "No primary monitor found");
    return TRUE;
  }
  if (primary_monitor != phosh_shell_get_primary_monitor (shell)) {
    g_debug ("New primary monitor is %s", primary_monitor->name);
    phosh_shell_set_primary_monitor (shell, primary_monitor);
  }

  phosh_display_dbus_display_config_complete_apply_monitors_config (
    skeleton,
    invocation);

  return TRUE;
}


static void
phosh_monitor_manager_display_config_init (PhoshDisplayDbusDisplayConfigIface *iface)
{
  iface->handle_get_resources = phosh_monitor_manager_handle_get_resources;
  iface->handle_change_backlight = phosh_monitor_manager_handle_change_backlight;
  iface->handle_get_crtc_gamma = phosh_monitor_manager_handle_get_crtc_gamma;
  iface->handle_set_crtc_gamma = phosh_monitor_manager_handle_set_crtc_gamma;
  iface->handle_get_current_state = phosh_monitor_manager_handle_get_current_state;
  iface->handle_apply_monitors_config = phosh_monitor_manager_handle_apply_monitors_config;
}


static void
power_save_mode_changed_cb (PhoshMonitorManager *self,
                            GParamSpec          *pspec,
                            gpointer             user_data)
{
  gint mode;

  mode = phosh_display_dbus_display_config_get_power_save_mode (
    PHOSH_DISPLAY_DBUS_DISPLAY_CONFIG (self));
  g_debug ("Power save mode %d requested", mode);
}


static void
on_name_acquired (GDBusConnection *connection,
                  const char      *name,
                  gpointer         user_data)
{
  g_debug ("Acquired name %s", name);
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
  PhoshMonitorManager *self = user_data;

  /* We need to use Mutter's object path here to make gnome-settings happy */
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                    connection,
                                    "/org/gnome/Mutter/DisplayConfig",
                                    NULL);
}


static void
phosh_monitor_manager_finalize (GObject *object)
{
  PhoshMonitorManager *self = PHOSH_MONITOR_MANAGER (object);

  g_ptr_array_free (self->monitors, TRUE);

  G_OBJECT_CLASS (phosh_monitor_manager_parent_class)->finalize (object);
}


static void
phosh_monitor_manager_constructed (GObject *object)
{
  PhoshMonitorManager *self = PHOSH_MONITOR_MANAGER (object);

  G_OBJECT_CLASS (phosh_monitor_manager_parent_class)->constructed (object);
  self->dbus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                       "org.gnome.Mutter.DisplayConfig",
                                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                                       G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                       on_bus_acquired,
                                       on_name_acquired,
                                       on_name_lost,
                                       g_object_ref (self),
                                       g_object_unref);

  g_signal_connect (self, "notify::power-save-mode",
                    G_CALLBACK (power_save_mode_changed_cb), NULL);
}


static void
phosh_monitor_manager_class_init (PhoshMonitorManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_monitor_manager_constructed;
  object_class->finalize = phosh_monitor_manager_finalize;
}


static void
phosh_monitor_manager_init (PhoshMonitorManager *self)
{
  self->monitors = g_ptr_array_new_with_free_func ((GDestroyNotify) (g_object_unref));
  self->serial = 1;
}

PhoshMonitorManager *
phosh_monitor_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_MONITOR_MANAGER, NULL);
}


void
phosh_monitor_manager_add_monitor (PhoshMonitorManager *self, PhoshMonitor *monitor)
{
  g_ptr_array_add (self->monitors, monitor);
}


PhoshMonitor *
phosh_monitor_manager_get_monitor (PhoshMonitorManager *self, guint num)
{
  if (num >= self->monitors->len)
    return NULL;

  return g_ptr_array_index (self->monitors, num);
}


PhoshMonitor *
phosh_monitor_manager_find_monitor (PhoshMonitorManager *self, const gchar *name)
{
  for (int i = 0; i < self->monitors->len; i++) {
    PhoshMonitor *monitor = g_ptr_array_index (self->monitors, i);
    if (!g_strcmp0 (monitor->name, name))
        return monitor;
  }
  return NULL;
}


guint
phosh_monitor_manager_get_num_monitors (PhoshMonitorManager *self)
{
  return self->monitors->len;
}
