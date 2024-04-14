/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-head"

#include "head-priv.h"

#include <gdk/gdkwayland.h>

/**
 * PhoshHead:
 *
 * An output head
 *
 * A output head (usually a monitor). Only enabled heads correspond to a
 * wl_output and #PhoshMonitor. #PhoshHead should be considered an
 * implementation detail of #PhoshMonitorManager and not be used outside of it.
 */

enum {
  SIGNAL_HEAD_FINISHED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

enum {
  PHOSH_HEAD_PROP_0,
  PHOSH_HEAD_PROP_WLR_HEAD,
  PHOSH_HEAD_PROP_NAME,
  PHOSH_HEAD_PROP_LAST_PROP,
};
static GParamSpec *props[PHOSH_HEAD_PROP_LAST_PROP];

#define MINIMUM_LOGICAL_AREA_LANDSCAPE (800 * 480)
#define MINIMUM_LOGICAL_AREA_PORTRAIT (360 * 720)
#define MINIMUM_SCALE_FACTOR 1.0f
#define MAXIMUM_SCALE_FACTOR 4.0f
#define SCALE_FACTORS_PER_INTEGER 4
#define SCALE_FACTORS_STEPS (1.0 / (float) SCALE_FACTORS_PER_INTEGER)

G_DEFINE_TYPE (PhoshHead, phosh_head, G_TYPE_OBJECT);


static void
phosh_head_mode_destroy (PhoshHeadMode *mode)
{
  g_return_if_fail (PHOSH_IS_HEAD (mode->head));

  g_clear_pointer (&mode->wlr_mode, zwlr_output_mode_v1_destroy);
  g_free (mode->name);
  g_free (mode);
}


static void
mode_name (PhoshHeadMode *mode)
{
  if (mode->name)
    return;

  if (!mode->refresh || !mode->width || !mode->height)
    return;

  mode->name = g_strdup_printf ("%dx%d@%.0f", mode->width, mode->height,
                                mode->refresh / 1000.0);

}


static gboolean
is_logical_size_large_enough (float width, float height)
{
  int area = (width > height) ? MINIMUM_LOGICAL_AREA_LANDSCAPE : MINIMUM_LOGICAL_AREA_PORTRAIT;

  return width * height >= area;
}


static gboolean
is_valid_scale (float width, float height, float scale)
{
  float scaled_h = height / scale;
  float scaled_w = width / scale;

  if (scale < MINIMUM_SCALE_FACTOR || scale > MAXIMUM_SCALE_FACTOR)
    return FALSE;

  return is_logical_size_large_enough (floorf (scaled_w), floorf(scaled_h));
}


static void
zwlr_output_mode_v1_handle_size (void *data, struct zwlr_output_mode_v1 *wlr_mode,
                                 int32_t width, int32_t height)
{
  PhoshHeadMode *mode = data;

  mode->width = width;
  mode->height = height;
  mode_name (mode);
}


static void
zwlr_output_mode_v1_handle_refresh (void                       *data,
                                    struct zwlr_output_mode_v1 *wlr_mode,
                                    int32_t                     refresh)
{
  PhoshHeadMode *mode = data;

  mode->refresh = refresh;
  mode_name (mode);
}


static void
zwlr_output_mode_v1_handle_preferred (void                       *data,
                                      struct zwlr_output_mode_v1 *wlr_mode)
{
  PhoshHeadMode *mode = data;

  mode->preferred = TRUE;
}


static void
zwlr_output_mode_v1_handle_finished (void                       *data,
                                     struct zwlr_output_mode_v1 *wlr_mode)
{
  PhoshHeadMode *mode = data;

