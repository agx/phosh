/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-system-modal-dialog"

#include "phosh-config.h"

#include "animation.h"
#include "shell.h"
#include "system-modal-dialog.h"
#include "swipe-away-bin.h"
#include "util.h"

#include <gmobile.h>

/**
 * PhoshSystemModalDialog:
 *
 * A modal system dialog
 *
 * The #PhoshSystemModalDialog is used as a base class for system modal dialogs
 * such as #PhoshSystemPrompt or #PhoshNetworkAuthPrompt. It consists of a title
 * at the top, a content widget below that and button are at the bottom.
 * The content widget can be set via #phosh_system_modal_dialog_set_content() and buttons
 * to the button area added via #phosh_system_modal_dialog_add_button().
 *
 * # CSS Style classes
 *
 * A system modal dialog uses several style classes for consistent layout:
 * ".phosh-system-modal-dialog" for the whole dialog area,
 * ".phosh-system-modal-dialog-title" for the dialog title,
 * ".phosh-system-modal-dialog-content" for the content area and
 * ".phosh-system-modal-dialog-buttons" for the button area.
 *
 * # PhoshSystemModalDialog as #GtkBuildable
 *
 * The content widget and buttons can be specified using type
 * &lt;phosh-dialog-content&gt; and &lt;phosh-dialog-button&gt; type attributes:
 *
 * |[
 * <object class="PhoshSystemModalDialog"/>
 *   <child type="phosh-dialog-content">
 *     <object class="GtkBox">
 *       <property name="visible">True</property>
 *       <property name="orientation">vertical</property>
 *       <child>
 *         ...
 *       </child>
 *     </object>
 *   </child>
 *   <child type="phosh-dialog-button">
 *     <object class="GtkButton">
 *       <property name="label">Ok</property>
 *       ...
 *     </object>
 *   </child>
 *   <child type="phosh-dialog-button">
 *     <object class="GtkButton">
 *       <property name="label">Cancel</property>
 *       ...
 *     </object>
 *   </child>
 * </object>
 * ]|
 */

enum {
  PROP_0,
  PROP_TITLE,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

enum {
  DIALOG_CANCELED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

typedef struct {
  gchar          *title;

  GtkWidget      *lbl_title;
  GtkWidget      *box_dialog;
  GtkWidget      *box_buttons;

  PhoshAnimation *animation;
} PhoshSystemModalDialogPrivate;

static void phosh_system_modal_dialog_buildable_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (PhoshSystemModalDialog, phosh_system_modal_dialog,
                         PHOSH_TYPE_SYSTEM_MODAL,
                         G_ADD_PRIVATE (PhoshSystemModalDialog)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE,
                                                phosh_system_modal_dialog_buildable_init))

static void
phosh_system_modal_dialog_set_property (GObject      *obj,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  PhoshSystemModalDialog *self = PHOSH_SYSTEM_MODAL_DIALOG (obj);

  switch (prop_id) {
  case PROP_TITLE:
    phosh_system_modal_dialog_set_title (self, g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    break;
  }
}


static void
phosh_system_modal_dialog_get_property (GObject    *obj,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  PhoshSystemModalDialog *self = PHOSH_SYSTEM_MODAL_DIALOG (obj);
  PhoshSystemModalDialogPrivate *priv = phosh_system_modal_dialog_get_instance_private (self);

  switch (prop_id) {
  case PROP_TITLE:
    g_value_set_string (value, priv->title);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    break;
  }
}


static void
on_removed_by_swipe (PhoshSystemModalDialog *self)
{
  g_signal_emit (self, signals[DIALOG_CANCELED], 0);
}


static gboolean
on_key_press_event (PhoshSystemModalDialog *self, GdkEventKey *event, gpointer data)
{
  gboolean handled = FALSE;

  g_return_val_if_fail (PHOSH_IS_SYSTEM_MODAL_DIALOG (self), FALSE);

  switch (event->keyval) {
  case GDK_KEY_Escape:
    g_signal_emit (self, signals[DIALOG_CANCELED], 0);
    handled = TRUE;
    break;
  default:
    /* nothing to do */
    break;
  }

  return handled;
}


static void
phosh_system_modal_dialog_finalize (GObject *obj)
{
  PhoshSystemModalDialog *self = PHOSH_SYSTEM_MODAL_DIALOG (obj);
  PhoshSystemModalDialogPrivate *priv = phosh_system_modal_dialog_get_instance_private (self);

  g_clear_pointer (&priv->animation, phosh_animation_unref);
  g_clear_pointer (&priv->title, g_free);

  G_OBJECT_CLASS (phosh_system_modal_dialog_parent_class)->finalize (obj);
}


static void
phosh_system_modal_dialog_class_init (PhoshSystemModalDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_system_modal_dialog_get_property;
  object_class->set_property = phosh_system_modal_dialog_set_property;
  object_class->finalize = phosh_system_modal_dialog_finalize;

  /**
   * PhoshSystemModalDialog:title
   *
   * The dialog's title
   */
  props[PROP_TITLE] = g_param_spec_string ("title", "", "",
                                           NULL,
                                           G_PARAM_READWRITE |
                                           G_PARAM_STATIC_STRINGS |
                                           G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  /**
   * PhoshSystemModalDialog::dialog-canceled:
   *
   * The ::dialog-canceled signal is emitted when the dialog was canceled and should be
   * hidden or destroyed.
   */
  signals[DIALOG_CANCELED] = g_signal_new ("dialog-canceled",
                                           G_TYPE_FROM_CLASS (klass),
                                           G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                           NULL, G_TYPE_NONE, 0);

  g_type_ensure (PHOSH_TYPE_SWIPE_AWAY_BIN);
  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/system-modal-dialog.ui");
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSystemModalDialog, lbl_title);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSystemModalDialog, box_dialog);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshSystemModalDialog, box_buttons);
  gtk_widget_class_bind_template_callback (widget_class, on_removed_by_swipe);
}


