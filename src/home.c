/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-home"

#include "config.h"
#include "arrow.h"
#include "overview.h"
#include "home.h"
#include "shell.h"
#include "phosh-enums.h"
#include "osk-button.h"

#include <handy.h>

#define KEYBINDINGS_SCHEMA_ID "org.gnome.shell.keybindings"
#define KEYBINDING_KEY_TOGGLE_OVERVIEW "toggle-overview"
#define KEYBINDING_KEY_TOGGLE_APPLICATION_VIEW "toggle-application-view"

/**
 * SECTION:home
 * @short_description: The home surface contains the overview and
 * the button to fold and unfold the overview.
 * @Title: PhoshHome
 *
 * #PhoshHome contains the #PhoshOverview that manages running
 * applications and the app grid. It also manages a button
 * at the bottom of the screen to fold and unfold the #PhoshOverview
 * and a button to toggle the OSK.
 */
enum {
  OSK_ACTIVATED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


enum {
  PROP_0,
  PROP_HOME_STATE,
  PROP_OSK_ENABLED,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];


struct _PhoshHome
{
  PhoshLayerSurface parent;

  GtkWidget *btn_home;
  GtkWidget *arrow_home;
  GtkWidget *btn_osk;
  GtkWidget *rev_home;
  GtkWidget *overview;

  struct {
    double progress;
    gint64 last_frame;
  } animation;

  PhoshHomeState state;

  /* Keybinding */
  GStrv           action_names;
  GSettings      *settings;

