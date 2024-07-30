/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-settings"

#include <glib/gi18n.h>

#include "phosh-config.h"

#include "media-player.h"
#include "mode-manager.h"
#include "plugin-loader.h"
#include "shell.h"
#include "settings.h"
#include "quick-setting.h"
#include "settings/audio-settings.h"
#include "settings/brightness.h"
#include "torch-info.h"
#include "torch-manager.h"
#include "vpn-manager.h"
#include "bt-status-page.h"
#include "wwan/wwan-manager.h"
#include "notifications/notify-manager.h"
#include "notifications/notification-frame.h"
#include "rotateinfo.h"
#include "util.h"

#include <gio/gdesktopappinfo.h>
#include <xkbcommon/xkbcommon.h>

#include <math.h>

#define STACK_CHILD_NOTIFICATIONS    "notifications"
#define STACK_CHILD_NO_NOTIFICATIONS "no-notifications"

/**
 * PhoshSettings:
 *
 * The settings menu
 */
enum {
  PROP_0,
  PROP_ON_LOCKSCREEN,
  PROP_DRAG_HANDLE_OFFSET,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

enum {
  SETTING_DONE,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

typedef struct _PhoshSettings
{
  GtkBin parent;

  gboolean   on_lockscreen;
  gint       drag_handle_offset;
  guint      debounce_handle;

  GtkWidget *scrolled_window;
  GtkWidget *box_sliders;
  GtkWidget *box_settings;
  GtkWidget *quick_settings;
  GtkWidget *scale_brightness;
  GtkWidget *media_player;
  PhoshAudioSettings *audio_settings;

  GtkStack  *stack;

  /* The area with media widget, notifications */
  GtkWidget *box_bottom_half;
  /* Notifications */
  GtkWidget *list_notifications;
  GtkWidget *stack_notifications;

  /* Torch */
  PhoshTorchManager *torch_manager;
  GtkWidget *scale_torch;
  gboolean setting_torch;

  /* Custom quick settings */
  GSettings         *plugin_settings;
  PhoshPluginLoader *plugin_loader;
  GPtrArray         *custom_quick_settings;
} PhoshSettings;


G_DEFINE_TYPE (PhoshSettings, phosh_settings, GTK_TYPE_BIN)


static void
set_on_lockscreen (PhoshSettings *self, gboolean on_lockscreen)
{
  if (self->on_lockscreen == on_lockscreen)
    return;

  self->on_lockscreen = on_lockscreen;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ON_LOCKSCREEN]);

  if (self->on_lockscreen)
    gtk_stack_set_visible_child_name (GTK_STACK (self->stack), "quick_settings_page");
}


