/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Somewhat based on mutter's src/backends/meta-monitor-manager.c
 */

#define G_LOG_DOMAIN "phosh-monitor-manager"

#include "monitor-manager.h"
#include "monitor/head.h"
#include "monitor/monitor.h"

#include "gamma-control-client-protocol.h"
#include "phosh-wayland.h"
#include "shell.h"

#include <gdk/gdkwayland.h>

/**
 * SECTION:monitor-manager
 * @short_description: The singleton that manages available monitors
 * @Title: PhoshMonitorManager
 *
 * #PhoshMonitorManager keeps tracks of and configure available monitors
 * and handles the "org.gnome.Mutter.DisplayConfig" DBus protocol.
 */

/* Equivalent to the 'layout-mode' enum in org.gnome.Mutter.DisplayConfig */
typedef enum PhoshMonitorMAnagerLayoutMode {
  PHOSH_MONITOR_MANAGER_LAYOUT_MODE_LOGICAL = 1,
  PHOSH_MONITOR_MANAGER_LAYOUT_MODE_PHYSICAL = 2
} PhoshMonitorManagerLayoutMode;

enum {
  PROP_0,
  PROP_N_MONITORS,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

enum {
  SIGNAL_MONITOR_ADDED,
  SIGNAL_MONITOR_REMOVED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

static void phosh_monitor_manager_display_config_init (
  PhoshDisplayDbusDisplayConfigIface *iface);

typedef struct _PhoshMonitorManager
{
  PhoshDisplayDbusDisplayConfigSkeleton parent;

  GPtrArray *monitors;   /* Currently known monitors */
  GPtrArray *heads;      /* Currently known heads */

  int dbus_name_id;
  int serial;

  uint32_t zwlr_output_serial;
} PhoshMonitorManager;

G_DEFINE_TYPE_WITH_CODE (PhoshMonitorManager,
                         phosh_monitor_manager,
                         PHOSH_DISPLAY_DBUS_TYPE_DISPLAY_CONFIG_SKELETON,
                         G_IMPLEMENT_INTERFACE (
                           PHOSH_DISPLAY_DBUS_TYPE_DISPLAY_CONFIG,
                           phosh_monitor_manager_display_config_init));


static const double known_diagonals[] = {
  12.1,
  13.3,
  15.6,
};


static char *
diagonal_to_str (double d)
{
  unsigned int i;

  for (i = 0; i < G_N_ELEMENTS (known_diagonals); i++) {
    double delta;

    delta = fabs(known_diagonals[i] - d);
    if (delta < 0.1)
      return g_strdup_printf ("%0.1lf\"", known_diagonals[i]);
  }

  return g_strdup_printf ("%d\"", (int) (d + 0.5));
}


static char*
get_display_name (PhoshHead *head)
{
  const char *vendor_name = NULL;
  const char *product_name = NULL;
  g_autofree char *inches = NULL;

  if (phosh_head_is_builtin (head))
    return g_strdup (_("Built-in display"));

  if (head->phys.width > 0 && head->phys.height > 0) {
    double d = sqrt (head->phys.width * head->phys.width +
                     head->phys.height * head->phys.height);
    inches = diagonal_to_str (d / 25.4);
  }

  if (head->product && g_strcmp0 (head->product, ""))
    product_name = head->product;

  if (head->vendor && g_strcmp0 (head->vendor, ""))
    vendor_name = head->vendor;

  if (vendor_name != NULL) {
    if (inches != NULL) {
      return g_strdup_printf (C_("This is a monitor vendor name, followed by a "
                                 "size in inches, like 'Dell 15\"'",
                                 "%s %s"),
                              vendor_name, inches);
    }
    if (product_name != NULL) {
      return g_strdup_printf (C_("This is a monitor vendor name followed by "
                                 "product/model name where size in inches "
                                 "could not be calculated, e.g. Dell U2414H",
                                 "%s %sn"),
                              vendor_name, product_name);
    }
  }

  if (head->name)
    return g_strdup (head->name);

  /* Translators: An unknown monitor type */
  return g_strdup (_("Unknown"));
}


static PhoshHead *
phosh_monitor_manager_get_head_from_monitor (PhoshMonitorManager *self, PhoshMonitor *monitor)
{
  for (int i = 0; i < self->heads->len; i++) {
    PhoshHead *head = g_ptr_array_index (self->heads, i);
    if (!g_strcmp0 (monitor->name, head->name)) {
      return head;
    }
  }

  return NULL;
}


static gint32
phosh_monitor_manager_flip_transform (PhoshMonitorTransform transform)
{
  /* Wayland rotation is opposite from DBus */
  switch (transform) {
  case PHOSH_MONITOR_TRANSFORM_90:
    return WL_OUTPUT_TRANSFORM_270;
    break;
  case PHOSH_MONITOR_TRANSFORM_270:
    return WL_OUTPUT_TRANSFORM_90;
    break;
  case PHOSH_MONITOR_TRANSFORM_FLIPPED_90:
    return WL_OUTPUT_TRANSFORM_FLIPPED_270;
    break;
  case PHOSH_MONITOR_TRANSFORM_FLIPPED_270:
    return WL_OUTPUT_TRANSFORM_FLIPPED_90;
    break;
  case PHOSH_MONITOR_TRANSFORM_NORMAL:
  case PHOSH_MONITOR_TRANSFORM_180:
  case PHOSH_MONITOR_TRANSFORM_FLIPPED:
  case PHOSH_MONITOR_TRANSFORM_FLIPPED_180:
  default:
    /* Nothing to be done */
    return transform;
  }
}

/*
 * DBus Interface
 */

static gboolean
phosh_monitor_manager_handle_get_resources (
  PhoshDisplayDbusDisplayConfig *skeleton,
  GDBusMethodInvocation *invocation)
{
  PhoshMonitorManager *self = PHOSH_MONITOR_MANAGER (skeleton);
  GVariantBuilder crtc_builder, output_builder, mode_builder;
  PhoshMonitor *primary_monitor;
  PhoshHead *primary_head;

  g_return_val_if_fail (self->monitors->len, FALSE);
  g_debug ("DBus %s", __func__);

  primary_monitor = phosh_shell_get_primary_monitor (phosh_shell_get_default());
  if (!primary_monitor) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_ACCESS_DENIED,
                                           "No primary monitor found");
    return TRUE;
  }
  primary_head = phosh_monitor_manager_get_head_from_monitor (self, primary_monitor);
  if (!primary_head) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_ACCESS_DENIED,
                                           "No primary monitor found");
    return TRUE;
  }

  g_variant_builder_init (&crtc_builder, G_VARIANT_TYPE ("a(uxiiiiiuaua{sv})"));
  g_variant_builder_init (&output_builder, G_VARIANT_TYPE ("a(uxiausauaua{sv})"));
  g_variant_builder_init (&mode_builder, G_VARIANT_TYPE ("a(uxuudu)"));

  /* CRTCs (logical monitor) */
  for (int i = 0; i < self->heads->len; i++) {
    PhoshHead *head = g_ptr_array_index (self->heads, i);
    GVariantBuilder transforms;

    if (!head->enabled) {
      g_debug ("Skipping disabled %s", head->name);
      continue;
    }

    g_variant_builder_init (&transforms, G_VARIANT_TYPE ("au"));
    for (int j = 0; j <= WL_OUTPUT_TRANSFORM_FLIPPED_270; j++)
      g_variant_builder_add (&transforms, "u", j);

    g_variant_builder_add (&crtc_builder, "(uxiiiiiuaua{sv})",
                           (guint32)i,           /* ID */
                           (guint64)i,           /* crtc->crtc_id, */
                           (gint32)head->x,
                           (gint32)head->y,
                           (gint32)head->mode->width,
                           (gint32)head->mode->height,
                           (gint32)0,           /* current_mode_index, */
                           (guint32)head->transform,
                           &transforms,
                           NULL                 /* properties */);
  }

  /* outputs (physical screen) */
  for (int i = 0; i < self->heads->len; i++) {
    PhoshHead *head = g_ptr_array_index (self->heads, i);
    GVariantBuilder crtcs, modes, clones, properties;
    gboolean is_primary;

    g_variant_builder_init (&crtcs, G_VARIANT_TYPE ("au"));
    g_variant_builder_add (&crtcs, "u", i /* possible_crtc_index */);
    g_variant_builder_init (&modes, G_VARIANT_TYPE ("au"));
    g_variant_builder_add (&modes, "u", 0 /* mode_index */);
    g_variant_builder_init (&clones, G_VARIANT_TYPE ("au"));
    g_variant_builder_add (&clones, "u", -1 /* possible_clone_index */);
    g_variant_builder_init (&properties, G_VARIANT_TYPE ("a{sv}"));
    g_variant_builder_add (&properties, "{sv}", "vendor",
                           g_variant_new_string (head->vendor ?: ""));
    g_variant_builder_add (&properties, "{sv}", "product",
                           g_variant_new_string (head->product ?: ""));
    g_variant_builder_add (&properties, "{sv}", "width-mm",
                           g_variant_new_int32 (head->phys.width));
    g_variant_builder_add (&properties, "{sv}", "height-mm",
                           g_variant_new_int32 (head->phys.height));
    is_primary = (head == primary_head);
    g_variant_builder_add (&properties, "{sv}", "primary",
                           g_variant_new_boolean (is_primary));

    g_variant_builder_add (&output_builder, "(uxiausauaua{sv})",
                           (guint32)i, /* ID */
                           (guint64)i, /* output->winsys_id, */
                           (int) i,    /* crtc_index, */
                           &crtcs,
                           head->name, /* output->name */
                           &modes,
                           &clones,
                           &properties);
  }

  /* Don't bother setting up modes, they're ignored */

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
  int                    value)
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
  PhoshMonitor *primary_monitor;
  PhoshHead *primary_head;

  g_debug ("DBus call %s", __func__);

  primary_monitor = phosh_shell_get_primary_monitor (phosh_shell_get_default());
  if (!primary_monitor) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_ACCESS_DENIED,
                                           "No primary monitor found");
    return TRUE;
  }
  primary_head = phosh_monitor_manager_get_head_from_monitor (self, primary_monitor);
  if (!primary_head) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_ACCESS_DENIED,
                                           "No primary monitor found");
    return TRUE;
  }

  g_variant_builder_init (&monitors_builder,
                          G_VARIANT_TYPE (MONITORS_FORMAT));
  g_variant_builder_init (&logical_monitors_builder,
                          G_VARIANT_TYPE (LOGICAL_MONITORS_FORMAT));

  /* connected physical monitors */
  for (int i = 0; i < self->heads->len; i++) {
    double scale = 1.0;
    PhoshHead *head = g_ptr_array_index (self->heads, i);
    GVariantBuilder modes_builder, supported_scales_builder, mode_properties_builder,
      monitor_properties_builder;
    char *display_name;
    gboolean is_builtin;
    g_autofree char *serial = NULL;

    g_variant_builder_init (&modes_builder, G_VARIANT_TYPE (MODES_FORMAT));

    for (int k = 0; k < head->modes->len; k++) {
      PhoshHeadMode *mode = g_ptr_array_index (head->modes, k);
      g_autofree char *mode_name = NULL;

      g_variant_builder_init (&supported_scales_builder,
                              G_VARIANT_TYPE ("ad"));
      g_variant_builder_add (&supported_scales_builder, "d",
                             (double)head->scale);

      g_variant_builder_init (&mode_properties_builder,
                              G_VARIANT_TYPE ("a{sv}"));
      g_variant_builder_add (&mode_properties_builder, "{sv}",
                             "is-current",
                             g_variant_new_boolean (head->mode == mode));
      g_variant_builder_add (&mode_properties_builder, "{sv}",
                             "is-preferred",
                             g_variant_new_boolean (mode->preferred));

      mode_name = g_strdup_printf ("%dx%d@%.0f", mode->width, mode->height,
                                   mode->refresh / 1000.0);

      g_variant_builder_add (&modes_builder, MODE_FORMAT,
                             mode_name, /* mode_id */
                             (gint32)mode->width,
                             (gint32)mode->height,
                             (double)mode->refresh / 1000.0,
                             (double)scale,  /* preferred_scale, */
                             &supported_scales_builder,
                             &mode_properties_builder);
    }

    g_variant_builder_init (&monitor_properties_builder,
                            G_VARIANT_TYPE ("a{sv}"));

    is_builtin = phosh_head_is_builtin (head);
    g_variant_builder_add (&monitor_properties_builder, "{sv}",
                           "is-builtin",
                           g_variant_new_boolean (is_builtin));

    display_name = get_display_name (head);
    g_variant_builder_add (&monitor_properties_builder, "{sv}",
                             "display-name",
                             g_variant_new_take_string (display_name));

    serial = g_strdup_printf ("00%d", i);
    g_variant_builder_add (&monitors_builder, MONITOR_FORMAT,
                           head->name,                       /* monitor_spec->connector */
                           head->vendor ?: "",               /* monitor_spec->vendor, */
                           head->product ?: "",              /* monitor_spec->product, */
                           head->serial ?: "",               /* monitor_spec->serial, */
                           &modes_builder,
                           &monitor_properties_builder);
  }

  /* Current logical monitor configuration */
  for (int i = 0; i < self->heads->len; i++) {
    PhoshHead *head = g_ptr_array_index (self->heads, i);
    GVariantBuilder logical_monitor_monitors_builder;
    g_autofree char *serial = NULL;
    gboolean is_primary;

    if (!head->enabled)
      continue;

    serial = g_strdup_printf ("00%d", i);
    g_variant_builder_init (&logical_monitor_monitors_builder,
                            G_VARIANT_TYPE (LOGICAL_MONITOR_MONITORS_FORMAT));
    g_variant_builder_add (&logical_monitor_monitors_builder,
                           MONITOR_SPEC_FORMAT,
                           head->name,                       /* monitor_spec->connector, */
                           head->vendor ?: "",               /* monitor_spec->vendor, */
                           head->product ?: "",              /* monitor_spec->product, */
                           head->serial ?:""                 /* monitor_spec->serial, */
      );

    is_primary = (head == primary_head);
    g_variant_builder_add (&logical_monitors_builder,
                           LOGICAL_MONITOR_FORMAT,
                           (gint32)head->x,             /* logical_monitor->rect.x */
                           (gint32)head->y,             /* logical_monitor->rect.y */
                           (double)head->scale,         /* (double) logical_monitor->scale */
                           (guint32)head->transform,    /* logical_monitor->transform */
                           is_primary,             /* logical_monitor->is_primary */
                           &logical_monitor_monitors_builder,
                           NULL);
  }

  g_variant_builder_init (&properties_builder, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_add (&properties_builder, "{sv}",
                         "supports-mirroring",
                         g_variant_new_boolean (FALSE));

  g_variant_builder_add (&properties_builder, "{sv}",
                         "layout-mode",
                         g_variant_new_uint32 (PHOSH_MONITOR_MANAGER_LAYOUT_MODE_LOGICAL));

  g_variant_builder_add (&properties_builder, "{sv}",
                         "supports-changing-layout-mode",
                         g_variant_new_boolean (TRUE));

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

static PhoshHead *
phosh_monitor_manager_find_head (PhoshMonitorManager *self, const char *name)
{
  for (int i = 0; i < self->heads->len; i++) {
    PhoshHead *head = g_ptr_array_index (self->heads, i);
    if (!g_strcmp0 (head->name, name))
        return head;
  }
  return NULL;
}

static PhoshHead *
find_head_from_variant (PhoshMonitorManager *self,
                        GVariant            *monitor_config_variant,
                        PhoshMonitor       **monitor,
                        gchar              **mode,
                        GError             **err)
{
  g_autofree char *connector = NULL;
  char *mode_id = NULL;
  g_autoptr (GVariant) properties_variant = NULL;
  GVariantIter iter;
  GVariant *value;
  PhoshHead *head;
  gchar *key;

  g_return_val_if_fail (*mode == NULL || *monitor == NULL, NULL);

  g_variant_get (monitor_config_variant, "(ss@a{sv})",
                 &connector,
                 &mode_id,
                 &properties_variant);

  /* Look for the monitor associated with this connector */
  *monitor = phosh_monitor_manager_find_monitor (self, connector);
  head = phosh_monitor_manager_find_head (self, connector);
  if (head == NULL) {
    g_set_error (err, G_IO_ERROR, G_IO_ERROR_FAILED,
                 "Could not find monitor for connector '%s'", connector);
    return NULL;
  }
  g_debug ("%s should have mode id %s", head->name, mode_id);

  g_variant_iter_init (&iter, properties_variant);
  while (g_variant_iter_loop (&iter, "{sv}", &key, &value)) {
    /* We don't bother about any properties here */
    g_debug ("Monitor '%s':  prop '%s'", connector, key);
  }

  *mode = mode_id;
  return head;
}


#define LOGICAL_MONITOR_CONFIG_FORMAT "(iidub" MONITOR_CONFIGS_FORMAT ")"


static PhoshMonitor *
config_head_config_from_logical_monitor_variant (PhoshMonitorManager *self,
                                                 GVariant            *logical_monitor_config_variant,
                                                 GError             **err)
{
  int x, y;
  unsigned int transform;
  double scale;
  gboolean is_primary;
  g_autoptr (GVariantIter) monitor_configs_iter = NULL;
  PhoshMonitor *monitor = NULL;

  g_variant_get (logical_monitor_config_variant, LOGICAL_MONITOR_CONFIG_FORMAT,
                 &x,
                 &y,
                 &scale,
                 &transform,
                 &is_primary,
                 &monitor_configs_iter);

  while (TRUE) {
    PhoshHead *head;
    g_autofree char *mode = NULL;

    g_autoptr (GVariant) monitor_config_variant =
      g_variant_iter_next_value (monitor_configs_iter);

    if (!monitor_config_variant)
      break;

    monitor = NULL;
    head = find_head_from_variant (self,
                                   monitor_config_variant,
                                   &monitor,
                                   &mode,
                                   err);
    if (head == NULL)
      break;

    g_debug ("Head %s in logical monitor config %p",
             head->name,
             logical_monitor_config_variant);
    /* TODO: find new head mode from mode name */
    head->pending.scale = scale;
    head->pending.x = x;
    head->pending.y = y;
    head->pending.transform = phosh_monitor_manager_flip_transform (transform);
    head->pending.enabled = TRUE;
    head->pending.seen = TRUE;
  }

  return is_primary ? monitor : NULL;
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
  GVariant *layout_mode_variant = NULL;
  PhoshMonitorManagerLayoutMode layout_mode = PHOSH_MONITOR_MANAGER_LAYOUT_MODE_LOGICAL;

  g_debug ("DBus call %s, method: %d", __func__, method);

  if (serial != self->serial) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_ACCESS_DENIED,
                                             "The requested configuration is based on stale information");
    return TRUE;
  }

  if (phosh_shell_get_locked (phosh_shell_get_default ())) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_ACCESS_DENIED,
                                             "Can't configure outputs when locked");
    return TRUE;
  }

  if (properties_variant) {
    layout_mode_variant = g_variant_lookup_value (properties_variant,
                                                  "layout-mode",
                                                  G_VARIANT_TYPE ("u"));
    if (layout_mode_variant) {
      g_variant_get (layout_mode_variant, "u", &layout_mode);
      g_debug ("Requested layout mode %d", layout_mode);
    }
  }

  if (layout_mode != PHOSH_MONITOR_MANAGER_LAYOUT_MODE_LOGICAL) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_ACCESS_DENIED,
                                           "Invalid layout mode specified");
    return TRUE;
  }


  /* Make sure we refresh only heads from this config run */
  for (int i = 0; i < self->heads->len; i++) {
    PhoshHead *head = g_ptr_array_index (self->heads, i);
    head->pending.seen = FALSE;
  }

  g_variant_iter_init (&logical_monitor_configs_iter,
                       logical_monitor_configs_variant);

  while (TRUE) {
    PhoshMonitor *mon;

    g_autoptr (GVariant) logical_monitor_config_variant =
      g_variant_iter_next_value (&logical_monitor_configs_iter);

    if (!logical_monitor_config_variant)
      break;

    mon = config_head_config_from_logical_monitor_variant (self,
                                                           logical_monitor_config_variant,
                                                           &err);

    if (mon == NULL && err) {
      g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                             G_DBUS_ERROR_ACCESS_DENIED,
                                             "%s", err->message);
      return TRUE;
    }

    if (mon) {
      if (primary_monitor) {
        g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                               G_DBUS_ERROR_ACCESS_DENIED,
                                               "Only one primary monitor possible");
        return TRUE;
      } else {
        primary_monitor = mon;
      }
    }
  }

  if (primary_monitor == NULL) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_ACCESS_DENIED,
                                           "No primary monitor found");
    return TRUE;
  }

  if (method == PHOSH_MONITOR_MANAGER_CONFIG_METHOD_PERSISTENT) {

    /* TODO: This only works when both monitors are enabled. So
       we need to first enable the head, wait for monitor to be
       configured then move the primary output */
    if (primary_monitor != phosh_shell_get_primary_monitor (shell)) {
      g_debug ("New primary monitor is %s", primary_monitor->name);
      phosh_shell_set_primary_monitor (shell, primary_monitor);
    }

    for (int i = 0; i < self->heads->len; i++) {
      PhoshHead *head = g_ptr_array_index (self->heads, i);

      /* If head was not seen during this invocation, disable it */
      if (!head->pending.seen)
        head->pending.enabled = FALSE;

      head->pending.seen = FALSE;
    }

    /* Apply new config */
    phosh_monitor_manager_apply_monitor_config (self);
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
  int mode, ps_mode;

  mode = phosh_display_dbus_display_config_get_power_save_mode (
    PHOSH_DISPLAY_DBUS_DISPLAY_CONFIG (self));
  g_debug ("Power save mode %d requested", mode);

  switch (mode) {
  case 0:
    ps_mode = PHOSH_MONITOR_POWER_SAVE_MODE_ON;
    break;
  default:
    ps_mode = PHOSH_MONITOR_POWER_SAVE_MODE_OFF;
    break;
  }

  for (int i = 0; i < self->monitors->len; i++) {
    PhoshMonitor *monitor = g_ptr_array_index (self->monitors, i);

    phosh_monitor_set_power_save_mode (monitor, ps_mode);
  }
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

/*
 * wl_output wayland protocol
 */

static PhoshMonitor *
find_monitor_by_wl_output (PhoshMonitorManager *self, struct wl_output *output)
{
  for (int i = 0; i < self->monitors->len; i++) {
    PhoshMonitor *monitor = g_ptr_array_index (self->monitors, i);
    if (monitor->wl_output == output)
        return monitor;
  }
  return NULL;
}


static void
on_monitor_removed (PhoshMonitorManager *self,
                    PhoshMonitor        *monitor,
                    gpointer            *data)
{
  g_return_if_fail (PHOSH_IS_MONITOR (monitor));
  g_return_if_fail (PHOSH_IS_MONITOR_MANAGER (self));

  g_debug("Monitor %p (%s) removed", monitor, monitor->name);
  g_ptr_array_remove (self->monitors, monitor);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_N_MONITORS]);
}


