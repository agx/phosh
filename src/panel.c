/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Somewhat based on maynard's panel which is
 * Copyright (C) 2014 Collabora Ltd. *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#define G_LOG_DOMAIN "phosh-panel"

#include "config.h"

#include "panel.h"
#include "shell.h"
#include "session.h"
#include "settings.h"
#include "util.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-wall-clock.h>
#include <libgnome-desktop/gnome-xkb-info.h>

#include <glib/gi18n.h>

enum {
  SETTINGS_ACTIVATED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

typedef struct {
  PhoshPanelState state;

  GtkWidget *stack;
  GtkWidget *box;            /* main content box */
  GtkWidget *btn_top_panel;
  GtkWidget *lbl_clock;
  GtkWidget *lbl_lang;
  GtkWidget *settings;       /* settings menu */

  GnomeWallClock *wall_clock;
  GnomeXkbInfo *xkbinfo;
  GSettings *input_settings;
  GdkSeat *seat;

  GSimpleActionGroup *actions;
} PhoshPanelPrivate;

typedef struct _PhoshPanel
{
  PhoshLayerSurface parent;
} PhoshPanel;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshPanel, phosh_panel, PHOSH_TYPE_LAYER_SURFACE)


static void
on_shutdown_action (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer      data)
{
  PhoshPanel *self = PHOSH_PANEL(data);

  g_return_if_fail (PHOSH_IS_PANEL (self));
  phosh_session_shutdown ();
  /* TODO: Since we don't implement
   * gnome.SessionManager.EndSessionDialog yet */
  phosh_session_shutdown ();
  phosh_panel_fold (self);
}


static void
on_lockscreen_action (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer      data)
{
  PhoshPanel *self = PHOSH_PANEL(data);

  g_return_if_fail (PHOSH_IS_PANEL (self));
  phosh_shell_lock (phosh_shell_get_default ());
  phosh_panel_fold (self);
}


static void
on_logout_action (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer      data)
{
  PhoshPanel *self = PHOSH_PANEL(data);

  g_return_if_fail (PHOSH_IS_PANEL (self));
  phosh_session_logout ();
  phosh_panel_fold (self);
}


static void
top_panel_clicked_cb (PhoshPanel *self, GtkButton *btn)
{
  g_return_if_fail (PHOSH_IS_PANEL (self));
  g_return_if_fail (GTK_IS_BUTTON (btn));
  g_signal_emit(self, signals[SETTINGS_ACTIVATED], 0);
}


static void
wall_clock_notify_cb (PhoshPanel *self,
                      GParamSpec *pspec,
                      GnomeWallClock *wall_clock)
{
  PhoshPanelPrivate *priv = phosh_panel_get_instance_private (self);
  const gchar *str;

  g_return_if_fail (PHOSH_IS_PANEL (self));
  g_return_if_fail (GNOME_IS_WALL_CLOCK (wall_clock));

  str = gnome_wall_clock_get_clock(wall_clock);
  gtk_label_set_text (GTK_LABEL (priv->lbl_clock), str);
}


static gboolean
needs_keyboard_label (PhoshPanel *self)
{
  PhoshPanelPrivate *priv;
  GList *slaves;
  g_autoptr(GVariant) sources = NULL;

  priv = phosh_panel_get_instance_private (self);
  g_return_val_if_fail (GDK_IS_SEAT (priv->seat), FALSE);
  g_return_val_if_fail (G_IS_SETTINGS (priv->input_settings), FALSE);

  sources = g_settings_get_value(priv->input_settings, "sources");
  if (g_variant_n_children (sources) < 2)
    return FALSE;

  slaves = gdk_seat_get_slaves (priv->seat, GDK_SEAT_CAPABILITY_KEYBOARD);
  if (!slaves)
    return FALSE;

  g_list_free (slaves);
  return TRUE;
}


static void
on_seat_device_changed (PhoshPanel *self, GdkDevice  *device, GdkSeat *seat)
{
  gboolean visible;
  PhoshPanelPrivate *priv;

  g_return_if_fail (PHOSH_IS_PANEL (self));
  g_return_if_fail (GDK_IS_SEAT (seat));

  priv = phosh_panel_get_instance_private (self);
  visible = needs_keyboard_label (self);
  gtk_widget_set_visible (priv->lbl_lang, visible);
}