static void
phosh_settings_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  PhoshSettings *self = PHOSH_SETTINGS (object);

  switch (property_id) {
  case PROP_ON_LOCKSCREEN:
    set_on_lockscreen (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_settings_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  PhoshSettings *self = PHOSH_SETTINGS (object);

  switch (property_id) {
  case PROP_ON_LOCKSCREEN:
    g_value_set_boolean (value, self->on_lockscreen);
    break;
  case PROP_DRAG_HANDLE_OFFSET:
    g_value_set_int (value, self->drag_handle_offset);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
calc_drag_handle_offset (PhoshSettings *self)
{
  int h = 0;
  int box_height, sw_height, stack_height = 0;
  int stack_y = 0;
  gboolean success;
  const char *stack_page;

  h = gtk_widget_get_allocated_height (GTK_WIDGET (self));
  /* On the lock screen the whole surface is fine */
  if (self->on_lockscreen)
    goto out;

  box_height = gtk_widget_get_allocated_height (self->box_settings);
  sw_height = gtk_widget_get_allocated_height (self->scrolled_window);
  if (box_height > sw_height) {
    h = 0; /* Don't enlarge drag handle if box needs scrolling */
    goto out;
  }

  stack_page = gtk_stack_get_visible_child_name (GTK_STACK (self->stack));
  g_debug ("Calculating drag offset: on stack page: %s", stack_page);

  /* Make sure quick settings' status pages are scrollable */
  if (g_strcmp0 (stack_page, "status_page") != 0)
    goto out;

  success = gtk_widget_translate_coordinates (GTK_WIDGET (self->stack), GTK_WIDGET (self),
                                              0, 0, NULL, &stack_y);

  if (!success) {
    g_warning ("Calculating drag offset: Unable to get stack page's y coordinate");
    goto out;
  }

  stack_height = gtk_widget_get_allocated_height (GTK_WIDGET (self->stack));
  h = stack_y + stack_height;

  g_debug ("Calculating drag offset: stack_y = %d, stack_height = %d, height = %d",
           stack_y, stack_height, h);

 out:
  if (self->drag_handle_offset == h)
    return;

  self->drag_handle_offset = h;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_DRAG_HANDLE_OFFSET]);
}


static void
on_size_allocate (PhoshSettings *self)
{
  calc_drag_handle_offset (self);

  return;
}


static void
delayed_update_drag_handle_offset (gpointer data)
{
  PhoshSettings *self = PHOSH_SETTINGS (data);

  self->debounce_handle = 0;
  calc_drag_handle_offset (self);
}


static void
update_drag_handle_offset (PhoshSettings *self)
{
  g_clear_handle_id (&self->debounce_handle, g_source_remove);
  self->debounce_handle = g_timeout_add_once (200, delayed_update_drag_handle_offset, self);
  g_source_set_name_by_id (self->debounce_handle, "[phosh] delayed_update_drag_handle_offset");
}


static void
on_stack_visible_child_changed (PhoshSettings *self)
{
  static GtkWidget *last_status_page;
  GtkWidget *child, *revealer;
  gboolean reveal;

  update_drag_handle_offset (self);

  child = gtk_stack_get_visible_child (GTK_STACK (self->stack));
  if (child == self->quick_settings && last_status_page) {
    revealer = last_status_page;
    reveal = FALSE;
  } else {
    g_return_if_fail (GTK_IS_REVEALER (child));
    revealer = child;
    reveal = TRUE;
  }

  gtk_revealer_set_reveal_child (GTK_REVEALER (revealer), reveal);
  last_status_page = revealer;
}


static void
close_settings_menu (PhoshSettings *self)
{
  g_signal_emit (self, signals[SETTING_DONE], 0);
  phosh_trigger_feedback ("button-pressed");
}

static void
brightness_value_changed_cb (GtkScale *scale_brightness, gpointer unused)
{
  int brightness;

  brightness = (int)gtk_range_get_value (GTK_RANGE (scale_brightness));
  brightness_set (brightness);
}

static void
rotation_setting_clicked_cb (PhoshSettings *self)
{
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshRotationManager *rotation_manager;
  PhoshRotationManagerMode mode;
  PhoshMonitorTransform transform;
  gboolean locked;

  g_return_if_fail (PHOSH_IS_SETTINGS (self));

  rotation_manager = phosh_shell_get_rotation_manager (shell);
  g_return_if_fail (rotation_manager);
  mode = phosh_rotation_manager_get_mode (PHOSH_ROTATION_MANAGER (rotation_manager));

  switch (mode) {
  case PHOSH_ROTATION_MANAGER_MODE_OFF:
    transform = phosh_rotation_manager_get_transform (rotation_manager) ?
      PHOSH_MONITOR_TRANSFORM_NORMAL : PHOSH_MONITOR_TRANSFORM_270;
    phosh_rotation_manager_set_transform (rotation_manager, transform);
    g_signal_emit (self, signals[SETTING_DONE], 0);
    break;
  case PHOSH_ROTATION_MANAGER_MODE_SENSOR:
    locked = phosh_rotation_manager_get_orientation_locked (rotation_manager);
    phosh_rotation_manager_set_orientation_locked (rotation_manager, !locked);
    break;
  default:
    g_assert_not_reached ();
  }
}


static void
open_settings_panel (PhoshSettings *self, const char *panel)
{
  if (self->on_lockscreen)
    return;

  phosh_quick_setting_open_settings_panel (panel);
  close_settings_menu (self);
}


static void
on_launch_panel_activated (GSimpleAction *action, GVariant *param, gpointer data)
{
  PhoshSettings *self = PHOSH_SETTINGS (data);
  const char *panel;

  panel = g_variant_get_string (param, NULL);

  open_settings_panel (self, panel);
  phosh_audio_settings_hide_details (self->audio_settings);
}

static void
on_close_status_page_activated (GSimpleAction *action, GVariant *param, gpointer data)
{
  PhoshSettings *self = PHOSH_SETTINGS (data);
  GtkStack *stack = GTK_STACK (self->stack);

  gtk_stack_set_visible_child_name (stack, "quick_settings_page");
}


static void
rotation_setting_long_pressed_cb (PhoshSettings *self)
{
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshRotationManagerMode mode;
  PhoshRotationManager *rotation_manager;

  rotation_manager = phosh_shell_get_rotation_manager (shell);
  g_return_if_fail (rotation_manager);

  mode = phosh_rotation_manager_get_mode (rotation_manager);
  switch (mode) {
  case PHOSH_ROTATION_MANAGER_MODE_OFF:
    mode = PHOSH_ROTATION_MANAGER_MODE_SENSOR;
    break;
  case PHOSH_ROTATION_MANAGER_MODE_SENSOR:
    mode = PHOSH_ROTATION_MANAGER_MODE_OFF;
    break;
  default:
    g_assert_not_reached ();
  }
  g_debug ("Rotation manager mode: %d", mode);
  phosh_rotation_manager_set_mode (rotation_manager, mode);
}

static void
feedback_setting_clicked_cb (PhoshSettings *self)
{
  PhoshShell *shell;
  PhoshFeedbackManager *manager;

  shell = phosh_shell_get_default ();
  g_return_if_fail (PHOSH_IS_SHELL (shell));
  manager = phosh_shell_get_feedback_manager (shell);
  g_return_if_fail (PHOSH_IS_FEEDBACK_MANAGER (manager));
  phosh_feedback_manager_toggle (manager);
}

static void
wifi_setting_clicked_cb (PhoshSettings *self)
{
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshWifiManager *manager;
  gboolean enabled;

  g_return_if_fail (PHOSH_IS_SETTINGS (self));

  manager = phosh_shell_get_wifi_manager (shell);
  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (manager));

  enabled = phosh_wifi_manager_get_enabled (manager);
  phosh_wifi_manager_set_enabled (manager, !enabled);
}

