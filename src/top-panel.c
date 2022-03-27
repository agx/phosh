/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Somewhat based on maynard's panel which is
 * Copyright (C) 2014 Collabora Ltd. *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#define G_LOG_DOMAIN "phosh-top-panel"

#include "config.h"

#include "shell.h"
#include "session-manager.h"
#include "settings.h"
#include "top-panel.h"
#include "util.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-wall-clock.h>
#include <libgnome-desktop/gnome-xkb-info.h>

#include <glib/gi18n.h>

#define KEYBINDINGS_SCHEMA_ID "org.gnome.shell.keybindings"
#define KEYBINDING_KEY_TOGGLE_MESSAGE_TRAY "toggle-message-tray"

/**
 * SECTION:top-panel
 * @short_description: The top panel
 * @Title: PhoshTopPanel
 *
 * The top panel containing the clock and status indicators. When activated
 * int unfolds the #PhoshSettings.
 */

enum {
  SETTINGS_ACTIVATED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

typedef struct _PhoshTopPanel {
  PhoshLayerSurface parent;

  PhoshTopPanelState state;

  GtkWidget *btn_power;
  GtkWidget *menu_power;
  GtkWidget *stack;
  GtkWidget *box;            /* main content box */
  GtkWidget *btn_top_panel;
  GtkWidget *lbl_clock;
  GtkWidget *lbl_lang;
  GtkWidget *settings;       /* settings menu */
  GtkWidget *batteryinfo;

  GnomeWallClock *wall_clock;
  GnomeXkbInfo *xkbinfo;
  GSettings *input_settings;
  GSettings *interface_settings;
  GdkSeat *seat;

  GSimpleActionGroup *actions;

  /* Keybinding */
  GStrv           action_names;
  GSettings      *kb_settings;

  GtkGesture     *click_gesture; /* needed so that the gesture isn't destroyed immediately */
} PhoshTopPanel;

G_DEFINE_TYPE (PhoshTopPanel, phosh_top_panel, PHOSH_TYPE_LAYER_SURFACE)


static void
on_shutdown_action (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       data)
{
  PhoshTopPanel *self = PHOSH_TOP_PANEL(data);
  PhoshSessionManager *sm = phosh_shell_get_session_manager (phosh_shell_get_default ());

  g_return_if_fail (PHOSH_IS_TOP_PANEL (self));
  phosh_session_manager_shutdown (sm);
  phosh_top_panel_fold (self);
}


static void
on_restart_action (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       data)
{
  PhoshTopPanel *self = PHOSH_TOP_PANEL(data);
  PhoshSessionManager *sm = phosh_shell_get_session_manager (phosh_shell_get_default ());

  g_return_if_fail (PHOSH_IS_TOP_PANEL (self));
  g_return_if_fail (PHOSH_IS_SESSION_MANAGER (sm));

  phosh_session_manager_reboot (sm);
  phosh_top_panel_fold (self);
}


static void
on_lockscreen_action (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer      data)
{
  PhoshTopPanel *self = PHOSH_TOP_PANEL(data);

  g_return_if_fail (PHOSH_IS_TOP_PANEL (self));
  phosh_shell_lock (phosh_shell_get_default ());
  phosh_top_panel_fold (self);
}


static void
on_logout_action (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer      data)
{
  PhoshTopPanel *self = PHOSH_TOP_PANEL(data);
  PhoshSessionManager *sm = phosh_shell_get_session_manager (phosh_shell_get_default ());

  g_return_if_fail (PHOSH_IS_TOP_PANEL (self));
  g_return_if_fail (PHOSH_IS_SESSION_MANAGER (sm));
  phosh_session_manager_logout (sm);
  phosh_top_panel_fold (self);
}


static void
top_panel_clicked_cb (PhoshTopPanel *self, GtkButton *btn)
{
  g_return_if_fail (PHOSH_IS_TOP_PANEL (self));
  g_return_if_fail (GTK_IS_BUTTON (btn));

  if (phosh_shell_get_locked (phosh_shell_get_default ())) {
    return;
  }

  g_signal_emit(self, signals[SETTINGS_ACTIVATED], 0);
}


static void
wall_clock_notify_cb (PhoshTopPanel  *self,
                      GParamSpec     *pspec,
                      GnomeWallClock *wall_clock)
{
  const char *str;

  g_return_if_fail (PHOSH_IS_TOP_PANEL (self));
  g_return_if_fail (GNOME_IS_WALL_CLOCK (wall_clock));

  str = gnome_wall_clock_get_clock(wall_clock);
  gtk_label_set_text (GTK_LABEL (self->lbl_clock), str);
}


static gboolean
needs_keyboard_label (PhoshTopPanel *self)
{
  GList *slaves;
  g_autoptr(GVariant) sources = NULL;

  g_return_val_if_fail (GDK_IS_SEAT (self->seat), FALSE);
  g_return_val_if_fail (G_IS_SETTINGS (self->input_settings), FALSE);

  sources = g_settings_get_value(self->input_settings, "sources");
  if (g_variant_n_children (sources) < 2)
    return FALSE;

  slaves = gdk_seat_get_slaves (self->seat, GDK_SEAT_CAPABILITY_KEYBOARD);
  if (!slaves)
    return FALSE;

  g_list_free (slaves);
  return TRUE;
}


static void
on_seat_device_changed (PhoshTopPanel *self, GdkDevice  *device, GdkSeat *seat)
{
  gboolean visible;

  g_return_if_fail (PHOSH_IS_TOP_PANEL (self));
  g_return_if_fail (GDK_IS_SEAT (seat));

  visible = needs_keyboard_label (self);
  gtk_widget_set_visible (self->lbl_lang, visible);
}


static void
on_input_setting_changed (PhoshTopPanel *self,
                          const char    *key,
                          GSettings     *settings)
{
  g_autoptr(GVariant) sources = NULL;
  GVariantIter iter;
  g_autofree char *id = NULL;
  g_autofree char *type = NULL;
  const char *name;

  if (!needs_keyboard_label (self)) {
    gtk_widget_hide (self->lbl_lang);
    return;
  }

  sources = g_settings_get_value(settings, "sources");
  g_variant_iter_init (&iter, sources);
  g_variant_iter_next (&iter, "(ss)", &type, &id);

  if (g_strcmp0 (type, "xkb")) {
    g_debug ("Not a xkb layout: '%s' - ignoring", id);
    return;
  }

  if (!gnome_xkb_info_get_layout_info (self->xkbinfo, id,
                                       NULL, &name, NULL, NULL)) {
    g_debug ("Failed to get layout info for %s", id);
    name = id;
  }
  g_debug ("Layout is %s", name);
  gtk_label_set_text (GTK_LABEL (self->lbl_lang), name);
  gtk_widget_show (self->lbl_lang);
}


static gboolean
on_key_press_event (PhoshTopPanel *self, GdkEventKey *event, gpointer data)
{
  gboolean handled = FALSE;

  g_return_val_if_fail (PHOSH_IS_TOP_PANEL (self), FALSE);

  if (!self->settings)
    return handled;

  switch (event->keyval) {
    case GDK_KEY_Escape:
      phosh_top_panel_fold (self);
      handled = TRUE;
      break;
    default:
      /* nothing to do */
      break;
    }
  return handled;
}


static void
released_cb (PhoshTopPanel *self)
{
  phosh_trigger_feedback ("button-released");

  /*
   * The popover has to be popdown manually as it doesn't happen
   * automatically when the power button is tapped with touch
   */
  if (gtk_widget_is_visible (self->menu_power))
    gtk_popover_popdown (GTK_POPOVER (self->menu_power));
  else
    phosh_top_panel_fold (self);

}


static void
toggle_message_tray_action (GSimpleAction *action, GVariant *param, gpointer data)
{
  PhoshTopPanel *self = PHOSH_TOP_PANEL (data);

  g_return_if_fail (PHOSH_IS_TOP_PANEL (self));

  phosh_top_panel_toggle_fold (self);
  /* TODO: focus message tray when */
}


static void
add_keybindings (PhoshTopPanel *self)
{
  g_auto (GStrv) keybindings = NULL;
  g_autoptr (GArray) actions = g_array_new (FALSE, TRUE, sizeof (GActionEntry));

  keybindings = g_settings_get_strv (self->kb_settings, KEYBINDING_KEY_TOGGLE_MESSAGE_TRAY);
  for (int i = 0; i < g_strv_length (keybindings); i++) {
    GActionEntry entry = { .name = keybindings[i], .activate = toggle_message_tray_action };
    g_array_append_val (actions, entry);
  }
  phosh_shell_add_global_keyboard_action_entries (phosh_shell_get_default (),
                                                  (GActionEntry*) actions->data,
                                                  actions->len,
                                                  self);
  self->action_names = g_steal_pointer (&keybindings);
}


static void
on_keybindings_changed (PhoshTopPanel *self,
                        gchar         *key,
                        GSettings     *settings)
{
  /* For now just redo all keybindings */
  g_debug ("Updating keybindings");
  phosh_shell_remove_global_keyboard_action_entries (phosh_shell_get_default (),
                                                     self->action_names);
  g_clear_pointer (&self->action_names, g_strfreev);
  add_keybindings (self);
}



static GActionEntry entries[] = {
  { .name = "poweroff", .activate = on_shutdown_action },
  { .name = "restart", .activate = on_restart_action },
  { .name = "lockscreen", .activate = on_lockscreen_action },
  { .name = "logout", .activate = on_logout_action },
};


static void
phosh_top_panel_constructed (GObject *object)
{
  PhoshTopPanel *self = PHOSH_TOP_PANEL (object);
  GdkDisplay *display = gdk_display_get_default ();

  G_OBJECT_CLASS (phosh_top_panel_parent_class)->constructed (object);

  self->state = PHOSH_TOP_PANEL_STATE_FOLDED;
  self->wall_clock = gnome_wall_clock_new ();

  g_signal_connect_object (self->wall_clock,
                           "notify::clock",
                           G_CALLBACK (wall_clock_notify_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->btn_top_panel,
                           "clicked",
                           G_CALLBACK (top_panel_clicked_cb),
                           self,
                           G_CONNECT_SWAPPED);

  phosh_connect_feedback (self->btn_top_panel);

  gtk_window_set_title (GTK_WINDOW (self), "phosh panel");

  /* Button properties */
  gtk_style_context_remove_class (gtk_widget_get_style_context (self->btn_top_panel),
                                  "button");
  gtk_style_context_remove_class (gtk_widget_get_style_context (self->btn_top_panel),
                                  "image-button");

  wall_clock_notify_cb (self, NULL, self->wall_clock);

  /* language indicator */
  if (display) {
    self->input_settings = g_settings_new ("org.gnome.desktop.input-sources");
    self->xkbinfo = gnome_xkb_info_new ();
    self->seat = gdk_display_get_default_seat (display);
    g_object_connect (self->seat,
                      "swapped_signal::device-added", G_CALLBACK (on_seat_device_changed), self,
                      "swapped_signal::device-removed", G_CALLBACK (on_seat_device_changed), self,
                      NULL);
    g_signal_connect_swapped (self->input_settings,
                              "changed::sources", G_CALLBACK (on_input_setting_changed),
                              self);
    on_input_setting_changed (self, NULL, self->input_settings);
  }

  /* Settings menu and it's top-bar / menu */
  gtk_widget_add_events (GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);
  g_signal_connect (G_OBJECT (self),
                    "key-press-event",
                    G_CALLBACK (on_key_press_event),
                    NULL);
  g_signal_connect_swapped (self->settings,
                            "setting-done",
                            G_CALLBACK (phosh_top_panel_fold),
                            self);

  self->actions = g_simple_action_group_new ();
  gtk_widget_insert_action_group (GTK_WIDGET (self), "panel",
                                  G_ACTION_GROUP (self->actions));
  g_action_map_add_action_entries (G_ACTION_MAP (self->actions),
                                   entries, G_N_ELEMENTS (entries),
                                   self);
  if (!phosh_shell_started_by_display_manager (phosh_shell_get_default ())) {
    GAction *action = g_action_map_lookup_action (G_ACTION_MAP (self->actions),
                                                  "logout");
    g_simple_action_set_enabled (G_SIMPLE_ACTION(action), FALSE);
  }

  self->interface_settings = g_settings_new ("org.gnome.desktop.interface");
  g_settings_bind (self->interface_settings,
                   "show-battery-percentage",
                   self->batteryinfo,
                   "show-detail",
                   G_SETTINGS_BIND_GET);

  g_signal_connect_swapped (self->kb_settings,
                            "changed::" KEYBINDING_KEY_TOGGLE_MESSAGE_TRAY,
                            G_CALLBACK (on_keybindings_changed),
                            self);
  add_keybindings (self);
}


static void
phosh_top_panel_dispose (GObject *object)
{
  PhoshTopPanel *self = PHOSH_TOP_PANEL (object);

  g_clear_object (&self->kb_settings);
  g_clear_object (&self->wall_clock);
  g_clear_object (&self->xkbinfo);
  g_clear_object (&self->input_settings);
  g_clear_object (&self->interface_settings);
  g_clear_object (&self->actions);
  g_clear_pointer (&self->action_names, g_strfreev);
  self->seat = NULL;

  G_OBJECT_CLASS (phosh_top_panel_parent_class)->dispose (object);
}


static void
phosh_top_panel_class_init (PhoshTopPanelClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_top_panel_constructed;
  object_class->dispose = phosh_top_panel_dispose;

  gtk_widget_class_set_css_name (widget_class, "phosh-top-panel");

  signals[SETTINGS_ACTIVATED] = g_signal_new ("settings-activated",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/top-panel.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshTopPanel, btn_power);
  gtk_widget_class_bind_template_child (widget_class, PhoshTopPanel, menu_power);
  gtk_widget_class_bind_template_child (widget_class, PhoshTopPanel, btn_top_panel);
  gtk_widget_class_bind_template_child (widget_class, PhoshTopPanel, batteryinfo);
  gtk_widget_class_bind_template_child (widget_class, PhoshTopPanel, lbl_clock);
  gtk_widget_class_bind_template_child (widget_class, PhoshTopPanel, lbl_lang);
  gtk_widget_class_bind_template_child (widget_class, PhoshTopPanel, box);
  gtk_widget_class_bind_template_child (widget_class, PhoshTopPanel, stack);
  gtk_widget_class_bind_template_child (widget_class, PhoshTopPanel, settings);
  gtk_widget_class_bind_template_child (widget_class, PhoshTopPanel, click_gesture);
  gtk_widget_class_bind_template_callback (widget_class, released_cb);
}


static void
phosh_top_panel_init (PhoshTopPanel *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->kb_settings = g_settings_new (KEYBINDINGS_SCHEMA_ID);
}


GtkWidget *
phosh_top_panel_new (struct zwlr_layer_shell_v1 *layer_shell,
                     struct wl_output           *wl_output)
{
  return g_object_new (PHOSH_TYPE_TOP_PANEL,
                       "layer-shell", layer_shell,
                       "wl-output", wl_output,
                       "height", PHOSH_TOP_PANEL_HEIGHT,
                       "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", PHOSH_TOP_PANEL_HEIGHT,
                       "namespace", "phosh",
                       NULL);
}


void
phosh_top_panel_fold (PhoshTopPanel *self)
{
  int width;

  g_return_if_fail (PHOSH_IS_TOP_PANEL (self));

  if (self->state == PHOSH_TOP_PANEL_STATE_FOLDED)
	return;

  gtk_widget_hide (self->menu_power);
  gtk_stack_set_transition_type (GTK_STACK (self->stack), GTK_STACK_TRANSITION_TYPE_SLIDE_UP);
  gtk_stack_set_visible_child_name (GTK_STACK (self->stack), "topbar");
  gtk_widget_hide (self->settings);
  phosh_layer_surface_set_kbd_interactivity (PHOSH_LAYER_SURFACE (self), FALSE);
  gtk_window_get_size (GTK_WINDOW (self), &width, NULL);
  gtk_window_resize (GTK_WINDOW (self), width, PHOSH_TOP_PANEL_HEIGHT);
  self->state = PHOSH_TOP_PANEL_STATE_FOLDED;
}


void
phosh_top_panel_unfold (PhoshTopPanel *self)
{
  g_return_if_fail (PHOSH_IS_TOP_PANEL (self));

  if (self->state == PHOSH_TOP_PANEL_STATE_UNFOLDED)
	return;

  phosh_layer_surface_set_kbd_interactivity (PHOSH_LAYER_SURFACE (self), TRUE);
  gtk_widget_show (self->settings);
  gtk_stack_set_transition_type (GTK_STACK (self->stack), GTK_STACK_TRANSITION_TYPE_SLIDE_DOWN);
  gtk_stack_set_visible_child_name(GTK_STACK (self->stack), "settings");
  self->state =PHOSH_TOP_PANEL_STATE_UNFOLDED;
}


void
phosh_top_panel_toggle_fold (PhoshTopPanel *self)
{
  g_return_if_fail (PHOSH_IS_TOP_PANEL (self));

  if (self->state == PHOSH_TOP_PANEL_STATE_UNFOLDED) {
    phosh_top_panel_fold (self);
  } else {
    phosh_top_panel_unfold (self);
  }
}


PhoshTopPanelState
phosh_top_panel_get_state (PhoshTopPanel *self)
{
  g_return_val_if_fail (PHOSH_IS_TOP_PANEL (self), PHOSH_TOP_PANEL_STATE_FOLDED);

  return self->state;
}
