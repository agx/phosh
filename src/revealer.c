/*
 * Copyright (C) 2023 Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "phosh-revealer"

#include "phosh-config.h"

#include "revealer.h"

/**
 * PhoshRevealer:
 *
 * Reveals e.g. a [class@StatusIcon] in the [class@TopPanel].
 *
 * Similar to [class@Gtk.Revealer] but toggles the transition based on
 * the `show-child` property which also triggers the child's
 * visibility so it doesn't use up any size when not revealed
 * (e.g. when using the `crossfade` animation).
 *
 * Since: 0.25.0
 */

enum {
  PROP_0,
  PROP_CHILD,
  PROP_SHOW_CHILD,
  PROP_TRANSITION_DURATION,
  PROP_TRANSITION_TYPE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshRevealer {
  GtkBin       parent;

  GtkRevealer *revealer;
  GtkWidget   *child;
  gboolean     show_child;
  guint        transition_duration;
  GtkRevealerTransitionType transition_type;
};
G_DEFINE_TYPE (PhoshRevealer, phosh_revealer, GTK_TYPE_BIN)


static void
on_child_revealed_changed (PhoshRevealer *self)
{
  gboolean visible;

  visible = (gtk_revealer_get_child_revealed (GTK_REVEALER (self->revealer)) ||
             gtk_revealer_get_reveal_child (GTK_REVEALER (self->revealer)));
  if (visible)
    return;

  /* Hide the widget so it gives up it's space */
  if (self->child)
    gtk_widget_set_visible (self->child, FALSE);
}


static void
phosh_revealer_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  PhoshRevealer *self = PHOSH_REVEALER (object);