static void
wifi_setting_long_pressed_cb (PhoshSettings *self)
{
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshWifiManager *manager;

  if (self->on_lockscreen)
    return;

  manager = phosh_shell_get_wifi_manager (shell);
  g_return_if_fail (PHOSH_IS_WIFI_MANAGER (manager));

  if (phosh_wifi_manager_get_enabled (manager))
    phosh_wifi_manager_request_scan (manager);

  gtk_stack_set_visible_child_name (self->stack, "wifi_status_page");
}

static void
wwan_setting_clicked_cb (PhoshSettings *self)
{
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshWWan *wwan;
  gboolean enabled;

  g_return_if_fail (PHOSH_IS_SETTINGS (self));

  wwan = phosh_shell_get_wwan (shell);
  g_return_if_fail (PHOSH_IS_WWAN (wwan));

  enabled = phosh_wwan_is_enabled (wwan);
  phosh_wwan_set_enabled (wwan, !enabled);
}

static void
wwan_setting_long_pressed_cb (PhoshSettings *self)
{
  open_settings_panel (self, "wwan");
}


static void
on_toggle_bt_activated (GSimpleAction *action, GVariant *param, gpointer data)
{
  PhoshSettings *self = PHOSH_SETTINGS (data);
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshBtManager *manager;
  gboolean enabled;

  g_return_if_fail (PHOSH_IS_SETTINGS (self));

  manager = phosh_shell_get_bt_manager (shell);
  g_return_if_fail (PHOSH_IS_BT_MANAGER (manager));

  enabled = phosh_bt_manager_get_enabled (manager);
  phosh_bt_manager_set_enabled (manager, !enabled);
}