static void
on_wl_outputs_changed (PhoshMonitorManager *self, GParamSpec *pspec, PhoshWayland *wl)
{
  GHashTable *wl_outputs = phosh_wayland_get_wl_outputs (wl);
  GHashTableIter iter;
  struct wl_output *wl_output;
  PhoshMonitor *monitor;

  /* Check for gone outputs */
  for (int i = 0; i < self->monitors->len; i++) {
    monitor = g_ptr_array_index (self->monitors, i);
    if (!phosh_wayland_has_wl_output (wl, monitor->wl_output)) {
      g_debug ("Monitor %p (%s) gone", monitor, monitor->name);
      g_signal_emit (self, signals[SIGNAL_MONITOR_REMOVED], 0, monitor);
      /* The monitor is removed from monitors in the class'es default
       * signal handler */
      return;
    }
  }

  /* Check for new outputs */
  g_hash_table_iter_init (&iter, wl_outputs);
  while (g_hash_table_iter_next (&iter, NULL, (gpointer)&wl_output)) {
    if (!find_monitor_by_wl_output (self, wl_output)) {
      monitor = phosh_monitor_new_from_wl_output (wl_output);
      phosh_monitor_manager_add_monitor (self, monitor);
      g_debug ("Monitor %p added", monitor);
      return;
    }
  }
}