static GtkBuildableIface *parent_buildable_iface;


static void
phosh_system_modal_dialog_buildable_add_child (GtkBuildable *buildable,
                                               GtkBuilder   *builder,
                                               GObject      *child,
                                               const gchar  *type)
{
  PhoshSystemModalDialog *self = PHOSH_SYSTEM_MODAL_DIALOG (buildable);

  if (g_strcmp0 (type, "phosh-dialog-content") == 0) {
    phosh_system_modal_dialog_set_content (self, GTK_WIDGET (child));
    return;
  }

  if (g_strcmp0 (type, "phosh-dialog-button") == 0) {
    phosh_system_modal_dialog_add_button (self, GTK_WIDGET (child), -1);
    return;
  }

  /* The parent is a container itself so chain up */
  parent_buildable_iface->add_child (buildable, builder, child, type);
}


static void
phosh_system_modal_dialog_buildable_init (GtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = phosh_system_modal_dialog_buildable_add_child;
}



static void
phosh_system_modal_dialog_init (PhoshSystemModalDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_add_events (GTK_WIDGET (self), GDK_KEY_PRESS_MASK);
  g_signal_connect (G_OBJECT (self),
                    "key_press_event",
                    G_CALLBACK (on_key_press_event),
                    NULL);
}

/**
 * phosh_system_modal_dialog_new:
 *
 * Create a new system-modal dialog.
 *
 * Returns: A new system modal dialog
 */
GtkWidget *
phosh_system_modal_dialog_new (void)
{
  return g_object_new (PHOSH_TYPE_SYSTEM_MODAL_DIALOG, NULL);
}


/**
 * phosh_system_modal_dialog_set_content:
 * @self: The #PhoshSystemModalDialog
 * @content: The widget for the dialog's content area
 *
 * Adds the given widget as the dialog's content area. It is a programming error
 * to set the content more than once.
 */
void
phosh_system_modal_dialog_set_content (PhoshSystemModalDialog *self, GtkWidget *content)
{
  PhoshSystemModalDialogPrivate *priv;

  g_return_if_fail (PHOSH_IS_SYSTEM_MODAL_DIALOG (self));
  g_return_if_fail (GTK_IS_WIDGET (content));

  priv = phosh_system_modal_dialog_get_instance_private (PHOSH_SYSTEM_MODAL_DIALOG (self));

  gtk_box_pack_start (GTK_BOX (priv->box_dialog), GTK_WIDGET (content), FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (priv->box_dialog), GTK_WIDGET (content), 1);
  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (content)),
                               "phosh-system-modal-dialog-content");
}


/**
 * phosh_system_modal_dialog_add_button:
 * @self: The #PhoshSystemModalDialog
 * @button: The button for the dialog's button area
 * @position: The buttons position in the box or -1
 *
 * Adds the given button to the dialog's content area at the given position. If
 * the posiion is `-1` the button is appended at the end.
 */