static void
bt_setting_long_pressed_cb (PhoshSettings *self)
{
  if (self->on_lockscreen)
    return;

  gtk_stack_set_visible_child_name (self->stack, "bt_status_page");
}


static void
feedback_setting_long_pressed_cb (PhoshSettings *self)
{
  open_settings_panel (self, "notifications");
}


static void
battery_setting_clicked_cb (PhoshSettings *self)
{
  open_settings_panel (self, "power");
}


static void
torch_setting_clicked_cb (PhoshSettings *self)
{
  PhoshShell *shell;
  PhoshTorchManager *manager;

  shell = phosh_shell_get_default ();
  g_return_if_fail (PHOSH_IS_SHELL (shell));
  manager = phosh_shell_get_torch_manager (shell);
  g_return_if_fail (PHOSH_IS_TORCH_MANAGER (manager));
  phosh_torch_manager_toggle (manager);
}


static void
docked_setting_clicked_cb (PhoshSettings *self)
{
  PhoshShell *shell;
  PhoshDockedManager *manager;
  gboolean enabled;

  shell = phosh_shell_get_default ();
  g_return_if_fail (PHOSH_IS_SHELL (shell));
  manager = phosh_shell_get_docked_manager (shell);
  g_return_if_fail (PHOSH_IS_DOCKED_MANAGER (manager));

  enabled = phosh_docked_manager_get_enabled (manager);
  phosh_docked_manager_set_enabled (manager, !enabled);
}

static void
docked_setting_long_pressed_cb (PhoshSettings *self)
{
  open_settings_panel (self, "display");
}


static void
on_vpn_setting_clicked (PhoshSettings *self)
{
  PhoshShell *shell = phosh_shell_get_default ();
  PhoshVpnManager *vpn_manager;

  g_return_if_fail (PHOSH_IS_SETTINGS (self));
  g_return_if_fail (PHOSH_IS_SHELL (shell));

  g_debug ("Toggling VPN connection");

  vpn_manager = phosh_shell_get_vpn_manager (shell);
  phosh_vpn_manager_toggle_last_connection (vpn_manager);
}


static void
on_vpn_setting_long_pressed (PhoshSettings *self)
{
  open_settings_panel (self, "network");
}


static void
on_is_headphone_changed (PhoshSettings      *self,
                         GParamSpec         *pspec,
                         PhoshAudioSettings *audio_settings)
{
  PhoshMediaPlayer *media_player;

  g_return_if_fail (PHOSH_IS_SETTINGS (self));
  g_return_if_fail (PHOSH_IS_AUDIO_SETTINGS (audio_settings));
  media_player = PHOSH_MEDIA_PLAYER (self->media_player);

  if (phosh_audio_settings_get_output_is_headphone (self->audio_settings) ||
      !phosh_media_player_get_is_playable (media_player) ||
      phosh_media_player_get_status (media_player) != PHOSH_MEDIA_PLAYER_STATUS_PLAYING) {
    return;
  }

  phosh_media_player_toggle_play_pause (media_player);
}


static void
on_quicksetting_activated (PhoshSettings   *self,
                           GtkFlowBoxChild *child,
                           GtkFlowBox      *box)

{
  GtkWidget *quick_setting;

  quick_setting = gtk_bin_get_child (GTK_BIN (child));
  gtk_button_clicked (GTK_BUTTON (quick_setting));
}

static void
unload_custom_quick_setting (GtkWidget *quick_setting)
{
  GtkWidget *flow_box_child = gtk_widget_get_parent (quick_setting);
  GtkWidget *flow_box = gtk_widget_get_parent (flow_box_child);
  gtk_container_remove (GTK_CONTAINER (flow_box), flow_box_child);
}

static void
load_custom_quick_settings (PhoshSettings *self, GSettings *settings, gchar *key)
{
  g_auto (GStrv) plugins = NULL;
  GtkWidget *widget;

  g_ptr_array_remove_range (self->custom_quick_settings, 0, self->custom_quick_settings->len);
  plugins = g_settings_get_strv (self->plugin_settings, "quick-settings");

  if (plugins == NULL)
    return;

  for (int i = 0; plugins[i]; i++) {
    g_debug ("Loading custom quick setting: %s", plugins[i]);
    widget = phosh_plugin_loader_load_plugin (self->plugin_loader, plugins[i]);

    if (widget == NULL) {
      g_warning ("Custom quick setting '%s' not found", plugins[i]);
    } else {
      gtk_container_add (GTK_CONTAINER (self->quick_settings), widget);
      g_ptr_array_add (self->custom_quick_settings, widget);
    }
  }
}