  /* Array removal triggers phosh_head_mode_destroy */
  if (!g_ptr_array_remove (mode->head->modes, mode))
    g_warning ("Failed to remove mode %p", mode);
}


static const struct zwlr_output_mode_v1_listener mode_listener = {
  .size = zwlr_output_mode_v1_handle_size,
  .refresh = zwlr_output_mode_v1_handle_refresh,
  .preferred = zwlr_output_mode_v1_handle_preferred,
  .finished = zwlr_output_mode_v1_handle_finished,
};

static PhoshHeadMode *
phosh_head_mode_new_from_wlr_mode (PhoshHead *head, struct zwlr_output_mode_v1 *wlr_mode)
{
  PhoshHeadMode *mode = g_new0 (PhoshHeadMode, 1);

  mode->wlr_mode = wlr_mode;
  /* head outlives the mode so no need to take a ref */
  mode->head = head;
  zwlr_output_mode_v1_add_listener (mode->wlr_mode, &mode_listener, mode);
  return mode;
}

static void
head_handle_name (void                       *data,
                  struct zwlr_output_head_v1 *head,
                  const char                 *name)
{
  PhoshHead *self = PHOSH_HEAD (data);

  g_return_if_fail (PHOSH_IS_HEAD (self));
  g_debug ("Head %p is named %s", self, name);

  self->name = g_strdup (name);

  /* wlroots uses the connector's name as output name so
     try to derive the connector type from it */
  self->conn_type = phosh_monitor_connector_type_from_name (name);
}


static void
head_handle_description (void                       *data,
                         struct zwlr_output_head_v1 *head,
                         const char                 *description)
{
  PhoshHead *self = PHOSH_HEAD (data);

  g_return_if_fail (PHOSH_IS_HEAD (self));
  g_debug ("Head %p has description %s", self, description);

  self->description = g_strdup (description);
}


static void
head_handle_physical_size  (void *data,
                            struct zwlr_output_head_v1 *head,
                            int32_t width, int32_t height)
{
  PhoshHead *self = PHOSH_HEAD (data);

  g_return_if_fail (PHOSH_IS_HEAD (self));

  g_debug ("Head %p has physical size %dx%d", self, width, height);
  self->phys.width = width;
  self->phys.height = height;
}


static void
head_handle_mode (void                       *data,
                  struct zwlr_output_head_v1 *head,
                  struct zwlr_output_mode_v1 *wlr_mode)
{
  PhoshHead *self = PHOSH_HEAD (data);
  PhoshHeadMode *mode;

  g_return_if_fail (PHOSH_IS_HEAD (self));

  mode = phosh_head_mode_new_from_wlr_mode (self, wlr_mode);
  g_debug ("Head %p has mode %p", self, wlr_mode);
  g_ptr_array_add (self->modes, mode);
}


static void
head_handle_enabled (void                       *data,
                     struct zwlr_output_head_v1 *head,
                     int32_t                     enabled)
{
  PhoshHead *self = PHOSH_HEAD (data);

  g_return_if_fail (PHOSH_IS_HEAD (self));
  self->enabled = self->pending.enabled = !!enabled;
  g_debug ("Head %p is %sabled", self, self->enabled ? "en" : "dis");
}


static void
head_handle_current_mode (void                       *data,
                          struct zwlr_output_head_v1 *head,
                          struct zwlr_output_mode_v1 *wlr_mode)
{
  PhoshHead *self = PHOSH_HEAD (data);

  g_return_if_fail (PHOSH_IS_HEAD (self));

  for (int i = 0; i < self->modes->len; i++) {
    PhoshHeadMode *mode = g_ptr_array_index (self->modes, i);

    if (mode->wlr_mode == wlr_mode) {
      g_debug ("Head %p has current mode %p", self, mode);
      self->mode = self->pending.mode = mode;
      return;
    }
  }

  g_warning ("Head %p received invalid current mode %px", head, wlr_mode);
}


static void
head_handle_position (void *data,
                      struct zwlr_output_head_v1 *head,
                      int32_t x, int32_t y)
{
  PhoshHead *self = PHOSH_HEAD (data);

  g_return_if_fail (PHOSH_IS_HEAD (self));
  g_debug ("Head %p has pos %d,%d", self, x, y);
  self->x = self->pending.x = x;
  self->y = self->pending.y = y;
}


static void
head_handle_transform (void                       *data,
                       struct zwlr_output_head_v1 *head,
                       int32_t                     transform)
{
  PhoshHead *self = PHOSH_HEAD (data);

  g_return_if_fail (PHOSH_IS_HEAD (self));
  g_debug ("Head %p has transform %d", self, transform);
  self->transform = self->pending.transform = transform;
}


static void
head_handle_scale (void                       *data,
                   struct zwlr_output_head_v1 *head,
                   wl_fixed_t                  scale)
{
  PhoshHead *self = PHOSH_HEAD (data);

  g_return_if_fail (PHOSH_IS_HEAD (self));
  self->scale = self->pending.scale = wl_fixed_to_double(scale);
  g_debug ("Head %p has scale %f", self, self->scale);

}


static void
head_handle_finished (void                       *data,
                      struct zwlr_output_head_v1 *head)
{
  PhoshHead *self = PHOSH_HEAD (data);