  /* osk button */
  gboolean        osk_enabled;
};
G_DEFINE_TYPE(PhoshHome, phosh_home, PHOSH_TYPE_LAYER_SURFACE);


static void
phosh_home_update_osk_button (PhoshHome *self)
{
  gboolean visible = FALSE;

  if (self->osk_enabled && self->state == PHOSH_HOME_STATE_FOLDED)
    visible = TRUE;

  gtk_widget_set_visible (self->btn_osk, visible);
}


static void
phosh_home_set_property (GObject *object,
                          guint property_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
  PhoshHome *self = PHOSH_HOME (object);

  switch (property_id) {
    case PROP_HOME_STATE:
      self->state = g_value_get_enum (value);
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HOME_STATE]);
      break;
    case PROP_OSK_ENABLED:
      self->osk_enabled = g_value_get_boolean (value);
      phosh_home_update_osk_button (self);
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_OSK_ENABLED]);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_home_get_property (GObject *object,
                          guint property_id,
                          GValue *value,
                          GParamSpec *pspec)
{
  PhoshHome *self = PHOSH_HOME (object);

  switch (property_id) {
    case PROP_HOME_STATE:
      g_value_set_enum (value, self->state);
      break;
    case PROP_OSK_ENABLED:
      g_value_set_boolean (value, self->osk_enabled);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_home_resize (PhoshHome *self)
{
  int margin;
  int height;
  double progress = hdy_ease_out_cubic (self->animation.progress);

  if (self->state == PHOSH_HOME_STATE_UNFOLDED)
    progress = 1.0 - progress;

  phosh_arrow_set_progress (PHOSH_ARROW (self->arrow_home), 1 - progress);

  g_object_get (self, "configured-height", &height, NULL);
  margin = (-height + PHOSH_HOME_BUTTON_HEIGHT) * progress;

  phosh_layer_surface_set_margins (PHOSH_LAYER_SURFACE (self), 0, 0, margin, 0);
  /* Adjust the exclusive zone since exclusive zone includes margins.
     We don't want to change the effective exclusive zone at all to
     prevent all clients from being resized. */
  phosh_layer_surface_set_exclusive_zone (PHOSH_LAYER_SURFACE (self),
                                          -margin + PHOSH_HOME_BUTTON_HEIGHT);

  phosh_layer_surface_wl_surface_commit (PHOSH_LAYER_SURFACE (self));
}


static void
home_clicked_cb (PhoshHome *self, GtkButton *btn)
{
  g_return_if_fail (PHOSH_IS_HOME (self));
  g_return_if_fail (GTK_IS_BUTTON (btn));

  phosh_home_set_state (self, !self->state);
}


static void
osk_clicked_cb (PhoshHome *self, GtkButton *btn)
{
  g_return_if_fail (PHOSH_IS_HOME (self));
  g_return_if_fail (GTK_IS_BUTTON (btn));
  g_signal_emit(self, signals[OSK_ACTIVATED], 0);
}


static void
fold_cb (PhoshHome *self, PhoshOverview *overview)
{
  g_return_if_fail (PHOSH_IS_HOME (self));
  g_return_if_fail (PHOSH_IS_OVERVIEW (overview));

  phosh_home_set_state (self, PHOSH_HOME_STATE_FOLDED);
}


static void
on_has_activities_changed (PhoshHome *self)
{
  gboolean reveal;

  g_return_if_fail (PHOSH_IS_HOME (self));

  reveal = (phosh_overview_has_running_activities (PHOSH_OVERVIEW (self->overview)) ||
            self->state == PHOSH_HOME_STATE_FOLDED);
  gtk_revealer_set_reveal_child (GTK_REVEALER (self->rev_home), reveal);
}


static gboolean
window_key_press_event_cb (PhoshHome *self, GdkEvent *event, gpointer data)
{
  gboolean ret = GDK_EVENT_PROPAGATE;
  guint keyval;
  g_return_val_if_fail (PHOSH_IS_HOME (self), GDK_EVENT_PROPAGATE);

  if (self->state != PHOSH_HOME_STATE_UNFOLDED)
    return GDK_EVENT_PROPAGATE;

  if (!gdk_event_get_keyval (event, &keyval))
    return GDK_EVENT_PROPAGATE;

  switch (keyval) {
    case GDK_KEY_Escape:
      phosh_home_set_state (self, PHOSH_HOME_STATE_FOLDED);
      ret = GDK_EVENT_STOP;
      break;
    case GDK_KEY_Return:
      ret = GDK_EVENT_PROPAGATE;
      break;
    default:
      /* Focus search when typing */
      ret = phosh_overview_handle_search (PHOSH_OVERVIEW (self->overview), event);
  }

  return ret;
}


static void
toggle_overview_action (GSimpleAction *action, GVariant *param, gpointer data)
{
  PhoshHome *self = PHOSH_HOME (data);
  PhoshHomeState state;

  g_return_if_fail (PHOSH_IS_HOME (self));

  state = self->state == PHOSH_HOME_STATE_UNFOLDED ?
    PHOSH_HOME_STATE_FOLDED : PHOSH_HOME_STATE_UNFOLDED;
  phosh_home_set_state (self, state);
}


static void
toggle_application_view_action (GSimpleAction *action, GVariant *param, gpointer data)
{
  PhoshHome *self = PHOSH_HOME (data);
  PhoshHomeState state;

  g_return_if_fail (PHOSH_IS_HOME (self));

  state = self->state == PHOSH_HOME_STATE_UNFOLDED ?
    PHOSH_HOME_STATE_FOLDED : PHOSH_HOME_STATE_UNFOLDED;
  phosh_home_set_state (self, state);
  phosh_overview_focus_app_search (PHOSH_OVERVIEW (self->overview));
}


static void
add_keybindings (PhoshHome *self)
{
  GStrv overview_bindings;
  GStrv app_view_bindings;
  GPtrArray *action_names = g_ptr_array_new ();
  g_autoptr (GSettings) settings = g_settings_new (KEYBINDINGS_SCHEMA_ID);
  g_autoptr (GArray) actions = g_array_new (FALSE, TRUE, sizeof (GActionEntry));

  overview_bindings = g_settings_get_strv (settings, KEYBINDING_KEY_TOGGLE_OVERVIEW);
  for (int i = 0; i < g_strv_length (overview_bindings); i++) {
    GActionEntry entry = { .name = overview_bindings[i], .activate = toggle_overview_action };
    g_array_append_val (actions, entry);
    g_ptr_array_add (action_names, overview_bindings[i]);
  }
  /* Free GStrv container but keep individual strings for action_names */
  g_free (overview_bindings);

  app_view_bindings = g_settings_get_strv (settings, KEYBINDING_KEY_TOGGLE_APPLICATION_VIEW);
  for (int i = 0; i < g_strv_length (app_view_bindings); i++) {
    GActionEntry entry = { .name = app_view_bindings[i], .activate = toggle_application_view_action };
    g_array_append_val (actions, entry);
    g_ptr_array_add (action_names, app_view_bindings[i]);
  }
  /* Free GStrv container but keep individual strings for action_names */
  g_free (app_view_bindings);
  g_ptr_array_add (action_names, NULL);

  phosh_shell_add_global_keyboard_action_entries (phosh_shell_get_default (),
                                                  (GActionEntry*) actions->data,
                                                  actions->len,
                                                  self);
  self->action_names = (GStrv) g_ptr_array_free (action_names, FALSE);
}


static void
on_keybindings_changed (PhoshHome *self,
                        gchar     *key,
                        GSettings *settings)
{
  /* For now just redo all keybindings */
  g_debug ("Updating keybindings");
  phosh_shell_remove_global_keyboard_action_entries (phosh_shell_get_default (),
                                                     self->action_names);
  g_clear_pointer (&self->action_names, g_strfreev);
  add_keybindings (self);
}


static void
phosh_home_constructed (GObject *object)
{
  PhoshHome *self = PHOSH_HOME (object);
  g_autoptr (GSettings) settings = NULL;

  g_signal_connect (self,
                    "configured",
                    G_CALLBACK (phosh_home_resize),
                    NULL);

  g_object_connect (self->settings,
                    "swapped-signal::changed::" KEYBINDING_KEY_TOGGLE_OVERVIEW,
                    on_keybindings_changed, self,
                    "swapped-signal::changed::" KEYBINDING_KEY_TOGGLE_APPLICATION_VIEW,
                    on_keybindings_changed, self,
                    NULL);
  add_keybindings (self);

  phosh_connect_feedback (self->btn_home);

  settings = g_settings_new ("org.gnome.desktop.a11y.applications");
  g_settings_bind (settings, "screen-keyboard-enabled",
                   self, "osk-enabled", G_SETTINGS_BIND_GET);

  G_OBJECT_CLASS (phosh_home_parent_class)->constructed (object);
}


static void
phosh_home_dispose (GObject *object)
{
  PhoshHome *self = PHOSH_HOME (object);

  g_clear_object (&self->settings);

  if (self->action_names) {
    phosh_shell_remove_global_keyboard_action_entries (phosh_shell_get_default (),
                                                       self->action_names);
    g_clear_pointer (&self->action_names, g_strfreev);
  }

  G_OBJECT_CLASS (phosh_home_parent_class)->dispose (object);
}


static void
phosh_home_class_init (PhoshHomeClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_home_constructed;
  object_class->dispose = phosh_home_dispose;

  object_class->set_property = phosh_home_set_property;
  object_class->get_property = phosh_home_get_property;

  signals[OSK_ACTIVATED] = g_signal_new ("osk-activated",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  props[PROP_HOME_STATE] =
    g_param_spec_enum ("state",
                       "Home State",
                       "The state of the home screen",
                       PHOSH_TYPE_HOME_STATE,
                       PHOSH_HOME_STATE_FOLDED,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_OSK_ENABLED] =
    g_param_spec_boolean ("osk-enabled",
                          "OSK enabled",
                          "Whether the on screen keyboard is enabled",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  g_type_ensure (PHOSH_TYPE_ARROW);
  g_type_ensure (PHOSH_TYPE_OSK_BUTTON);
  g_type_ensure (PHOSH_TYPE_OVERVIEW);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/home.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, arrow_home);
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, btn_home);
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, btn_osk);
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, overview);
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, rev_home);
  gtk_widget_class_bind_template_callback (widget_class, fold_cb);
  gtk_widget_class_bind_template_callback (widget_class, home_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_has_activities_changed);
  gtk_widget_class_bind_template_callback (widget_class, osk_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, window_key_press_event_cb);

  gtk_widget_class_set_css_name (widget_class, "phosh-home");
}