/*
 * wlr_output_manager wayland protocol
 */

static void
on_head_finished (PhoshMonitorManager *self,
                  PhoshHead *head)
{
  g_return_if_fail (PHOSH_IS_MONITOR_MANAGER (self));

  if (g_ptr_array_remove (self->heads, head))
    g_debug ("Removing head %p", head);
  else
    g_warning ("Tried to remove inexistend head %p", head);

  phosh_display_dbus_display_config_emit_monitors_changed (PHOSH_DISPLAY_DBUS_DISPLAY_CONFIG (self));
}


static void
zwlr_output_manager_v1_handle_head (void *data,
                                    struct zwlr_output_manager_v1 *manager,
                                    struct zwlr_output_head_v1 *wlr_head)
{
  PhoshMonitorManager *self = data;
  PhoshHead *head;

  g_return_if_fail (PHOSH_IS_MONITOR_MANAGER (self));

  head = phosh_head_new_from_wlr_head (wlr_head);
  g_debug ("New head %p", head);
  g_ptr_array_add (self->heads, head);
  g_signal_connect_swapped (head, "head-finished", G_CALLBACK (on_head_finished), self);

  phosh_display_dbus_display_config_emit_monitors_changed (PHOSH_DISPLAY_DBUS_DISPLAY_CONFIG (self));
}