void
phosh_system_modal_dialog_add_button (PhoshSystemModalDialog *self, GtkWidget *button, gint position)
{
  PhoshSystemModalDialogPrivate *priv;

  g_return_if_fail (PHOSH_IS_SYSTEM_MODAL_DIALOG (self));
  g_return_if_fail (GTK_IS_BUTTON (button));

  priv = phosh_system_modal_dialog_get_instance_private (PHOSH_SYSTEM_MODAL_DIALOG (self));

  gtk_box_pack_start (GTK_BOX (priv->box_buttons), GTK_WIDGET (button), TRUE, TRUE, 0);
  if (position >= 0)
    gtk_box_reorder_child (GTK_BOX (priv->box_buttons), GTK_WIDGET (button), position);
}


void
phosh_system_modal_dialog_remove_button (PhoshSystemModalDialog *self, GtkWidget *button)
{
  PhoshSystemModalDialogPrivate *priv;

  g_return_if_fail (PHOSH_IS_SYSTEM_MODAL_DIALOG (self));
  g_return_if_fail (GTK_IS_BUTTON (button));
  priv = phosh_system_modal_dialog_get_instance_private (PHOSH_SYSTEM_MODAL_DIALOG (self));

  gtk_container_remove (GTK_CONTAINER (priv->box_buttons), button);
}

/**
 * phosh_system_modal_dialog_get_buttons:
 * @self: A modal dialog
 *
 * Get the dialog's buttons
 *
 * Returns:(element-type GtkWidget)(transfer container): The buttons
 */
GList *
phosh_system_modal_dialog_get_buttons (PhoshSystemModalDialog *self)
{
  PhoshSystemModalDialogPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_SYSTEM_MODAL_DIALOG (self), NULL);
  priv = phosh_system_modal_dialog_get_instance_private (PHOSH_SYSTEM_MODAL_DIALOG (self));

  return gtk_container_get_children (GTK_CONTAINER (priv->box_buttons));
}


void
phosh_system_modal_dialog_set_title (PhoshSystemModalDialog *self, const gchar *title)
{
  PhoshSystemModalDialogPrivate *priv;

  g_return_if_fail (PHOSH_IS_SYSTEM_MODAL_DIALOG (self));
  priv = phosh_system_modal_dialog_get_instance_private (PHOSH_SYSTEM_MODAL_DIALOG (self));

  if (g_strcmp0 (priv->title, title) == 0)
    return;

  g_free (priv->title);
  priv->title = g_strdup (title);

  gtk_label_set_label (GTK_LABEL (priv->lbl_title), priv->title);
  gtk_widget_set_visible (priv->lbl_title, !gm_str_is_null_or_empty (priv->title));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TITLE]);
}

static void
animation_value_cb (double value, PhoshSystemModalDialog *self)
{
  phosh_layer_surface_set_alpha (PHOSH_LAYER_SURFACE (self), 1.0 - value);
}


static void
animation_done_cb (PhoshSystemModalDialog *self)
{
  PhoshSystemModalDialogPrivate *priv = phosh_system_modal_dialog_get_instance_private (self);

  g_clear_pointer (&priv->animation, phosh_animation_unref);

  gtk_widget_destroy (GTK_WIDGET (self));
}

/**
 * phosh_system_modal_dialog_close:
 * @self: The dialog to close
 *
 * Hides the dialog and destroys it. When the compositor supports it
 * uses an animation. If you want to destroy the dialog directly use
 * `gtk_widget_destroy()`.
 */
void
phosh_system_modal_dialog_close (PhoshSystemModalDialog *self)
{
  PhoshSystemModalDialogPrivate *priv;

  g_return_if_fail (PHOSH_IS_SYSTEM_MODAL_DIALOG (self));
  priv = phosh_system_modal_dialog_get_instance_private (self);

  /* Until we can assume phoc with alpha layer-surface support */
  if (!phosh_layer_surface_has_alpha (PHOSH_LAYER_SURFACE (self))) {
    gtk_widget_destroy (GTK_WIDGET (self));
    return;
  }

  priv->animation = phosh_animation_new (GTK_WIDGET (self),
                                         0.0,
                                         1.0,
                                         150 * PHOSH_ANIMATION_SLOWDOWN,
                                         PHOSH_ANIMATION_TYPE_EASE_OUT_CUBIC,
                                         (PhoshAnimationValueCallback) animation_value_cb,
                                         (PhoshAnimationDoneCallback) animation_done_cb,
                                         self);
  phosh_animation_start (priv->animation);
}
