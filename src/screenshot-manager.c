/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-screenshot-manager"

#include "config.h"
#include "phosh-wayland.h"
#include "screenshot-manager.h"
#include "shell.h"
#include "util.h"
#include "wl-buffer.h"

#include "dbus/phosh-screenshot-dbus.h"

#define BUS_NAME "org.gnome.Shell.Screenshot"
#define OBJECT_PATH "/org/gnome/Shell/Screenshot"

/**
 * SECTION:screenshot-manager
 * @short_description: Screenshot interaction
 * @Title: PhoshScreenshotManager
 *
 * The #PhoshScreenshotManager is responsible for
 * taking screenshots.
 */

static void phosh_screenshot_manager_screenshot_iface_init (
  PhoshDBusScreenshotIface *iface);


typedef struct _ScreencopyFrame {
  struct zwlr_screencopy_frame_v1 *frame;
  uint32_t                         flags;
  char                            *filename;
  GDBusMethodInvocation           *invocation;
  PhoshWlBuffer                   *buffer;
} ScreencopyFrame;


typedef struct _PhoshScreenshotManager {
  PhoshDBusScreenshotSkeleton        parent;

  int                                dbus_name_id;
  struct zwlr_screencopy_manager_v1 *wl_scm;
  ScreencopyFrame                   *frame;
} PhoshScreenshotManager;


G_DEFINE_TYPE_WITH_CODE (PhoshScreenshotManager,
                         phosh_screenshot_manager,
                         PHOSH_DBUS_TYPE_SCREENSHOT_SKELETON,
                         G_IMPLEMENT_INTERFACE (
                           PHOSH_DBUS_TYPE_SCREENSHOT,
                           phosh_screenshot_manager_screenshot_iface_init));


static void
screencopy_frame_dispose (ScreencopyFrame *frame)
{
  phosh_wl_buffer_destroy (frame->buffer);
  g_clear_pointer (&frame->frame, zwlr_screencopy_frame_v1_destroy);
  g_free (frame->filename);
  g_free (frame);
}


static void
screencopy_frame_handle_buffer (void                            *data,
                                struct zwlr_screencopy_frame_v1 *frame,
                                uint32_t                         format,
                                uint32_t                         width,
                                uint32_t                         height,
                                uint32_t                         stride)
{
  PhoshScreenshotManager *self = PHOSH_SCREENSHOT_MANAGER (data);

  g_return_if_fail (PHOSH_IS_SCREENSHOT_MANAGER (self));

  g_debug ("Handling buffer %dx%d for %s", width, height, self->frame->filename);
  self->frame->buffer = phosh_wl_buffer_new (format, width, height, stride);
  g_return_if_fail (self->frame->buffer);

  zwlr_screencopy_frame_v1_copy (frame, self->frame->buffer->wl_buffer);
}


static void
screencopy_frame_handle_flags (void                            *data,
                               struct zwlr_screencopy_frame_v1 *frame,
                               uint32_t                         flags)
{
  PhoshScreenshotManager *self = PHOSH_SCREENSHOT_MANAGER (data);

  g_return_if_fail (PHOSH_IS_SCREENSHOT_MANAGER (self));
  g_return_if_fail (self->frame);

  self->frame->flags = flags;
}


static void
screencopy_done (PhoshScreenshotManager *self, gboolean success)
{
  phosh_dbus_screenshot_complete_screenshot (PHOSH_DBUS_SCREENSHOT (self),
                                             self->frame->invocation,
                                             success,
                                             self->frame->filename ?: "");
  g_clear_pointer (&self->frame, screencopy_frame_dispose);
  /* TODO: GNOME >= 40 wants us to emit the click sound from here */
  /* TODO: flash screen */
}


static void
on_save_pixbuf_ready (GObject      *source_object,
                      GAsyncResult *res,
                      gpointer      user_data)
{
  gboolean success;

  g_autoptr (GError) err = NULL;
  PhoshScreenshotManager *self = PHOSH_SCREENSHOT_MANAGER (user_data);

  g_return_if_fail (PHOSH_IS_SCREENSHOT_MANAGER (self));

  success = gdk_pixbuf_save_to_stream_finish (res, &err);
  if (!success)
    g_warning ("Failed to save screenshot to %s: %s", self->frame->filename, err->message);

  screencopy_done (self, success);
  g_object_unref (self);
}