static void
zwlr_output_manager_v1_handle_done (void *data,
                                    struct zwlr_output_manager_v1 *manager,
                                    uint32_t serial)
{
  PhoshMonitorManager *self = data;

  g_return_if_fail (PHOSH_IS_MONITOR_MANAGER (self));
  g_debug ("Got zwlr_output_serial %u", serial);
  self->zwlr_output_serial = serial;
  self->serial++;

  phosh_display_dbus_display_config_emit_monitors_changed (PHOSH_DISPLAY_DBUS_DISPLAY_CONFIG (self));
}


static const struct zwlr_output_manager_v1_listener zwlr_output_manager_v1_listener = {
  .head = zwlr_output_manager_v1_handle_head,
  .done = zwlr_output_manager_v1_handle_done,
};


static void
zwlr_output_configuration_v1_handle_succeeded (void *data,
                                               struct zwlr_output_configuration_v1 *config)
{
  g_debug ("New output configuration %p applied", config);
  zwlr_output_configuration_v1_destroy (config);
}


static void
zwlr_output_configuration_v1_handle_failed (void *data,
                                            struct zwlr_output_configuration_v1 *config)
{
  /* TODO: bubble up error */
  g_warning ("Failed to apply New output %p configuration", config);
  zwlr_output_configuration_v1_destroy (config);
}


