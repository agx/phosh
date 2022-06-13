/*
 * Copyright (C) 2018-2022 Purism SPC
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
#include "util.h"

#include <handy.h>

#define KEYBINDINGS_SCHEMA_ID "org.gnome.shell.keybindings"
#define KEYBINDING_KEY_TOGGLE_OVERVIEW "toggle-overview"
#define KEYBINDING_KEY_TOGGLE_APPLICATION_VIEW "toggle-application-view"

#define PHOSH_HOME_DRAG_THRESHOLD 0.3

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
  PhoshDragSurface parent;

  GtkWidget *arrow_home;
  GtkWidget *btn_osk;
  GtkWidget *overview;
  guint      debounce_handle;

  PhoshHomeState state;

  /* Keybinding */
  GStrv           action_names;
  GSettings      *settings;

  /* osk button */
  gboolean        osk_enabled;

  GtkGesture     *click_gesture; /* needed so that the gesture isn't destroyed immediately */
};
G_DEFINE_TYPE(PhoshHome, phosh_home, PHOSH_TYPE_DRAG_SURFACE);


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
update_drag_handle (PhoshHome *self, gboolean commit)
{
  gboolean success;
  gint handle = 0;
  PhoshAppGrid *app_grid;
  gboolean arrow_visible = TRUE;
  PhoshDragSurfaceDragMode drag_mode = PHOSH_DRAG_SURFACE_DRAG_MODE_HANDLE;

  /* Update the handle's arrow and dragability */
  if (phosh_overview_has_running_activities (PHOSH_OVERVIEW (self->overview)) == FALSE &&
    self->state == PHOSH_HOME_STATE_UNFOLDED) {
    arrow_visible = FALSE;
    drag_mode = PHOSH_DRAG_SURFACE_DRAG_MODE_NONE;
  }
  gtk_widget_set_visible (GTK_WIDGET (self->arrow_home), arrow_visible);
  phosh_drag_surface_set_drag_mode (PHOSH_DRAG_SURFACE (self), drag_mode);

  /* Update hande size */
  app_grid = phosh_overview_get_app_grid (PHOSH_OVERVIEW (self->overview));
  success = gtk_widget_translate_coordinates (GTK_WIDGET (app_grid),
                                              GTK_WIDGET (self),
                                              0, 0, NULL, &handle);
  if (!success) {
    g_warning ("Failed to get handle position");
    handle = PHOSH_HOME_BUTTON_HEIGHT;
  }

  g_debug ("Drag Handle: %d", handle);
  phosh_drag_surface_set_drag_handle (PHOSH_DRAG_SURFACE (self), handle);
  if (commit)
    phosh_layer_surface_wl_surface_commit (PHOSH_LAYER_SURFACE (self));
}


static int
get_margin (gint height)
{
  return (-1 * height) + PHOSH_HOME_BUTTON_HEIGHT;
}


static gboolean
on_configure_event (PhoshHome *self, GdkEventConfigure *event)
{
  guint margin;

  margin = get_margin (event->height);

  /* ignore popovers like the power menu */
  if (gtk_widget_get_window (GTK_WIDGET (self)) != event->window)
    return FALSE;

  g_debug ("%s: %dx%d,  margin: %d", __func__, event->height, event->width, margin);

  /* If the size changes we need to update the folded margin */
  phosh_drag_surface_set_margin (PHOSH_DRAG_SURFACE (self), margin, 0);
  /* Update drag handle since overview size might have changed */
  update_drag_handle (self, FALSE);
  phosh_layer_surface_wl_surface_commit (PHOSH_LAYER_SURFACE (self));

  return FALSE;
}


static void
on_home_released (GtkButton *button, int n_press, double x, double y, GtkGestureMultiPress *gesture)
{
  PhoshHome *self = g_object_get_data (G_OBJECT (gesture), "phosh-home");

  g_return_if_fail (PHOSH_IS_HOME (self));

  if (phosh_util_gesture_is_touch (GTK_GESTURE_SINGLE (gesture)) == FALSE)
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


static gboolean
delayed_handle_resize (gpointer data)
{
  PhoshHome *self = PHOSH_HOME (data);

  self->debounce_handle = 0;
  update_drag_handle (self, TRUE);
  return G_SOURCE_REMOVE;
}


static void
on_has_activities_changed (PhoshHome *self)
{
  g_return_if_fail (PHOSH_IS_HOME (self));

  if (phosh_overview_has_running_activities(PHOSH_OVERVIEW (self->overview)) == FALSE)
    phosh_home_set_state (self, PHOSH_HOME_STATE_UNFOLDED);

  /* TODO: we need to debounce the handle resize a little until all
     the queued resizing is done, would be nicer to have that tied to
     a signal */
  self->debounce_handle = g_timeout_add (200, delayed_handle_resize, self);
  g_source_set_name_by_id (self->debounce_handle, "[phosh] delayed_handle_resize");
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
phosh_home_dragged (PhoshDragSurface *self, int margin)
{
  PhoshHome *home = PHOSH_HOME (self);
  int width, height;
  gtk_window_get_size (GTK_WINDOW (self), &width, &height);
  phosh_arrow_set_progress (PHOSH_ARROW (home->arrow_home), 1.0 - (-margin / (double)(height - PHOSH_HOME_BUTTON_HEIGHT)));
  g_debug ("Margin: %d", margin);
}


static void
on_drag_state_changed (PhoshHome *self)
{
  PhoshHomeState state = PHOSH_HOME_STATE_FOLDED;
  PhoshDragSurfaceState drag_state;
  double arrow;
  gboolean kbd_interactivity = FALSE;

  drag_state = phosh_drag_surface_get_drag_state (PHOSH_DRAG_SURFACE (self));
  if (drag_state == PHOSH_DRAG_SURFACE_STATE_DRAGGED) {
    if (self->state == PHOSH_HOME_STATE_FOLDED) {
      phosh_overview_reset (PHOSH_OVERVIEW (self->overview));
    }
    return;
  }

  switch (drag_state) {
  case PHOSH_DRAG_SURFACE_STATE_UNFOLDED:
    state = PHOSH_HOME_STATE_UNFOLDED;
    kbd_interactivity = TRUE;
    arrow = 1.0;
    break;
  case PHOSH_DRAG_SURFACE_STATE_FOLDED:
    state = PHOSH_HOME_STATE_FOLDED;
    arrow = 0.0;
    break;
  case PHOSH_DRAG_SURFACE_STATE_DRAGGED:
  default:
    g_return_if_reached ();
    return;
  }

  if (state == self->state)
    return;

  self->state = state;
  phosh_home_update_osk_button (self);

  phosh_layer_surface_set_kbd_interactivity (PHOSH_LAYER_SURFACE (self), kbd_interactivity);
  update_drag_handle (self, FALSE);
  phosh_layer_surface_wl_surface_commit (PHOSH_LAYER_SURFACE (self));

  phosh_arrow_set_progress (PHOSH_ARROW (self->arrow_home), arrow);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_HOME_STATE]);
}