static void
phosh_home_init (PhoshHome *self)
{
  self->state = PHOSH_HOME_STATE_FOLDED;
  self->animation.progress = 1.0;
  self->settings = g_settings_new (KEYBINDINGS_SCHEMA_ID);

  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_home_new (struct zwlr_layer_shell_v1 *layer_shell,
                struct wl_output *wl_output)
{
  return g_object_new (PHOSH_TYPE_HOME,
                       "layer-shell", layer_shell,
                       "wl-output", wl_output,
                       "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_TOP,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", PHOSH_HOME_BUTTON_HEIGHT,
                       "namespace", "phosh home",
                       NULL);
}


static gboolean
animate_cb(GtkWidget *widget,
           GdkFrameClock *frame_clock,
           gpointer user_data)
{
  gint64 time;
  gboolean finished = FALSE;
  PhoshHome *self = PHOSH_HOME (widget);

  time = gdk_frame_clock_get_frame_time (frame_clock) - self->animation.last_frame;
  if (self->animation.last_frame < 0) {
    time = 0;
  }

  self->animation.progress += 0.06666 * time / 16666.00;
  self->animation.last_frame = gdk_frame_clock_get_frame_time (frame_clock);

  if (self->animation.progress >= 1.0) {
    finished = TRUE;
    self->animation.progress = 1.0;
  }

  phosh_home_resize (self);

  if (finished) {
    if (self->state == PHOSH_HOME_STATE_FOLDED)
      gtk_widget_hide (GTK_WIDGET (self->overview));
    return G_SOURCE_REMOVE;
  }

  return G_SOURCE_CONTINUE;
}


static double
reverse_ease_out_cubic (double t)
{
  return cbrt(t - 1) + 1;
}


/**
 * phosh_home_set_state:
 * @self: The home surface
 * @state: The state to set
 *
 * Set the state of the home screen. See #PhoshHomeState.
 */
void
phosh_home_set_state (PhoshHome *self, PhoshHomeState state)
{
  g_autofree char *state_name = NULL;
  gboolean enable_animations;
  gboolean kbd_interactivity;

  g_return_if_fail (PHOSH_IS_HOME (self));

  if (self->state == state)
    return;

  enable_animations = hdy_get_enable_animations (GTK_WIDGET (self));

  self->state = state;

  state_name = g_enum_to_string (PHOSH_TYPE_HOME_STATE, state);
  g_debug ("Setting state to %s", state_name);

  self->animation.last_frame = -1;
  self->animation.progress = enable_animations ? reverse_ease_out_cubic (1.0 - hdy_ease_out_cubic (self->animation.progress)) : 1.0;
  gtk_widget_add_tick_callback (GTK_WIDGET (self), animate_cb, NULL, NULL);

  if (state == PHOSH_HOME_STATE_UNFOLDED) {
    kbd_interactivity = TRUE;
    phosh_overview_reset (PHOSH_OVERVIEW (self->overview));
    gtk_widget_show (GTK_WIDGET (self->overview));
  } else {
    kbd_interactivity = FALSE;
  }
  phosh_home_update_osk_button (self);
  phosh_layer_surface_set_kbd_interactivity (PHOSH_LAYER_SURFACE (self), kbd_interactivity);

  on_has_activities_changed (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HOME_STATE]);
}


PhoshOverview*
phosh_home_get_overview (PhoshHome *self)
{
  g_return_val_if_fail (PHOSH_IS_HOME (self), NULL);

  return PHOSH_OVERVIEW (self->overview);
}