  switch (property_id) {
  case PROP_CHILD:
    phosh_revealer_set_child (self, g_value_get_object (value));
    break;
  case PROP_TRANSITION_DURATION:
    phosh_revealer_set_transition_duration (self, g_value_get_uint (value));
    break;
  case PROP_TRANSITION_TYPE:
    phosh_revealer_set_transition_type (self, g_value_get_enum (value));
    break;
  case PROP_SHOW_CHILD:
    phosh_revealer_set_show_child (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_revealer_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  PhoshRevealer *self = PHOSH_REVEALER (object);

  switch (property_id) {
  case PROP_CHILD:
    g_value_set_object (value, phosh_revealer_get_child (self));
    break;
  case PROP_TRANSITION_DURATION:
    g_value_set_uint (value, phosh_revealer_get_transition_duration (self));
    break;
  case PROP_TRANSITION_TYPE:
    g_value_set_enum (value, phosh_revealer_get_transition_type (self));
    break;
  case PROP_SHOW_CHILD:
    g_value_set_boolean (value, phosh_revealer_get_show_child (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_revealer_destroy (GtkWidget *widget)
{
  PhoshRevealer *self = PHOSH_REVEALER (widget);

  self->child = NULL;

  GTK_WIDGET_CLASS (phosh_revealer_parent_class)->destroy (widget);
}


static void
phosh_revealer_class_init (PhoshRevealerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = phosh_revealer_set_property;
  object_class->get_property = phosh_revealer_get_property;
  widget_class->destroy = phosh_revealer_destroy;

  /**
   * PhoshRevealer:child:
   *
   * The child to be revealed and hidden.
   */
  props[PROP_CHILD] =
    g_param_spec_object ("child", "", "",
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  /**
   * PhoshRevealer:transition-duration:
   *
   * The duration of transition.
   */
  props[PROP_TRANSITION_DURATION] =
    g_param_spec_uint ("transition-duration", "", "",
                       0, G_MAXUINT, 400,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  /**
   * PhoshRevealer:transition-type:
   *
   * The type of transition.
   */
  props[PROP_TRANSITION_TYPE] =
    g_param_spec_enum ("transition-type", "", "",
                       GTK_TYPE_REVEALER_TRANSITION_TYPE, GTK_REVEALER_TRANSITION_TYPE_CROSSFADE,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  /**
   * PhoshRevealer:show-child:
   *
   * Whether the child should be shown. This make it visible and fades
   * it in via the given transition. When %FALSE triggers the fade out
   * animation and hides the child at the end.
   */
  props[PROP_SHOW_CHILD] =
    g_param_spec_boolean ("show-child", "", "",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/mobi/phosh/ui/revealer.ui");

  gtk_widget_class_bind_template_child (widget_class, PhoshRevealer, revealer);

  gtk_widget_class_bind_template_callback (widget_class, on_child_revealed_changed);
}


static void
phosh_revealer_init (PhoshRevealer *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  self->transition_duration = 400;
  self->transition_type = GTK_REVEALER_TRANSITION_TYPE_CROSSFADE;
  on_child_revealed_changed (self);
}


PhoshRevealer *
phosh_revealer_new (void)
{
  return PHOSH_REVEALER (g_object_new (PHOSH_TYPE_REVEALER, NULL));
}

/**
 * phosh_revealer_get_child:
 * @self: The PhoshRevealer.
 *
 * Get the child of revealer.
 *
 * Returns:(transfer none): The child of revealer.
 */
GtkWidget *
phosh_revealer_get_child (PhoshRevealer *self)
{
  g_return_val_if_fail (PHOSH_IS_REVEALER (self), NULL);

  return self->child;
}

/**
 * phosh_revealer_set_child:
 * @self: The PhoshRevealer.
 * @child: The child to set.
 *
 * Set the child of revealer. Use `NULL` to remove existing child.
 */
void
phosh_revealer_set_child (PhoshRevealer *self, GtkWidget *child)
{
  g_return_if_fail (PHOSH_IS_REVEALER (self));
  g_return_if_fail (child == NULL || GTK_IS_WIDGET (child));

  if (child == self->child)
    return;

  if (self->child)
    gtk_container_remove (GTK_CONTAINER (self->revealer), self->child);

  self->child = child;

  if (self->child)
    gtk_container_add (GTK_CONTAINER (self->revealer), self->child);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CHILD]);
}

/**
 * phosh_revealer_get_show_child:
 * @self: The PhoshRevealer:
 *
 * If %TRUE the child should be shown, otherwise hidden.
 *
 * Returns: Whether the child should be shown.
 */
gboolean
phosh_revealer_get_show_child (PhoshRevealer *self)
{
  g_return_val_if_fail (PHOSH_IS_REVEALER (self), FALSE);

  return self->show_child;
}

/**
 * phosh_revealer_set_show_child:
 * @self: The PhoshRevealer:
 * @show_child: Whether the child should be shown
 *
 * If `show_child` is %TRUE, the child will be set visible and shown
 * using a %GtkRevealer Otherwise it will be hidden.
 */
void
phosh_revealer_set_show_child (PhoshRevealer *self, gboolean show_child)
{
  g_return_if_fail (PHOSH_IS_REVEALER (self));

  if (show_child == self->show_child)
    return;

  self->show_child = show_child;
  if (show_child) {
    if (self->child)
      gtk_widget_set_visible (self->child, TRUE);
  } else {
    /* Child will be hidden at the end of the animation */
  }

  gtk_revealer_set_reveal_child (self->revealer, show_child);
}

/**
 * phosh_revealer_get_transition_duration:
 * @self: The PhoshRevealer.
 *
 * Get the transition duration.
 *
 * Returns: The transition duration.
 */
guint
phosh_revealer_get_transition_duration (PhoshRevealer *self)
{
  g_return_val_if_fail (PHOSH_IS_REVEALER (self), 0);

  return self->transition_duration;
}

/**
 * phosh_revealer_set_transition_duration:
 * @self: The PhoshRevealer.
 * @transition_duraiton: Duration for transition.
 *
 * Set the transition duration.
 */
void
phosh_revealer_set_transition_duration (PhoshRevealer *self, guint transition_duration)
{
  g_return_if_fail (PHOSH_IS_REVEALER (self));

  if (self->transition_duration == transition_duration)
    return;

  self->transition_duration = transition_duration;
  gtk_revealer_set_transition_duration (self->revealer, self->transition_duration);
}

/**
 * phosh_revealer_get_transition_type:
 * @self: The PhoshRevealer.
 *
 * Get the transition type.
 *
 * Returns: The transition type.
 */
GtkRevealerTransitionType
phosh_revealer_get_transition_type (PhoshRevealer *self)
{
  g_return_val_if_fail (PHOSH_IS_REVEALER (self), GTK_REVEALER_TRANSITION_TYPE_NONE);

  return self->transition_type;
}

/**
 * phosh_revealer_set_transition_type:
 * @self: The PhoshRevealer.
 * @transition_type: Type for transition.
 *
 * Set the transition type.
 */
void
phosh_revealer_set_transition_type (PhoshRevealer *self, GtkRevealerTransitionType transition_type)
{
  g_return_if_fail (PHOSH_IS_REVEALER (self));

  if (self->transition_type == transition_type)
    return;

  self->transition_type = transition_type;
  gtk_revealer_set_transition_type (self->revealer, self->transition_type);
}
