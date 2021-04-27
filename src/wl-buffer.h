/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#pragma once

#include <glib.h>
#include <wayland-client.h>

G_BEGIN_DECLS

/**
 * PhoshWlBuffer:
 * @data: The actual data
 * @width: The buffer width in pixels
 * @height: The buffer height in pixels
 * @stride: The buffer stride in bytes
 * @format: The buffer format
 *
 * A buffer received from the Wayland compositor containing image
 * data.
 */
typedef struct {
  void              *data;
  uint32_t           width, height, stride;
  enum wl_shm_format format;
  /*< private >*/
  struct wl_buffer  *wl_buffer;
} PhoshWlBuffer;

PhoshWlBuffer *phosh_wl_buffer_new (enum wl_shm_format format, uint32_t width, uint32_t height, uint32_t stride);
void           phosh_wl_buffer_destroy (PhoshWlBuffer *self);
gsize          phosh_wl_buffer_get_size (PhoshWlBuffer *self);
GBytes        *phosh_wl_buffer_get_bytes (PhoshWlBuffer *self);

G_END_DECLS