static void
on_media_player_raised (PhoshSettings *self,
                        gpointer       unused)
{
  g_return_if_fail (PHOSH_IS_SETTINGS (self));
  g_signal_emit (self, signals[SETTING_DONE], 0);
}


static void
on_notifications_clear_all_clicked (PhoshSettings *self)
{
  PhoshNotifyManager *manager;

  manager = phosh_notify_manager_get_default ();
  phosh_notify_manager_close_all_notifications (manager, PHOSH_NOTIFICATION_REASON_DISMISSED);
}


static GtkWidget *
create_notification_row (gpointer item, gpointer data)
{
  GtkWidget *row = NULL;
  GtkWidget *frame = NULL;

  row = g_object_new (GTK_TYPE_LIST_BOX_ROW,
                      "activatable", FALSE,
                      "visible", TRUE,
                      NULL);

  frame = phosh_notification_frame_new (TRUE, NULL);
  phosh_notification_frame_bind_model (PHOSH_NOTIFICATION_FRAME (frame), item);

  gtk_widget_show (frame);

  gtk_container_add (GTK_CONTAINER (row), frame);

  return row;
}


static void
on_torch_scale_value_changed (PhoshSettings *self, GtkScale *scale_torch)
{
  double value;

  g_return_if_fail (PHOSH_IS_SETTINGS (self));
  g_return_if_fail (PHOSH_IS_TORCH_MANAGER (self->torch_manager));

  /* Only react to scale changes when torch is enabled */
  if (!phosh_torch_manager_get_enabled (self->torch_manager))
      return;

  self->setting_torch = TRUE;
  value = gtk_range_get_value (GTK_RANGE (self->scale_torch));
  g_debug ("Setting torch brightness to %.2f", value);
  phosh_torch_manager_set_scaled_brightness (self->torch_manager, value / 100.0);
}


static void
on_torch_brightness_changed (PhoshSettings *self, GParamSpec *pspec, PhoshTorchManager *manager)
{
  g_return_if_fail (PHOSH_IS_SETTINGS (self));
  g_return_if_fail (PHOSH_IS_TORCH_MANAGER (manager));

  if (self->setting_torch) {
    self->setting_torch = FALSE;
    return;
  }

  gtk_range_set_value (GTK_RANGE (self->scale_torch),
                       100.0 * phosh_torch_manager_get_scaled_brightness (self->torch_manager));
}


static void
on_notification_frames_items_changed (PhoshSettings *self,
                                      guint          position,
                                      guint          removed,
                                      guint          added,
                                      GListModel    *list)
{
  gboolean is_empty;
  const char *child_name;

  g_return_if_fail (PHOSH_IS_SETTINGS (self));
  g_return_if_fail (G_IS_LIST_MODEL (list));

  is_empty = !g_list_model_get_n_items (list);
  g_debug("Notification list empty: %d", is_empty);

  child_name = is_empty ? STACK_CHILD_NO_NOTIFICATIONS : STACK_CHILD_NOTIFICATIONS;
  gtk_stack_set_visible_child_name (GTK_STACK (self->stack_notifications), child_name);
  update_drag_handle_offset (self);
}


static void
setup_brightness_range (PhoshSettings *self)
{
  gulong value_changed_handler_id;

  gtk_range_set_range (GTK_RANGE (self->scale_brightness), 0, 100);
  gtk_range_set_round_digits (GTK_RANGE (self->scale_brightness), 0);
  gtk_range_set_increments (GTK_RANGE (self->scale_brightness), 1, 10);
  value_changed_handler_id = g_signal_connect (self->scale_brightness,
                    "value-changed",
                    G_CALLBACK(brightness_value_changed_cb),
                    NULL);
  brightness_init (GTK_SCALE (self->scale_brightness), value_changed_handler_id);
}