  g_return_if_fail (PHOSH_IS_HEAD (self));

  g_debug ("Head %p finished", self);
  g_signal_emit (self, signals[SIGNAL_HEAD_FINISHED], 0);
}

static void
head_handle_make (void *data,
                  struct zwlr_output_head_v1 *zwlr_output_head_v1,
                  const char *make)
{
  PhoshHead *self = PHOSH_HEAD (data);

  g_return_if_fail (PHOSH_IS_HEAD (self));

  g_free (self->vendor);
  self->vendor = g_strdup (make);
  g_debug ("Head %p has vendor %s", self, self->vendor);
}


static void
head_handle_model (void *data,
                   struct zwlr_output_head_v1 *zwlr_output_head_v1,
                   const char *model)
{
  PhoshHead *self = PHOSH_HEAD (data);

  g_return_if_fail (PHOSH_IS_HEAD (self));

  g_free (self->product);
  self->product = g_strdup (model);
  g_debug ("Head %p has product %s", self, self->product);
}


static void head_handle_serial_number (void *data,
                                       struct zwlr_output_head_v1 *zwlr_output_head_v1,
                                       const char *serial_number)
{
  PhoshHead *self = PHOSH_HEAD (data);

  g_return_if_fail (PHOSH_IS_HEAD (self));

  g_free (self->serial);
  self->serial = g_strdup (serial_number);
  g_debug ("Head %p has serial %s", self, self->serial);
}


static const struct zwlr_output_head_v1_listener zwlr_output_head_v1_listener =
{
  .name = head_handle_name,
  .description = head_handle_description,
  .physical_size = head_handle_physical_size,
  .mode = head_handle_mode,
  .enabled = head_handle_enabled,
  .current_mode = head_handle_current_mode,
  .position = head_handle_position,
  .transform = head_handle_transform,
  .scale = head_handle_scale,
  .finished = head_handle_finished,
  .make = head_handle_make,
  .model = head_handle_model,
  .serial_number = head_handle_serial_number,
};


static void
phosh_head_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  PhoshHead *self = PHOSH_HEAD (object);