static void
zwlr_output_configuration_v1_handle_cancelled (void *data,
                                               struct zwlr_output_configuration_v1 *config)
{
  zwlr_output_configuration_v1_destroy(config);
  g_warning("Failed to apply New output configuration %p due to changes", config);
}


static const struct zwlr_output_configuration_v1_listener config_listener = {
  .succeeded = zwlr_output_configuration_v1_handle_succeeded,
  .failed = zwlr_output_configuration_v1_handle_failed,
  .cancelled = zwlr_output_configuration_v1_handle_cancelled,
};


static void
phosh_monitor_manager_finalize (GObject *object)
{
  PhoshMonitorManager *self = PHOSH_MONITOR_MANAGER (object);

  g_ptr_array_free (self->monitors, TRUE);
  g_ptr_array_free (self->heads, TRUE);

  G_OBJECT_CLASS (phosh_monitor_manager_parent_class)->finalize (object);
}

/*
 * PhoshMonitorManager Class
 */

static void
phosh_monitor_manager_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  PhoshMonitorManager *self = PHOSH_MONITOR_MANAGER (object);

  switch (property_id) {
  case PROP_N_MONITORS:
    g_value_set_int (value, self->monitors->len);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static gboolean
on_idle (PhoshMonitorManager *self)
{
  self->dbus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                       "org.gnome.Mutter.DisplayConfig",
                                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                                       G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                       on_bus_acquired,
                                       on_name_acquired,
                                       on_name_lost,
                                       g_object_ref (self),
                                       g_object_unref);
  return FALSE;
}


