/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * based on LGPL-2.1+ HdyKeypad which is
 * Copyright (C) 2019 Purism SPC
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "keypad.h"
#include "util.h"

/**
 * SECTION:keypad
 * @short_description: A keypad for pin input
 * @Title: PhoshKeypad
 *
 * The #PhoshKeypad widget is a keypad for entering
 * or PIN codes on e.g. a #PhoshLockscreen.
 *
 * # CSS nodes
 *
 * #PhoshKeypad has a single CSS node with name phosh-keypad.
 */

typedef struct _PhoshKeypad {
  GtkGrid    parent;

  GtkEntry  *entry;
} PhoshKeypad;

G_DEFINE_TYPE (PhoshKeypad, phosh_keypad, GTK_TYPE_GRID)

enum {
  PROP_0,
  PROP_ENTRY,
  PROP_END_ACTION,
  PROP_START_ACTION,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

static void
symbol_clicked (PhoshKeypad *self,
                gchar        symbol)
{
  g_autofree gchar *string = g_strdup_printf ("%c", symbol);

  if (!self->entry)
    return;

  g_signal_emit_by_name (self->entry, "insert-at-cursor", string, NULL);
  /* Set focus to the entry only when it can get focus
   * https://gitlab.gnome.org/GNOME/gtk/issues/2204
   */
  if (gtk_widget_get_can_focus (GTK_WIDGET (self->entry)))
    gtk_entry_grab_focus_without_selecting (self->entry);
}


static void
on_button_clicked (PhoshKeypad *self,
                   GtkButton   *btn)
{
  GtkWidget *label = gtk_bin_get_child (GTK_BIN (btn));
  const char *text = gtk_label_get_label (GTK_LABEL (label));

  g_return_if_fail (!STR_IS_NULL_OR_EMPTY (text));

  symbol_clicked (self, text[0]);
  g_debug ("Button with number %c was pressed", text[0]);
}


static void
phosh_keypad_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  PhoshKeypad *self = PHOSH_KEYPAD (object);