static void
on_input_setting_changed (PhoshPanel  *self,
                          const gchar *key,
                          GSettings   *settings)
{
  PhoshPanelPrivate *priv = phosh_panel_get_instance_private (self);
  g_autoptr(GVariant) sources = NULL;
  GVariantIter iter;
  g_autofree gchar *id = NULL;
  g_autofree gchar *type = NULL;
  const gchar *name;

  if (!needs_keyboard_label (self)) {
    gtk_widget_hide (priv->lbl_lang);
    return;
  }

  sources = g_settings_get_value(settings, "sources");
  g_variant_iter_init (&iter, sources);
  g_variant_iter_next (&iter, "(ss)", &type, &id);

  if (g_strcmp0 (type, "xkb")) {
    g_debug ("Not a xkb layout: '%s' - ignoring", id);
    return;
  }

  if (!gnome_xkb_info_get_layout_info (priv->xkbinfo, id,
                                       NULL, &name, NULL, NULL)) {
    g_debug ("Failed to get layout info for %s", id);
    name = id;
  }
  g_debug ("Layout is %s", name);
  gtk_label_set_text (GTK_LABEL (priv->lbl_lang), name);
  gtk_widget_show (priv->lbl_lang);
}

static gboolean
on_key_press_event (PhoshPanel *self, GdkEventKey *event, gpointer data)
{
  gboolean handled = FALSE;
  PhoshPanelPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_PANEL (self), FALSE);
  priv = phosh_panel_get_instance_private (self);

  if (!priv->settings)
    return handled;

  switch (event->keyval) {
    case GDK_KEY_Escape:
      phosh_panel_fold (self);
      handled = TRUE;
      break;
    default:
      /* nothing to do */
      break;
    }
  return handled;
}

static gboolean
on_button_press_event (PhoshPanel *self, GdkEventKey *event, gpointer data)
{
  phosh_trigger_feedback ("button-pressed");
  phosh_panel_fold (self);
  return FALSE;
}

static GActionEntry entries[] = {
  { "poweroff", on_shutdown_action, NULL, NULL, NULL },
  { "lockscreen", on_lockscreen_action, NULL, NULL, NULL },
  { "logout", on_logout_action, NULL, NULL, NULL },
};

static void
phosh_panel_constructed (GObject *object)
{
  PhoshPanel *self = PHOSH_PANEL (object);
  PhoshPanelPrivate *priv = phosh_panel_get_instance_private (self);
  GdkDisplay *display = gdk_display_get_default ();

  G_OBJECT_CLASS (phosh_panel_parent_class)->constructed (object);

  priv->state = PHOSH_PANEL_STATE_FOLDED;
  priv->wall_clock = gnome_wall_clock_new ();

  g_signal_connect_object (priv->wall_clock,
                           "notify::clock",
                           G_CALLBACK (wall_clock_notify_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (priv->btn_top_panel,
                           "clicked",
                           G_CALLBACK (top_panel_clicked_cb),
                           self,
                           G_CONNECT_SWAPPED);

  phosh_connect_button_feedback (GTK_BUTTON (priv->btn_top_panel));

  gtk_window_set_title (GTK_WINDOW (self), "phosh panel");
  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "phosh-panel");

  /* Button properites */
  gtk_style_context_remove_class (gtk_widget_get_style_context (priv->btn_top_panel),
                                  "button");
  gtk_style_context_remove_class (gtk_widget_get_style_context (priv->btn_top_panel),
                                  "image-button");

  wall_clock_notify_cb (self, NULL, priv->wall_clock);

  /* language indicator */
  if (display) {
    priv->input_settings = g_settings_new ("org.gnome.desktop.input-sources");
    priv->xkbinfo = gnome_xkb_info_new ();
    priv->seat = gdk_display_get_default_seat (display);
    g_object_connect (priv->seat,
                      "swapped_signal::device-added", G_CALLBACK (on_seat_device_changed), self,
                      "swapped_signal::device-removed", G_CALLBACK (on_seat_device_changed), self,
                      NULL);
    g_signal_connect_swapped (priv->input_settings,
                              "changed::sources", G_CALLBACK (on_input_setting_changed),
                              self);
    on_input_setting_changed (self, NULL, priv->input_settings);
  }

  /* Settings menu and it's top-bar / menu */
  gtk_widget_add_events (GTK_WIDGET (self), GDK_KEY_PRESS_MASK);
  g_signal_connect (G_OBJECT (self),
                    "key-press-event",
                    G_CALLBACK (on_key_press_event),
                    NULL);
  g_signal_connect (G_OBJECT (self),
                    "button-press-event",
                    G_CALLBACK (on_button_press_event),
                    NULL);

  priv->actions = g_simple_action_group_new ();
  gtk_widget_insert_action_group (GTK_WIDGET (self), "panel",
                                  G_ACTION_GROUP (priv->actions));
  g_action_map_add_action_entries (G_ACTION_MAP (priv->actions),
                                   entries, G_N_ELEMENTS (entries),
                                   self);
  if (!phosh_shell_started_by_display_manager (phosh_shell_get_default ())) {
    GAction *action = g_action_map_lookup_action (G_ACTION_MAP (priv->actions),
                                                  "logout");
    g_simple_action_set_enabled (G_SIMPLE_ACTION(action), FALSE);
  }
}


static void
phosh_panel_dispose (GObject *object)
{
  PhoshPanel *self = PHOSH_PANEL (object);
  PhoshPanelPrivate *priv = phosh_panel_get_instance_private (self);

  g_clear_object (&priv->wall_clock);
  g_clear_object (&priv->xkbinfo);
  g_clear_object (&priv->input_settings);
  g_clear_object (&priv->actions);
  priv->seat = NULL;

  G_OBJECT_CLASS (phosh_panel_parent_class)->dispose (object);
}

static void
phosh_panel_class_init (PhoshPanelClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_panel_constructed;
  object_class->dispose = phosh_panel_dispose;

  signals[SETTINGS_ACTIVATED] = g_signal_new ("settings-activated",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  g_type_ensure (PHOSH_TYPE_SETTINGS);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/top-panel.ui");
  gtk_widget_class_bind_template_child_private (widget_class, PhoshPanel, btn_top_panel);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshPanel, lbl_clock);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshPanel, lbl_lang);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshPanel, box);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshPanel, stack);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshPanel, settings);
}


