/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

/* Stub wlr-output-management handler to not leak head and mode */


#include <testlib-head-stub.h>

static GPtrArray *_heads;
static GPtrArray *_modes;


static void
zwlr_output_mode_v1_handle_size (void *data, struct zwlr_output_mode_v1 *wlr_mode,
                                 int32_t width, int32_t height)
{
}


static void
zwlr_output_mode_v1_handle_refresh (void                       *data,
                                    struct zwlr_output_mode_v1 *wlr_mode,
                                    int32_t                     refresh)
{
}


static void
zwlr_output_mode_v1_handle_preferred (void                       *data,
                                      struct zwlr_output_mode_v1 *wlr_mode)
{
}


static void
zwlr_output_mode_v1_handle_finished (void                       *data,
                                     struct zwlr_output_mode_v1 *wlr_mode)
{
}

static const struct zwlr_output_mode_v1_listener mode_listener = {
  .size = zwlr_output_mode_v1_handle_size,
  .refresh = zwlr_output_mode_v1_handle_refresh,
  .preferred = zwlr_output_mode_v1_handle_preferred,
  .finished = zwlr_output_mode_v1_handle_finished,
};

static void
head_handle_name (void                       *data,
                  struct zwlr_output_head_v1 *head,
                  const char                 *name)
{
}


static void
head_handle_description (void                       *data,
                         struct zwlr_output_head_v1 *head,
                         const char                 *description)
{
}


static void
head_handle_physical_size  (void *data,
                            struct zwlr_output_head_v1 *head,
                            int32_t width, int32_t height)
{
}


static void
head_handle_mode (void                       *data,
                  struct zwlr_output_head_v1 *head,
                  struct zwlr_output_mode_v1 *wlr_mode)
{
  g_ptr_array_add (_modes, wlr_mode);
  zwlr_output_mode_v1_add_listener (wlr_mode, &mode_listener, NULL);
}


static void
head_handle_enabled (void                       *data,
                     struct zwlr_output_head_v1 *head,
                     int32_t                     enabled)
{
}


static void
head_handle_current_mode (void                       *data,
                          struct zwlr_output_head_v1 *head,
                          struct zwlr_output_mode_v1 *wlr_mode)
{
}


static void
head_handle_position (void *data,
                      struct zwlr_output_head_v1 *head,
                      int32_t x, int32_t y)
{
}


static void
head_handle_transform (void                       *data,
                       struct zwlr_output_head_v1 *head,
                       int32_t                     transform)
{
}


static void
head_handle_scale (void                       *data,
                   struct zwlr_output_head_v1 *head,
                   wl_fixed_t                  scale)
{
}


static void
head_handle_finished (void                       *data,
                      struct zwlr_output_head_v1 *head)
{
}

static void
head_handle_make (void *data,
                  struct zwlr_output_head_v1 *zwlr_output_head_v1,
                  const char *make)
{
}


static void
head_handle_model (void *data,
                   struct zwlr_output_head_v1 *zwlr_output_head_v1,
                   const char *model)
{
}


static void head_handle_serial_number (void *data,
                                       struct zwlr_output_head_v1 *zwlr_output_head_v1,
                                       const char *serial_number)
{
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
zwlr_output_manager_v1_handle_head (void *data,
                                    struct zwlr_output_manager_v1 *manager,
                                    struct zwlr_output_head_v1 *wlr_head)
{
  g_ptr_array_add (_heads, wlr_head);
  zwlr_output_head_v1_add_listener (wlr_head, &zwlr_output_head_v1_listener, NULL);
}


static void
zwlr_output_manager_v1_handle_done (void *data,
                                    struct zwlr_output_manager_v1 *manager,
                                    uint32_t serial)
{
}


static const struct zwlr_output_manager_v1_listener zwlr_output_manager_v1_listener = {
  .head = zwlr_output_manager_v1_handle_head,
  .done = zwlr_output_manager_v1_handle_done,
};


void
phosh_test_head_stub_init (PhoshWayland *wl)
{
  struct zwlr_output_manager_v1 *zwlr_output_manager_v1;

  _modes = g_ptr_array_new_with_free_func ((GDestroyNotify)zwlr_output_mode_v1_destroy);
  _heads = g_ptr_array_new_with_free_func ((GDestroyNotify)zwlr_output_head_v1_destroy);

  zwlr_output_manager_v1 = phosh_wayland_get_zwlr_output_manager_v1 (wl);
  zwlr_output_manager_v1_add_listener (zwlr_output_manager_v1,
                                       &zwlr_output_manager_v1_listener,
                                       NULL);
}

void
phosh_test_head_stub_destroy (void)
{
  if (_heads)
    g_ptr_array_free (_heads, TRUE);
  if (_modes)
    g_ptr_array_free (_modes, TRUE);
}