  switch (property_id) {
  case PHOSH_HEAD_PROP_WLR_HEAD:
    self->wlr_head = g_value_get_pointer (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_head_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  PhoshHead *self = PHOSH_HEAD (object);

  switch (property_id) {
  case PHOSH_HEAD_PROP_WLR_HEAD:
    g_value_set_pointer (value, self->wlr_head);
    break;
  case PHOSH_HEAD_PROP_NAME:
    g_value_set_string (value, self->name);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_head_dispose (GObject *object)
{
  PhoshHead *self = PHOSH_HEAD (object);

  g_ptr_array_free (self->modes, TRUE);
  g_clear_pointer (&self->description, g_free);
  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->vendor, g_free);
  g_clear_pointer (&self->product, g_free);
  g_clear_pointer (&self->serial, g_free);
  g_clear_pointer (&self->wlr_head, zwlr_output_head_v1_destroy);

  G_OBJECT_CLASS (phosh_head_parent_class)->dispose (object);
}


static void
phosh_head_constructed (GObject *object)
{
  PhoshHead *self = PHOSH_HEAD (object);

  self->modes = g_ptr_array_new_with_free_func ((GDestroyNotify)phosh_head_mode_destroy);
  zwlr_output_head_v1_add_listener (self->wlr_head, &zwlr_output_head_v1_listener, self);
}


static void
phosh_head_class_init (PhoshHeadClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_head_constructed;
  object_class->dispose = phosh_head_dispose;

  object_class->set_property = phosh_head_set_property;
  object_class->get_property = phosh_head_get_property;

  props[PHOSH_HEAD_PROP_WLR_HEAD] =
    g_param_spec_pointer ("wlr-head",
                          "wlr-head",
                          "The wlr head associated with this head",
                          G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS);
  props[PHOSH_HEAD_PROP_NAME] =
    g_param_spec_string ("name",
                         "Name",
                         "The head's name",
                         "",
                         G_PARAM_READABLE |
                         G_PARAM_STATIC_STRINGS);

  signals[SIGNAL_HEAD_FINISHED] = g_signal_new ("head-finished",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  g_object_class_install_properties (object_class, PHOSH_HEAD_PROP_LAST_PROP, props);
}


static void
phosh_head_init (PhoshHead *self)
{
}


PhoshHead *
phosh_head_new_from_wlr_head (gpointer wlr_head)
{
  return g_object_new (PHOSH_TYPE_HEAD, "wlr-head", wlr_head, NULL);
}


/**
 * phosh_head_set_pending_transform:
 * @self: The head to transform
 * @transform: The transform to apply to the head
 * @heads:(element-type PhoshHead): All currently known heads
 *
 * Set the heads pending transform. Move pending positions of heads to
 * the right and below @self around to avoid gaps and overlaps in the
 * layout.
 */
void
phosh_head_set_pending_transform (PhoshHead             *self,
                                  PhoshMonitorTransform  transform,
                                  GPtrArray             *heads)

{
  int dx, dy;
  gboolean was_tilted, tilted;

  g_return_if_fail (self);
  was_tilted = phosh_monitor_transform_is_tilted ((PhoshMonitorTransform) self->pending.transform);
  tilted = phosh_monitor_transform_is_tilted (transform);

  self->pending.transform = (enum wl_output_transform) transform;

  /* We need to adjust the positions of monitors to the right and below the transformed
     ones to avoid gaps if tilting changed */
  if (was_tilted == tilted)
    return;

  g_return_if_fail (self->pending.mode);
  g_return_if_fail (self->pending.scale >= 0.0);
  dx = (self->pending.mode->height - self->pending.mode->width) / self->pending.scale;
  dy = (self->pending.mode->width - self->pending.mode->height) / self->pending.scale;
  if (was_tilted) {
    dx *= -1;
    dy *= -1;
  }

  g_debug ("Output orientation of %s changed, "
           "adjusting layout: dx: %d, dy: %d", self->name, dx, dy);

  for (int i = 0; i < heads->len; i++) {
    PhoshHead *move_head = g_ptr_array_index (heads, i);

    if (move_head->pending.x > self->pending.x)
      move_head->pending.x += dx;

    if (move_head->pending.y > self->pending.y)
      move_head->pending.y += dy;
  }
}


/**
 * phosh_head_get_wlr_head:
 * @self: The #PhoshHead
 *
 * Get the heads wlr_head
 *
 * Returns:(transfer none): The wayland head.
 */
struct zwlr_output_head_v1 *
phosh_head_get_wlr_head (PhoshHead *self)
{
  g_return_val_if_fail (PHOSH_IS_HEAD (self), NULL);

  return self->wlr_head;
}

/**
 * phosh_head_is_enabled:
 * @self: The #PhoshHead
 *
 * Whether the head is enabled
 *
 * Returns: %TRUE if the head is enabled, otherwise %FALSE
 */
gboolean
phosh_head_get_enabled (PhoshHead *self)
{
  g_return_val_if_fail (PHOSH_IS_HEAD (self), TRUE);

  return self->enabled;
}

/**
 * phosh_head_get_preferred_mode:
 * @self: The #PhoshHead
 *
 * Get the preferred mode
 *
 * Returns:(transfer none): The preferred mode
 */
PhoshHeadMode *
phosh_head_get_preferred_mode (PhoshHead *self)
{
  g_return_val_if_fail (PHOSH_IS_HEAD (self), NULL);

  for (int i = 0; i < self->modes->len; i++) {
    PhoshHeadMode *mode = g_ptr_array_index (self->modes, i);

    if (mode->preferred)
      return mode;
  }

  return NULL;
}

/**
 * phosh_head_is_builtin:
 * @self: A #PhoshHead
 *
 * Checks whether the given head is a builtin display.
 *
 * Returns: %TRUE if the head built in panel (e.g. laptop panel or
 * phone LCD)
 */
gboolean
phosh_head_is_builtin (PhoshHead *self)
{
  g_return_val_if_fail (PHOSH_IS_HEAD (self), FALSE);

  return phosh_monitor_connector_is_builtin (self->conn_type);
}

/**
 * phosh_head_find_mode_by_name: (skip)
 * @self: A #PhoshHead
 * @name: The name of a mode
 *
 * Look up the given mode by its name.
 *
 * Returns: The #PhoshHeadMode if found, otherwise %NULL
 */
PhoshHeadMode *
phosh_head_find_mode_by_name (PhoshHead *self, const char *name)
{
  g_return_val_if_fail (PHOSH_IS_HEAD (self), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (int i = 0; i < self->modes->len; i++) {
    PhoshHeadMode *mode = g_ptr_array_index (self->modes, i);

    if (!g_strcmp0 (mode->name, name))
        return mode;
  }
  return NULL;
}


static float
get_closest_scale_factor_for_resolution (float width, float height, float scale, float threshold)
{
  unsigned int i;
  float scaled_h;
  float scaled_w;
  float best_scale;
  float base_scaled_w;
  gboolean found_one;

  best_scale = 0;

  if (!is_valid_scale (width, height, scale))
    return 0.0;

  if (G_APPROX_VALUE (fmodf (width, scale), 0.0, FLT_EPSILON) &&
      G_APPROX_VALUE (fmodf (height, scale), 0.0, FLT_EPSILON))
    return scale;

  i = 0;
  found_one = FALSE;
  base_scaled_w = floorf (width / scale);

  do {
    for (int j = 0; j < 2; j++) {
      float current_scale;
      int offset = i * (j ? 1 : -1);

      scaled_w = base_scaled_w + offset;
      current_scale = width / scaled_w;
      scaled_h = height / current_scale;

      if (current_scale >= scale + threshold ||
          current_scale <= scale - threshold ||
          current_scale < MINIMUM_SCALE_FACTOR ||
          current_scale > MAXIMUM_SCALE_FACTOR) {
        return best_scale;
      }

      if (G_APPROX_VALUE (floorf (scaled_h), scaled_h, FLT_EPSILON)) {
        found_one = TRUE;

        if (fabsf (current_scale - scale) < fabsf (best_scale - scale))
          best_scale = current_scale;
      }
    }
    i++;
  } while (!found_one);

  return best_scale;
}


float *
phosh_head_calculate_supported_mode_scales (PhoshHead     *head,
                                            PhoshHeadMode *mode,
                                            int           *n_supported_scales,
                                            gboolean       fractional)
{
  GArray *supported_scales;

  supported_scales = g_array_new (FALSE, FALSE, sizeof (float));

  if (fractional) {
    for (float i = floorf (MINIMUM_SCALE_FACTOR); i <= ceilf (MAXIMUM_SCALE_FACTOR); i++) {
      float max_bound;

      if (G_APPROX_VALUE (floorf (MINIMUM_SCALE_FACTOR), i, FLT_EPSILON) ||
          G_APPROX_VALUE (ceilf (MAXIMUM_SCALE_FACTOR), i, FLT_EPSILON)) {
        max_bound = SCALE_FACTORS_STEPS;
      } else {
        max_bound = SCALE_FACTORS_STEPS / 2.0;
      }

      for (int j = 0; j < SCALE_FACTORS_PER_INTEGER; j++) {
        float scale;
        float scale_value = i + j * SCALE_FACTORS_STEPS;

        scale = get_closest_scale_factor_for_resolution (mode->width, mode->height,
                                                         scale_value,
                                                         max_bound);
        if (scale > 0.0)
          g_array_append_val (supported_scales, scale);
      }
    }
  } else {
    for (float f = floorf (MINIMUM_SCALE_FACTOR); f <= ceilf (MAXIMUM_SCALE_FACTOR); f++) {
      if (is_valid_scale (mode->width, mode->height, f)) {
        g_array_append_val (supported_scales, f);
      }
    }
  }

  if (supported_scales->len == 0) {
    float fallback_scale = 1.0;
    g_array_append_val (supported_scales, fallback_scale);
  }

  *n_supported_scales = supported_scales->len;
  return (float *) g_array_free (supported_scales, FALSE);
}

/**
 * phosh_head_clear_pending:
 * @self: A #PhoshHead
 *
 * Clear all pending state. This can be used if e.g.  pending state
 * was set but the output configuration not submitted.
 */
void
phosh_head_clear_pending (PhoshHead *self)
{
  self->pending.seen = FALSE;
  self->pending.x = self->x;
  self->pending.y = self->y;
  self->pending.transform = self->transform;
  self->pending.mode = self->mode;
  self->pending.scale = self->scale;
  self->pending.enabled = self->enabled;
}


/**
 * phosh_head_set_pending_enabled:
 * @self: A #PhoshHead
 * @enabled: %TRUE if the head should be enabled, otherwise %FALSE
 *
 * Sets a head to pending enabled. This will become active after the next
 * call to #phosh_monitor_manager_apply_monitor_config().
 */
void
phosh_head_set_pending_enabled (PhoshHead *self, gboolean enabled)
{
  self->pending.enabled = enabled;
}
