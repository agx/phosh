/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * based on LGPL-2.1+ HdyKeypad which is
 * Copyright (C) 2019 Purism SPC
 */

#include "phosh-config.h"
#include <glib/gi18n-lib.h>

#include "keypad.h"
#include "util.h"

#include <gmobile.h>

/**
 * PhoshKeypad:
 *
 * A keypad for pin input
 *
 * The #PhoshKeypad widget mimics a physical keypad for entering
 * PIN codes on e.g. a #PhoshLockscreen. It can randomly
 * distribute (shuffle) the digits.
 *
 * # CSS nodes
 *
 * #PhoshKeypad has a single CSS node with name phosh-keypad.
 */

#define NUM_DIGITS 10
/* Positions of the buttons in the grid we shuffle as x,y coordinates */
static int btn_pos[NUM_DIGITS][2] = { { 1, 3 },
                                      { 0, 0 }, { 1, 0 }, { 2, 0 },
                                      { 0, 1 }, { 1, 1 }, { 2, 1 },
                                      { 0, 2 }, { 1, 2 }, { 2, 2 }};

typedef struct _PhoshKeypad {
  GtkGrid    parent;

  GtkEntry  *entry;
  /* The digit buttinos 1..9 and 0 */
  GtkWidget *buttons[10];

  gboolean   shuffle;
} PhoshKeypad;

G_DEFINE_TYPE (PhoshKeypad, phosh_keypad, GTK_TYPE_GRID)

enum {
  PROP_0,
  PROP_ENTRY,
  PROP_END_ACTION,
  PROP_START_ACTION,
  PROP_SHUFFLE,
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

  g_return_if_fail (!gm_str_is_null_or_empty (text));

  symbol_clicked (self, text[0]);
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
  case PROP_SHUFFLE:
    phosh_keypad_set_shuffle (self, g_value_get_boolean (value));
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
  case PROP_SHUFFLE:
    g_value_set_boolean (value, phosh_keypad_get_shuffle(self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
swap_buttons (PhoshKeypad *self, int pos_a, int pos_b)
{
  GtkWidget *a, *b;

  int c_a = btn_pos[pos_a][0];
  int r_a = btn_pos[pos_a][1];
  int c_b = btn_pos[pos_b][0];
  int r_b = btn_pos[pos_b][1];

  if (pos_a == pos_b)
    return;

  a = gtk_grid_get_child_at (GTK_GRID (self), c_a, r_a);
  gtk_container_remove (GTK_CONTAINER (self), a);

  b = gtk_grid_get_child_at (GTK_GRID (self), c_b, r_b);
  gtk_container_remove (GTK_CONTAINER (self), b);

  gtk_grid_attach (GTK_GRID (self), a, c_b, r_b, 1, 1);
  gtk_grid_attach (GTK_GRID (self), b, c_a, r_a, 1, 1);
}


static void
distribute_buttons (PhoshKeypad *self, gboolean shuffle)
{
  if (shuffle) {
    /* Fisher-Yates shuffle */
    for (int i = 0; i < NUM_DIGITS-1; i++) {
        int j = g_random_int_range (i, NUM_DIGITS);
        swap_buttons (self, i, j);
    }
  } else {
    /* Use sorted positions */
    for (int i = 0; i < NUM_DIGITS; i++) {
      GtkWidget *old;
      int c = btn_pos[i][0];
      int r = btn_pos[i][1];

      old = gtk_grid_get_child_at (GTK_GRID (self), c, r);
      gtk_container_remove (GTK_CONTAINER (self), old);
    }

    for (int i = 0; i < NUM_DIGITS; i++) {
      int c = btn_pos[i][0];
      int r = btn_pos[i][1];

      gtk_grid_attach (GTK_GRID (self), self->buttons[i], c, r, 1, 1);
    }
  }
}


static void
phosh_keypad_dispose (GObject *object)
{
  PhoshKeypad *self = PHOSH_KEYPAD (object);

  for (int i = 0; i < NUM_DIGITS; i++)
    g_clear_object (&self->buttons[i]);

  G_OBJECT_CLASS (phosh_keypad_parent_class)->dispose (object);
}


static void
phosh_keypad_class_init (PhoshKeypadClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = phosh_keypad_dispose;
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

  /**
   * PhoshKeypad:shuffle:
   *
   * Whether to shuffle digits. Setting this to %TRUE will make
   * the digits appear at random locations on the keypad.
   */
  props[PROP_SHUFFLE] =
    g_param_spec_boolean ("shuffle", "", "",
                          FALSE,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/keypad.ui");

  for (int i = 0; i < NUM_DIGITS; i++) {
    g_autofree char *name = g_strdup_printf ("btn_%d", i);
    gtk_widget_class_bind_template_child_full (widget_class,
                                               name,
                                               FALSE,
                                               G_STRUCT_OFFSET(PhoshKeypad, buttons[i]));
  }

  gtk_widget_class_bind_template_callback (widget_class, on_button_clicked);

  gtk_widget_class_set_accessible_role (widget_class, ATK_ROLE_DIAL);
  gtk_widget_class_set_css_name (widget_class, "phosh-keypad");
}


static void
phosh_keypad_init (PhoshKeypad *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  for (int i = 0; i < NUM_DIGITS; i++) {
    /* We hold an extra reference since we remove buttons when reordering
       them and so we don't need to bother then */
    g_object_ref (self->buttons[i]);
  }
  distribute_buttons (self, self->shuffle);

  gtk_widget_set_direction (GTK_WIDGET (self), GTK_TEXT_DIR_LTR);
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
 * Sets the widget for the lower left corner of
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
 * Returns the widget for the lower left corner of
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
 * Sets the widget for the lower right corner of
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
 * Returns the widget for the lower right corner of
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


void
phosh_keypad_set_shuffle (PhoshKeypad *self, gboolean shuffle)
{
  g_return_if_fail (PHOSH_IS_KEYPAD (self));

  if (self->shuffle == shuffle)
    return;

  self->shuffle = shuffle;
  distribute_buttons (self, shuffle);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SHUFFLE]);
}


gboolean
phosh_keypad_get_shuffle (PhoshKeypad *self)
{
  g_return_val_if_fail (PHOSH_IS_KEYPAD (self), FALSE);

  return self->shuffle;
}


/**
 * phosh_keypad_distribute:
 * @self: a #PhoshKeypad
 *
 * Redistribute buttons on keypad. If %PhoshKeypad:shuffle is %TRUE buttons
 * will be reshuffled otherwise they will be ordered.
 **/
void
phosh_keypad_distribute (PhoshKeypad *self)
{
  g_return_if_fail (PHOSH_IS_KEYPAD (self));

  distribute_buttons (self, self->shuffle);
}