static void
phosh_monitor_manager_constructed (GObject *object)
{
  PhoshMonitorManager *self = PHOSH_MONITOR_MANAGER (object);
  PhoshWayland *wl = phosh_wayland_get_default();
  GHashTableIter iter;
  struct wl_output *wl_output;
  struct zwlr_output_manager_v1 *zwlr_output_manager_v1;

  G_OBJECT_CLASS (phosh_monitor_manager_parent_class)->constructed (object);

  g_signal_connect (self, "notify::power-save-mode",
                    G_CALLBACK (power_save_mode_changed_cb), NULL);

  g_signal_connect_swapped (phosh_wayland_get_default(),
                            "notify::wl-outputs",
                            G_CALLBACK (on_wl_outputs_changed),
                            self);
  /* Get initial output list */
  g_hash_table_iter_init (&iter, phosh_wayland_get_wl_outputs (wl));
  while (g_hash_table_iter_next (&iter, NULL, (gpointer)&wl_output)) {
    PhoshMonitor *monitor = phosh_monitor_new_from_wl_output (wl_output);
    phosh_monitor_manager_add_monitor (self, monitor);
  }

  zwlr_output_manager_v1 = phosh_wayland_get_zwlr_output_manager_v1 (wl);
  zwlr_output_manager_v1_add_listener (zwlr_output_manager_v1,
                                       &zwlr_output_manager_v1_listener,
                                       self);

  g_idle_add ((GSourceFunc) on_idle, self);
}