static void
setup_torch (PhoshSettings *self)
{
  PhoshShell *shell = phosh_shell_get_default ();

  self->torch_manager = g_object_ref(phosh_shell_get_torch_manager (shell));

  gtk_range_set_range (GTK_RANGE (self->scale_torch), 40, 100);
  gtk_range_set_value (GTK_RANGE (self->scale_torch),
                       phosh_torch_manager_get_scaled_brightness (self->torch_manager) * 100.0);
  g_signal_connect_object (self->torch_manager,
                           "notify::brightness",
                           G_CALLBACK(on_torch_brightness_changed),
                           self,
                           G_CONNECT_SWAPPED);
}


static void
phosh_settings_constructed (GObject *object)
{
  PhoshSettings *self = PHOSH_SETTINGS (object);
  PhoshNotifyManager *manager;
  const char *plugin_dirs[] = { PHOSH_PLUGINS_DIR, NULL };

  G_OBJECT_CLASS (phosh_settings_parent_class)->constructed (object);

  setup_brightness_range (self);
  setup_torch (self);

  g_signal_connect (self->quick_settings,
                    "child-activated",
                    G_CALLBACK (on_quicksetting_activated),
                    self);

  manager = phosh_notify_manager_get_default ();
  gtk_list_box_bind_model (GTK_LIST_BOX (self->list_notifications),
                           G_LIST_MODEL (phosh_notify_manager_get_list (manager)),
                           create_notification_row,
                           NULL,
                           NULL);
  g_signal_connect_object (phosh_notify_manager_get_list (manager),
                           "items-changed",
                           G_CALLBACK (on_notification_frames_items_changed),
                           self,
                           G_CONNECT_SWAPPED);
  on_notification_frames_items_changed (self, -1, -1, -1,
                                        G_LIST_MODEL (phosh_notify_manager_get_list (manager)));

  g_object_bind_property (phosh_shell_get_default (),
                          "locked",
                          self,
                          "on-lockscreen",
                          G_BINDING_SYNC_CREATE);

  self->plugin_settings = g_settings_new ("sm.puri.phosh.plugins");
  self->plugin_loader = phosh_plugin_loader_new ((GStrv) plugin_dirs,
                                                 PHOSH_EXTENSION_POINT_QUICK_SETTING_WIDGET);
  self->custom_quick_settings = g_ptr_array_new_with_free_func ((GDestroyNotify) unload_custom_quick_setting);

  g_signal_connect_object (self->plugin_settings, "changed::quick-settings",
                           G_CALLBACK (load_custom_quick_settings), self, G_CONNECT_SWAPPED);

  load_custom_quick_settings (self, NULL, NULL);
}


static void
phosh_settings_dispose (GObject *object)
{
  PhoshSettings *self = PHOSH_SETTINGS (object);

  brightness_dispose ();

  g_clear_object (&self->torch_manager);

  g_clear_object (&self->plugin_settings);
  g_clear_object (&self->plugin_loader);
  if (self->custom_quick_settings) {
    g_ptr_array_free (self->custom_quick_settings, TRUE);
    self->custom_quick_settings = NULL;
  }

  G_OBJECT_CLASS (phosh_settings_parent_class)->dispose (object);
}


static void
phosh_settings_finalize (GObject *object)
{
  PhoshSettings *self = PHOSH_SETTINGS (object);

  g_clear_handle_id (&self->debounce_handle, g_source_remove);

  G_OBJECT_CLASS (phosh_settings_parent_class)->finalize (object);
}