  switch (property_id) {
  case PROP_ENTRY:
    phosh_keypad_set_entry (self, g_value_get_object (value));
    break;
  case PROP_END_ACTION:
    phosh_keypad_set_end_action (self, g_value_get_object (value));
    break;
  case PROP_START_ACTION:
    phosh_keypad_set_start_action (self, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_keypad_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  PhoshKeypad *self = PHOSH_KEYPAD (object);

  switch (property_id) {
  case PROP_ENTRY:
    g_value_set_object (value, phosh_keypad_get_entry (self));
    break;
  case PROP_START_ACTION:
    g_value_set_object (value, phosh_keypad_get_start_action (self));
    break;
  case PROP_END_ACTION:
    g_value_set_object (value, phosh_keypad_get_end_action (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_keypad_class_init (PhoshKeypadClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = phosh_keypad_set_property;
  object_class->get_property = phosh_keypad_get_property;

  /**
   * PhoshKeypad:entry:
   *
   * The entry widget connected to the keypad. See phosh_keypad_set_entry() for
   * details.
   */
  props[PROP_ENTRY] =
    g_param_spec_object ("entry",
                         "Entry",
                         "The entry widget connected to the keypad",
                         GTK_TYPE_ENTRY,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PhoshKeypad:end-action:
   *
   * The widget for the lower end corner of @self.
   */
  props[PROP_END_ACTION] =
    g_param_spec_object ("end-action",
                         "End action",
                         "The end action widget",
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PhoshKeypad:start-action:
   *
   * The widget for the lower start corner of @self.
   */
  props[PROP_START_ACTION] =
    g_param_spec_object ("start-action",
                         "Start action",
                         "The start action widget",
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/keypad.ui");

  gtk_widget_class_bind_template_callback (widget_class, on_button_clicked);

  gtk_widget_class_set_accessible_role (widget_class, ATK_ROLE_DIAL);
  gtk_widget_class_set_css_name (widget_class, "phosh-keypad");
}


static void
phosh_keypad_init (PhoshKeypad *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


/**
 * phosh_keypad_new:
 *
 * Create a new #PhoshKeypad widget.
 *
 * Returns: the newly created #PhoshKeypad widget
 */
GtkWidget *
phosh_keypad_new (void)
{
  return g_object_new (PHOSH_TYPE_KEYPAD, NULL);
}

/**
 * phosh_keypad_set_entry:
 * @self: a #PhoshKeypad
 * @entry: (nullable): a #GtkEntry
 *
 * Binds @entry to @self and blocks any input which wouldn't be possible to type
 * with with the keypad.
 */
void
phosh_keypad_set_entry (PhoshKeypad *self,
                        GtkEntry    *entry)
{
  g_return_if_fail (PHOSH_IS_KEYPAD (self));
  g_return_if_fail (entry == NULL || GTK_IS_ENTRY (entry));

  if (entry == self->entry)
    return;

  g_clear_object (&self->entry);

  if (entry) {
    self->entry = g_object_ref (entry);

    gtk_widget_show (GTK_WIDGET (self->entry));
    /* Workaround: To keep the osk closed
     * https://gitlab.gnome.org/GNOME/gtk/merge_requests/978#note_546576 */
    g_object_set (self->entry, "im-module", "gtk-im-context-none", NULL);
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENTRY]);
}


/**
 * phosh_keypad_get_entry:
 * @self: a #PhoshKeypad
 *
 * Get the connected entry. See phosh_keypad_set_entry() for details.
 *
 * Returns: (transfer none): the set #GtkEntry or %NULL if no widget was set
 */
GtkEntry *
phosh_keypad_get_entry (PhoshKeypad *self)
{
  g_return_val_if_fail (PHOSH_IS_KEYPAD (self), NULL);

  return self->entry;
}


/**
 * phosh_keypad_set_start_action:
 * @self: a #PhoshKeypad
 * @start_action: (nullable): the start action widget
 *
 * Sets the widget for the lower left corner (or right, in RTL locales) of
 * @self.
 */
void
phosh_keypad_set_start_action (PhoshKeypad *self,
                               GtkWidget   *start_action)
{
  GtkWidget *old_widget;

  g_return_if_fail (PHOSH_IS_KEYPAD (self));
  g_return_if_fail (start_action == NULL || GTK_IS_WIDGET (start_action));

  old_widget = gtk_grid_get_child_at (GTK_GRID (self), 0, 3);

  if (old_widget == start_action)
    return;

  if (old_widget != NULL)
    gtk_container_remove (GTK_CONTAINER (self), old_widget);

  if (start_action != NULL)
    gtk_grid_attach (GTK_GRID (self), start_action, 0, 3, 1, 1);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_START_ACTION]);
}


/**
 * phosh_keypad_get_start_action:
 * @self: a #PhoshKeypad
 *
 * Returns the widget for the lower left corner (or right, in RTL locales) of
 * @self.
 *
 * Returns: (transfer none) (nullable): the start action widget
 */
GtkWidget *
phosh_keypad_get_start_action (PhoshKeypad *self)
{
  g_return_val_if_fail (PHOSH_IS_KEYPAD (self), NULL);

  return gtk_grid_get_child_at (GTK_GRID (self), 0, 3);
}


/**
 * phosh_keypad_set_end_action:
 * @self: a #PhoshKeypad
 * @end_action: (nullable): the end action widget
 *
 * Sets the widget for the lower right corner (or left, in RTL locales) of
 * @self.
 */
void
phosh_keypad_set_end_action (PhoshKeypad *self,
                             GtkWidget   *end_action)
{
  GtkWidget *old_widget;

  g_return_if_fail (PHOSH_IS_KEYPAD (self));
  g_return_if_fail (end_action == NULL || GTK_IS_WIDGET (end_action));

  old_widget = gtk_grid_get_child_at (GTK_GRID (self), 2, 3);

  if (old_widget == end_action)
    return;

  if (old_widget != NULL)
    gtk_container_remove (GTK_CONTAINER (self), old_widget);

  if (end_action != NULL)
    gtk_grid_attach (GTK_GRID (self), end_action, 2, 3, 1, 1);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_END_ACTION]);
}


/**
 * phosh_keypad_get_end_action:
 * @self: a #PhoshKeypad
 *
 * Returns the widget for the lower right corner (or left, in RTL locales) of
 * @self.
 *
 * Returns: (transfer none) (nullable): the end action widget
 */
GtkWidget *
phosh_keypad_get_end_action (PhoshKeypad *self)
{
  g_return_val_if_fail (PHOSH_IS_KEYPAD (self), NULL);

  return gtk_grid_get_child_at (GTK_GRID (self), 2, 3);
}
