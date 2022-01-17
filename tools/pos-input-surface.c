/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "pos-input-surface"

#include "config.h"

#include "pos-input-surface.h"

/**
 * SECTION:input-surface
 * @short_description: Input surface of the os stub
 * @Title: PosInputSurface
 */

struct _PosInputSurface {
  PhoshLayerSurface   parent;

  gboolean            active;
  guint               serial;
  guint               purpose;
  guint               hint;
  gboolean            pending;

  GtkWidget          *active_label;
  GtkWidget          *purpose_label;
  GtkWidget          *hint_label;
  GtkWidget          *commits_label;
  GtkWidget          *active_pending_label;
  GtkWidget          *purpose_pending_label;
  GtkWidget          *hint_pending_label;
};

G_DEFINE_TYPE (PosInputSurface, pos_input_surface, PHOSH_TYPE_LAYER_SURFACE)

static const char *purposes[] = { "normal", "alpha", "digits", "number", "phone", "url",
  "email", "name", "password", "pin", "date", "time", "datetime",
  "terminal", NULL };

static const char *hints[] = { "completion", "spellcheck", "auto_capitalization",
  "lowercase", "uppercase", "titlecase", "hidden_text",
  "sensitive_data", "latin", "multiline", NULL};

static void
pos_input_surface_class_init (PosInputSurfaceClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/osk-stub/ui/input-surface.ui");
  gtk_widget_class_bind_template_child (widget_class, PosInputSurface, active_label);
  gtk_widget_class_bind_template_child (widget_class, PosInputSurface, purpose_label);
  gtk_widget_class_bind_template_child (widget_class, PosInputSurface, hint_label);
  gtk_widget_class_bind_template_child (widget_class, PosInputSurface, commits_label);
  gtk_widget_class_bind_template_child (widget_class, PosInputSurface, active_pending_label);
  gtk_widget_class_bind_template_child (widget_class, PosInputSurface, purpose_pending_label);
  gtk_widget_class_bind_template_child (widget_class, PosInputSurface, hint_pending_label);
}


static void
pos_input_surface_init (PosInputSurface *self)
{
  self->pending = TRUE;

  gtk_widget_init_template (GTK_WIDGET (self));
}

void
pos_input_surface_set_purpose (PosInputSurface *self, guint purpose)
{
  g_return_if_fail (POS_IS_INPUT_SURFACE (self));
  g_return_if_fail (purpose < g_strv_length ((GStrv)purposes));

  self->purpose = purpose;
  gtk_label_set_label (GTK_LABEL (self->purpose_pending_label), purposes[purpose]);
}

void
pos_input_surface_set_hint (PosInputSurface *self, guint hint)
{
  g_autoptr (GPtrArray) h = g_ptr_array_new ();
  g_autofree char *str = NULL;

  g_return_if_fail (POS_IS_INPUT_SURFACE (self));

  self->hint = hint;

  for (unsigned i = 0; i < g_strv_length ((GStrv)hints); i++) {
    if (hint & (1 << i)) {
      g_ptr_array_add (h, (gpointer)hints[i]);
    }
  }

  if (!h->pdata)
    g_ptr_array_add (h, "none");

  g_ptr_array_add (h, NULL);
  str = g_strjoinv (", ",  (GStrv)h->pdata);
  gtk_label_set_label (GTK_LABEL (self->hint_pending_label), str);
}

void
pos_input_surface_set_active (PosInputSurface *self, gboolean active)
{
  g_return_if_fail (POS_IS_INPUT_SURFACE (self));

  self->active = active;
  gtk_label_set_label (GTK_LABEL (self->active_pending_label), active ? "true" : "false");
}

gboolean
pos_input_surface_get_active (PosInputSurface *self)
{
  g_return_val_if_fail (POS_IS_INPUT_SURFACE (self), FALSE);

  return self->active;
}

void
pos_input_surface_done (PosInputSurface *self)
{
  const char *text;
  g_autofree char *commits = NULL;

  text = gtk_label_get_label (GTK_LABEL (self->active_pending_label));
  gtk_label_set_label (GTK_LABEL (self->active_label), text);

  text = gtk_label_get_label (GTK_LABEL (self->purpose_pending_label));
  gtk_label_set_label (GTK_LABEL (self->purpose_label), text);

  text = gtk_label_get_label (GTK_LABEL (self->hint_pending_label));
  gtk_label_set_label (GTK_LABEL (self->hint_label), text);

  self->serial++;
  commits = g_strdup_printf ("%d", self->serial);
  gtk_label_set_label (GTK_LABEL (self->commits_label), commits);
}