static void
phosh_panel_init (PhoshPanel *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_panel_new (struct zwlr_layer_shell_v1 *layer_shell,
                 struct wl_output *wl_output)
{
  return g_object_new (PHOSH_TYPE_PANEL,
                       "layer-shell", layer_shell,
                       "wl-output", wl_output,
                       "height", PHOSH_PANEL_HEIGHT,
                       "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_TOP,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", PHOSH_PANEL_HEIGHT,
                       "namespace", "phosh",
                       NULL);
}

void
phosh_panel_fold (PhoshPanel *self)
{
  PhoshPanelPrivate *priv;
  gint width;

  g_return_if_fail (PHOSH_IS_PANEL (self));
  priv = phosh_panel_get_instance_private (self);

  if (priv->state == PHOSH_PANEL_STATE_FOLDED)
	return;

  gtk_stack_set_transition_type (GTK_STACK (priv->stack), GTK_STACK_TRANSITION_TYPE_SLIDE_UP);
  gtk_stack_set_visible_child_name (GTK_STACK (priv->stack), "topbar");
  gtk_widget_hide (priv->settings);
  phosh_layer_surface_set_kbd_interactivity (PHOSH_LAYER_SURFACE (self), FALSE);
  gtk_window_get_size (GTK_WINDOW (self), &width, NULL);
  gtk_window_resize (GTK_WINDOW (self), width, PHOSH_PANEL_HEIGHT);
  priv->state = PHOSH_PANEL_STATE_FOLDED;
}

void
phosh_panel_unfold (PhoshPanel *self)
{
  PhoshPanelPrivate *priv;

  g_return_if_fail (PHOSH_IS_PANEL (self));
  priv = phosh_panel_get_instance_private (self);

  if (priv->state == PHOSH_PANEL_STATE_UNFOLDED)
	return;

  phosh_layer_surface_set_kbd_interactivity (PHOSH_LAYER_SURFACE (self), TRUE);
  gtk_widget_show (priv->settings);
  gtk_stack_set_transition_type (GTK_STACK (priv->stack), GTK_STACK_TRANSITION_TYPE_SLIDE_DOWN);
  gtk_stack_set_visible_child_name(GTK_STACK (priv->stack), "settings");
  g_signal_connect_swapped (priv->settings,
                            "setting-done",
                            G_CALLBACK(phosh_panel_fold),
                            self);
  priv->state =PHOSH_PANEL_STATE_UNFOLDED;
}

void
phosh_panel_toggle_fold (PhoshPanel *self)
{
  PhoshPanelPrivate *priv;
  g_return_if_fail (PHOSH_IS_PANEL (self));

  priv = phosh_panel_get_instance_private (self);
  if (priv->state == PHOSH_PANEL_STATE_UNFOLDED) {
    phosh_panel_fold (self);
  } else {
    phosh_panel_unfold (self);
  }
}