static void
screencopy_frame_handle_ready (void                            *data,
                               struct zwlr_screencopy_frame_v1 *frame,
                               uint32_t                         tv_sec_hi,
                               uint32_t                         tv_sec_lo,
                               uint32_t                         tv_nsec)
{
  PhoshScreenshotManager *self = PHOSH_SCREENSHOT_MANAGER (data);

  g_autoptr (GdkPixbuf) pixbuf = NULL;
  g_autoptr (GError) err = NULL;
  g_autoptr (GFileOutputStream) stream = NULL;
  g_autoptr (GFile) file = NULL;

  g_return_if_fail (PHOSH_IS_SCREENSHOT_MANAGER (self));
  g_debug ("Frame %p %dx%d, stride %d, format 0x%x ready, saving to %s",
           frame,
           self->frame->buffer->width,
           self->frame->buffer->height,
           self->frame->buffer->stride,
           self->frame->buffer->format,
           self->frame->filename);

  switch ((uint32_t) self->frame->buffer->format) {
  case WL_SHM_FORMAT_ABGR8888:
  case WL_SHM_FORMAT_XBGR8888:
    break;
  case WL_SHM_FORMAT_ARGB8888:
  case WL_SHM_FORMAT_XRGB8888: { /* ARGB -> ABGR */
    PhoshWlBuffer *buffer = self->frame->buffer;
    uint8_t *d = buffer->data;
    for (int i = 0; i < buffer->height; ++i) {
      for (int j = 0; j < buffer->width; ++j) {
        uint32_t *px = (uint32_t *)(d + i * buffer->stride + j * 4);
        uint8_t a = (*px >> 24) & 0xFF;
        uint8_t b = (*px >> 16) & 0xFF;
        uint8_t g = (*px >> 8) & 0xFF;
        uint8_t r = *px & 0xFF;
        *px = (a << 24) | (r << 16) | (g << 8) | b;
      }
    }
    if (buffer->format == WL_SHM_FORMAT_ARGB8888)
      buffer->format = WL_SHM_FORMAT_ABGR8888;
    else
      buffer->format = WL_SHM_FORMAT_XBGR8888;
  }
  break;
  default:
    goto error;
  }

  pixbuf = gdk_pixbuf_new_from_data (self->frame->buffer->data,
                                     GDK_COLORSPACE_RGB,
                                     TRUE,
                                     8,
                                     self->frame->buffer->width,
                                     self->frame->buffer->height,
                                     self->frame->buffer->stride,
                                     NULL,
                                     NULL);
  if (self->frame->flags & ZWLR_SCREENCOPY_FRAME_V1_FLAGS_Y_INVERT) {
    GdkPixbuf *tmp = gdk_pixbuf_flip (pixbuf, FALSE);
    g_object_unref (pixbuf);
    pixbuf = tmp;
  }

  file = g_file_new_for_path (self->frame->filename);
  stream = g_file_create (file, G_FILE_CREATE_NONE, NULL, &err);
  if (!stream) {
    g_warning ("Failed to save screenshot %s: %s", self->frame->filename, err->message);
    goto error;
  }

  gdk_pixbuf_save_to_stream_async (pixbuf,
                                   G_OUTPUT_STREAM (stream),
                                   "png",
                                   NULL,
                                   on_save_pixbuf_ready,
                                   g_object_ref (self),
                                   NULL);
  return;

error:
  screencopy_done (self, FALSE);
}


static void
screencopy_frame_handle_failed (void                            *data,
                                struct zwlr_screencopy_frame_v1 *frame)
{
  PhoshScreenshotManager *self = PHOSH_SCREENSHOT_MANAGER (data);

  g_return_if_fail (PHOSH_IS_SCREENSHOT_MANAGER (self));

  g_warning ("Failed to copy output\n");
  screencopy_done (self, FALSE);
}


static const struct zwlr_screencopy_frame_v1_listener screencopy_frame_listener = {
  .buffer = screencopy_frame_handle_buffer,
  .flags = screencopy_frame_handle_flags,
  .ready = screencopy_frame_handle_ready,
  .failed = screencopy_frame_handle_failed,
};


/**
 * build_screenshot_filename:
 * @pattern: Absolute path or relative name without extension
 *
 * Builds an absolute filename based on the given input pattern.
 * Returns: The target filename or %NULL on errors.
 */
static char *
build_screenshot_filename (const char *pattern)
{
  g_autofree char *filename = NULL;

  if (g_path_is_absolute (pattern)) {
    return g_strdup (pattern);
  } else {
    const char *dir = NULL;
    const char *const *dirs = (const char * []) {
      g_get_user_special_dir (G_USER_DIRECTORY_PICTURES),
      g_get_home_dir (),
      NULL
    };

    for (int i = 0; i < g_strv_length ((GStrv) dirs); i++) {
      if (g_file_test (dirs[i], G_FILE_TEST_EXISTS)) {
        dir = dirs[i];
        break;
      }
    }

    if (!dir)
      return NULL;

    filename = g_build_filename (dir, pattern, NULL);
  }
  if (!g_str_has_suffix (filename, ".png")) {
    g_autofree gchar *tmp = filename;
    filename = g_strdup_printf ("%s.png", filename);
  }

  return g_steal_pointer (&filename);
}