static void
phosh_monitor_manager_class_init (PhoshMonitorManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_monitor_manager_constructed;
  object_class->finalize = phosh_monitor_manager_finalize;
  object_class->get_property = phosh_monitor_manager_get_property;

  props[PROP_N_MONITORS] =
    g_param_spec_int ("n-monitors",
                      "Number of monitors",
                      "The number of enabled monitors",
                      0,
                      G_MAXINT,
                      0,
                      G_PARAM_READABLE |
                      G_PARAM_EXPLICIT_NOTIFY |
                      G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  /**
   * PhoshMonitorManager::monitor-added:
   * @manager: The #PhoshMonitorManager emitting the signal.
   * @monitor: The #PhoshMonitor being added.
   *
   * Emitted whenever a monitor is about to be added. Note
   * that the monitor might not yet be fully initialized. Use
   * phosh_monitor_is_configured() to check or listen for
   * the #PhoshMonitor::configured signal.
   */
  signals[SIGNAL_MONITOR_ADDED] = g_signal_new (
    "monitor-added",
    G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
    NULL, G_TYPE_NONE, 1, PHOSH_TYPE_MONITOR);

  /**
   * PhoshMonitorManager::monitor-removed:
   * @manager: The #PhoshMonitorManager emitting the signal.
   * @monitor: The #PhoshMonitor being removed.
   *
   * Emitted whenever a monitor is about to be removed.
   */
  signals[SIGNAL_MONITOR_REMOVED] = g_signal_new_class_handler (
    "monitor-removed",
    G_TYPE_FROM_CLASS (klass),
    G_SIGNAL_RUN_CLEANUP,
    G_CALLBACK (on_monitor_removed),
    NULL, NULL,  NULL, G_TYPE_NONE, 1, PHOSH_TYPE_MONITOR);
}


static void
phosh_monitor_manager_init (PhoshMonitorManager *self)
{
  self->monitors = g_ptr_array_new_with_free_func ((GDestroyNotify) (g_object_unref));
  self->heads = g_ptr_array_new_with_free_func ((GDestroyNotify) (g_object_unref));
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
  g_signal_emit (self, signals[SIGNAL_MONITOR_ADDED], 0, monitor);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_N_MONITORS]);
}