static void
phosh_home_constructed (GObject *object)
{
  PhoshHome *self = PHOSH_HOME (object);
  g_autoptr (GSettings) settings = NULL;

  G_OBJECT_CLASS (phosh_home_parent_class)->constructed (object);

  g_object_connect (self->settings,
                    "swapped-signal::changed::" KEYBINDING_KEY_TOGGLE_OVERVIEW,
                    on_keybindings_changed, self,
                    "swapped-signal::changed::" KEYBINDING_KEY_TOGGLE_APPLICATION_VIEW,
                    on_keybindings_changed, self,
                    NULL);
  add_keybindings (self);

  settings = g_settings_new ("org.gnome.desktop.a11y.applications");
  g_settings_bind (settings, "screen-keyboard-enabled",
                   self, "osk-enabled", G_SETTINGS_BIND_GET);

  g_signal_connect (self, "notify::drag-state", G_CALLBACK (on_drag_state_changed), NULL);

  g_object_set_data (G_OBJECT (self->click_gesture), "phosh-home", self);
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
  g_clear_handle_id (&self->debounce_handle, g_source_remove);

  G_OBJECT_CLASS (phosh_home_parent_class)->dispose (object);
}


static void
phosh_home_class_init (PhoshHomeClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  PhoshDragSurfaceClass *drag_surface_class = PHOSH_DRAG_SURFACE_CLASS (klass);

  object_class->constructed = phosh_home_constructed;
  object_class->dispose = phosh_home_dispose;

  object_class->set_property = phosh_home_set_property;
  object_class->get_property = phosh_home_get_property;

  drag_surface_class->dragged = phosh_home_dragged;

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
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, btn_osk);
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, click_gesture);
  gtk_widget_class_bind_template_child (widget_class, PhoshHome, overview);
  gtk_widget_class_bind_template_callback (widget_class, fold_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_home_released);
  gtk_widget_class_bind_template_callback (widget_class, on_has_activities_changed);
  gtk_widget_class_bind_template_callback (widget_class, osk_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, window_key_press_event_cb);

  gtk_widget_class_set_css_name (widget_class, "phosh-home");
}


static void
phosh_home_init (PhoshHome *self)
{
  self->state = PHOSH_HOME_STATE_FOLDED;
  self->settings = g_settings_new (KEYBINDINGS_SCHEMA_ID);

  gtk_widget_init_template (GTK_WIDGET (self));

  phosh_home_update_osk_button (self);

  /* Adjust margins and folded state on size changes */
  g_signal_connect (self, "configure-event", G_CALLBACK (on_configure_event), NULL);
}


GtkWidget *
phosh_home_new (struct zwlr_layer_shell_v1 *layer_shell,
                struct zphoc_layer_shell_effects_v1 *layer_shell_effects,
                struct wl_output *wl_output)
{
  return g_object_new (PHOSH_TYPE_HOME,
                       /* layer-surface */
                       "layer-shell", layer_shell,
                       "wl-output", wl_output,
                       "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_TOP,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", PHOSH_HOME_BUTTON_HEIGHT,
                       "namespace", "phosh home",
                       /* drag-surface */
                       "layer-shell-effects", layer_shell_effects,
                       "exclusive", PHOSH_HOME_BUTTON_HEIGHT,
                       "threshold", PHOSH_HOME_DRAG_THRESHOLD,
                       NULL);
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
  PhoshDragSurfaceState target_state = PHOSH_DRAG_SURFACE_STATE_FOLDED;

  g_return_if_fail (PHOSH_IS_HOME (self));

  if (self->state == state)
    return;

  state_name = g_enum_to_string (PHOSH_TYPE_HOME_STATE, state);
  g_debug ("Setting state to %s", state_name);

  if (state == PHOSH_HOME_STATE_UNFOLDED)
    target_state = PHOSH_DRAG_SURFACE_STATE_UNFOLDED;

  phosh_drag_surface_set_drag_state (PHOSH_DRAG_SURFACE (self), target_state);
}


PhoshOverview*
phosh_home_get_overview (PhoshHome *self)
{
  g_return_val_if_fail (PHOSH_IS_HOME (self), NULL);

  return PHOSH_OVERVIEW (self->overview);
}