static gboolean
handle_screenshot (PhoshDBusScreenshot   *object,
                   GDBusMethodInvocation *invocation,
                   gboolean               arg_include_cursor,
                   gboolean               arg_flash,
                   const char            *arg_filename)
{
  PhoshScreenshotManager *self = PHOSH_SCREENSHOT_MANAGER (object);
  PhoshWayland *wl = phosh_wayland_get_default ();
  struct zwlr_screencopy_manager_v1 *wl_scm;
  PhoshMonitor *monitor;
  ScreencopyFrame *frame;

  g_debug ("DBus call %s, cursor: %d, flash %d, to %s",
           __func__, arg_include_cursor, arg_flash, arg_filename);

  g_return_val_if_fail (PHOSH_IS_WAYLAND (wl), FALSE);

  wl_scm = phosh_wayland_get_zwlr_screencopy_manager_v1 (wl);
  if (!wl_scm) {
    phosh_dbus_screenshot_complete_screenshot (object, invocation, FALSE, "");
    return TRUE;
  }

  if (self->frame) {
    g_debug ("Screenshot already in progress");
    phosh_dbus_screenshot_complete_screenshot (object, invocation, FALSE, "");
    return TRUE;
  }

  monitor = phosh_shell_get_primary_monitor (phosh_shell_get_default ());
  frame = g_new0 (ScreencopyFrame, 1);

  frame->frame = zwlr_screencopy_manager_v1_capture_output (
    self->wl_scm, arg_include_cursor, monitor->wl_output);
  frame->invocation = invocation;
  self->frame = frame;

  frame->filename = build_screenshot_filename (arg_filename);
  if (!frame->filename) {
    screencopy_done (self, FALSE);
    return TRUE;
  }

  zwlr_screencopy_frame_v1_add_listener (frame->frame, &screencopy_frame_listener, self);
  return TRUE;
}


static void
phosh_screenshot_manager_screenshot_iface_init (PhoshDBusScreenshotIface *iface)
{
  iface->handle_screenshot = handle_screenshot;
}


static void
on_name_acquired (GDBusConnection *connection,
                  const char      *name,
                  gpointer         user_data)
{
  PhoshScreenshotManager *self = PHOSH_SCREENSHOT_MANAGER (user_data);

  g_return_if_fail (PHOSH_IS_SCREENSHOT_MANAGER (self));
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
  PhoshScreenshotManager *self = user_data;

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self),
                                    connection,
                                    OBJECT_PATH,
                                    NULL);
}


static void
phosh_screenshot_manager_constructed (GObject *object)
{
  PhoshScreenshotManager *self = PHOSH_SCREENSHOT_MANAGER (object);
  PhoshWayland *wl = phosh_wayland_get_default ();

  G_OBJECT_CLASS (phosh_screenshot_manager_parent_class)->constructed (object);

  self->dbus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                       BUS_NAME,
                                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                                       G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                       on_bus_acquired,
                                       on_name_acquired,
                                       on_name_lost,
                                       g_object_ref (self),
                                       g_object_unref);

  g_return_if_fail (PHOSH_IS_WAYLAND (wl));
  self->wl_scm = phosh_wayland_get_zwlr_screencopy_manager_v1 (wl);
}


static void
phosh_screenshot_manager_dispose (GObject *object)
{
  PhoshScreenshotManager *self = PHOSH_SCREENSHOT_MANAGER (object);

  if (self->dbus_name_id) {
    g_bus_unown_name (self->dbus_name_id);
    self->dbus_name_id = 0;
  }

  G_OBJECT_CLASS (phosh_screenshot_manager_parent_class)->dispose (object);
}


static void
phosh_screenshot_manager_class_init (PhoshScreenshotManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_screenshot_manager_constructed;
  object_class->dispose = phosh_screenshot_manager_dispose;
}


static void
phosh_screenshot_manager_init (PhoshScreenshotManager *self)
{
}

PhoshScreenshotManager *
phosh_screenshot_manager_new (void)
{
  return PHOSH_SCREENSHOT_MANAGER (g_object_new (PHOSH_TYPE_SCREENSHOT_MANAGER, NULL));
}