static void
phosh_settings_class_init (PhoshSettingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = phosh_settings_dispose;
  object_class->finalize = phosh_settings_finalize;
  object_class->constructed = phosh_settings_constructed;
  object_class->set_property = phosh_settings_set_property;
  object_class->get_property = phosh_settings_get_property;

  g_type_ensure (PHOSH_TYPE_BT_STATUS_PAGE);
  g_type_ensure (PHOSH_TYPE_WIFI_STATUS_PAGE);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/settings.ui");

  /* PhoshSettings:on-lockscreen:
   *
   * Whether settings are shown on lockscreen (%TRUE) or in the unlocked shell
   * (%FALSE).
   *
   * Consider this property to be read only. It's only r/w so we can
   * use a property binding with the [type@Shell]s "locked" property.
   */
  props[PROP_ON_LOCKSCREEN] =
    g_param_spec_boolean (
      "on-lockscreen", "", "",
      FALSE,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /* PhoshSettings:handle-offset:
   *
   * The offset from the bottom of the widget where it's safe to start
   * dragging. See phosh_settings_get_drag_drag_handle_offset().
   */
  props[PROP_DRAG_HANDLE_OFFSET] =
    g_param_spec_int (
      "drag-handle-offset", "", "",
      0,
      G_MAXINT,
      0,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  signals[SETTING_DONE] = g_signal_new ("setting-done",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  g_type_ensure (PHOSH_TYPE_AUDIO_SETTINGS);

  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, audio_settings);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, box_bottom_half);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, box_sliders);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, box_settings);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, list_notifications);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, media_player);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, quick_settings);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, scale_brightness);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, scale_torch);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, stack);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, stack_notifications);
  gtk_widget_class_bind_template_child (widget_class, PhoshSettings, scrolled_window);

  gtk_widget_class_bind_template_callback (widget_class, battery_setting_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, bt_setting_long_pressed_cb);
  gtk_widget_class_bind_template_callback (widget_class, docked_setting_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, docked_setting_long_pressed_cb);
  gtk_widget_class_bind_template_callback (widget_class, feedback_setting_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, feedback_setting_long_pressed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_media_player_raised);
  gtk_widget_class_bind_template_callback (widget_class, rotation_setting_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, rotation_setting_long_pressed_cb);
  gtk_widget_class_bind_template_callback (widget_class, torch_setting_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, wifi_setting_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, wifi_setting_long_pressed_cb);
  gtk_widget_class_bind_template_callback (widget_class, wwan_setting_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, wwan_setting_long_pressed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_is_headphone_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_notifications_clear_all_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_torch_scale_value_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_vpn_setting_long_pressed);
  gtk_widget_class_bind_template_callback (widget_class, on_vpn_setting_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_stack_visible_child_changed);
  gtk_widget_class_bind_template_callback (widget_class, update_drag_handle_offset);
}


static const GActionEntry entries[] = {
  { .name = "launch-panel", .activate = on_launch_panel_activated, .parameter_type = "s" },
  { .name = "close-status-page", .activate = on_close_status_page_activated },
  { .name = "toggle-bt", .activate = on_toggle_bt_activated },
};


static void
phosh_settings_init (PhoshSettings *self)
{
  g_autoptr (GActionMap) map = NULL;
  GAction *action;

  gtk_widget_init_template (GTK_WIDGET (self));

  map = G_ACTION_MAP (g_simple_action_group_new ());
  g_action_map_add_action_entries (map,
                                   entries,
                                   G_N_ELEMENTS (entries),
                                   self);
  gtk_widget_insert_action_group (GTK_WIDGET (self),
                                  "settings",
                                  G_ACTION_GROUP (map));

  action = g_action_map_lookup_action (map, "launch-panel");
  g_object_bind_property (self, "on-lockscreen",
                          action, "enabled",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  g_signal_connect (self, "size-allocate", G_CALLBACK (on_size_allocate), NULL);
}


GtkWidget *
phosh_settings_new (void)
{
  return g_object_new (PHOSH_TYPE_SETTINGS, NULL);
}

/**
 * phosh_settings_get_drag_handle_offset:
 * @self: The settings
 *
 * Get the y coordinate from the top of the widget where dragging
 * can start. E.g. we don't want drag to work on notifications as
 * notifications need to scroll in vertical direction.
 *
 * Returns: The y coordinate at which dragging the surface can start.
 */
gint
phosh_settings_get_drag_handle_offset (PhoshSettings *self)
{
  g_return_val_if_fail (PHOSH_IS_SETTINGS (self), 0);

  return self->drag_handle_offset;
}
