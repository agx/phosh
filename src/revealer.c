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
 * Reveals e.g. a [class@StatusIcon] in the [class@TopPanel].  Similar
 * to [class@Gtk.Revealer] but toggles the transition based on the
 * `show-child` property which also triggers the child's visibility so it
 * doesn't use up any size when not revealed (e.g. when using the `crossfade`
 * animation).
 */

enum {
  PROP_0,
  PROP_SHOW_CHILD,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshRevealer {
  GtkRevealer           parent;

  gboolean              show_child;
};
G_DEFINE_TYPE (PhoshRevealer, phosh_revealer, GTK_TYPE_REVEALER)


static void
on_child_revealed_changed (PhoshRevealer *self)
{
  gboolean visible;
  GtkWidget *child;

  g_return_if_fail (PHOSH_IS_REVEALER (self));

  visible = (gtk_revealer_get_child_revealed (GTK_REVEALER (self)) ||
             gtk_revealer_get_reveal_child (GTK_REVEALER (self)));
  if (visible)
    return;

  child = gtk_bin_get_child (GTK_BIN (self));
  /* Hide the widget so it gives up it's space */
  if (child)
    gtk_widget_set_visible (child, FALSE);
}


static void
phosh_revealer_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  PhoshRevealer *self = PHOSH_REVEALER (object);

  switch (property_id) {
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
  case PROP_SHOW_CHILD:
    g_value_set_boolean (value, phosh_revealer_get_show_child (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_revealer_class_init (PhoshRevealerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = phosh_revealer_set_property;
  object_class->get_property = phosh_revealer_get_property;

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
}


static void
phosh_revealer_init (PhoshRevealer *self)
{
  g_signal_connect (self, "notify::child-revealed", G_CALLBACK (on_child_revealed_changed), NULL);
  on_child_revealed_changed (self);
}


PhoshRevealer *
phosh_revealer_new (void)
{
  return PHOSH_REVEALER (g_object_new (PHOSH_TYPE_REVEALER, NULL));
}

/**
 * phosh_revealer_get_show_child:
 * @self: The PhoshRevealer:
 *
 * If %TRUE the child should be shown, otherwise hidden.
 *
 * Returns; Whether the child should be shown.
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
    GtkWidget *child ;

    child = gtk_bin_get_child (GTK_BIN (self));
    if (child) {
      gtk_widget_set_visible (child, TRUE);
    }
  } else {
    /* Child will be hidden at the end of the animation */
  }

  gtk_revealer_set_reveal_child (GTK_REVEALER (self), show_child);
}