PhoshMonitor *
phosh_monitor_manager_get_monitor (PhoshMonitorManager *self, guint num)
{
  if (num >= self->monitors->len)
    return NULL;

  return g_ptr_array_index (self->monitors, num);
}


PhoshMonitor *
phosh_monitor_manager_find_monitor (PhoshMonitorManager *self, const char *name)
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

/**
 * phosh_monitor_set_transform:
 * @self: A #PhoshMonitor
 * @mode: The #PhoshMonitorPowerSaveMode
 *
 * Sets monitor's transform. This will become active after the next
 * call to #phosh_monitor_manager_apply_monitor_config().
 */
void
phosh_monitor_manager_set_monitor_transform (PhoshMonitorManager *self,
                                             PhoshMonitor *monitor,
                                             PhoshMonitorTransform transform)
{
  g_autoptr(PhoshHead) head = NULL;

  g_return_if_fail (PHOSH_IS_MONITOR_MANAGER (self));
  g_return_if_fail (PHOSH_IS_MONITOR (monitor));
  g_return_if_fail (phosh_monitor_is_configured (monitor));
  head = g_object_ref (phosh_monitor_manager_get_head_from_monitor (self, monitor));
  g_return_if_fail (PHOSH_IS_HEAD (head));

  head->pending.transform = transform;
}

/**
 * phosh_monitor_manager_apply_monitor_config
 * @self: a #PhoshMonitorManager
 *
 * Applies a full output configuration
 */
void
phosh_monitor_manager_apply_monitor_config (PhoshMonitorManager *self)
{
  PhoshWayland *wl = phosh_wayland_get_default();
  struct zwlr_output_configuration_v1 *config;
  struct zwlr_output_manager_v1 *output_manager =
    phosh_wayland_get_zwlr_output_manager_v1 (wl);

  g_return_if_fail (PHOSH_IS_MONITOR_MANAGER (self));

  config = zwlr_output_manager_v1_create_configuration (output_manager,
                                                        self->zwlr_output_serial);

  zwlr_output_configuration_v1_add_listener (config, &config_listener, self);
  for (int i = 0; i < self->heads->len; i++) {
    struct zwlr_output_configuration_head_v1 *config_head;
    PhoshHead *head = g_ptr_array_index (self->heads, i);
    struct zwlr_output_head_v1 *wlr_head = phosh_head_get_wlr_head (head);
    struct zwlr_output_mode_v1 *wlr_mode;

    g_debug ("Adding %sabled head %s to configuration",
             head->pending.enabled ? "en" : "dis",
             head->name);

    if (!head->pending.enabled) {
      zwlr_output_configuration_v1_disable_head (config, wlr_head);
      continue;
    }

    config_head = zwlr_output_configuration_v1_enable_head (config, wlr_head);

    /* A disabled head might not have a mode when set */
    if (head->pending.mode) {
      wlr_mode = head->pending.mode->wlr_mode;
    } else {
      wlr_mode = phosh_head_get_preferred_mode (head)->wlr_mode;
    }

    zwlr_output_configuration_head_v1_set_mode (config_head, wlr_mode);
    zwlr_output_configuration_head_v1_set_position (config_head,
                                                    head->pending.x, head->pending.y);
    zwlr_output_configuration_head_v1_set_transform (config_head, head->pending.transform);
    zwlr_output_configuration_head_v1_set_scale (config_head,
                                                 wl_fixed_from_double(head->pending.scale));
  }

  zwlr_output_configuration_v1_apply (config);
}
