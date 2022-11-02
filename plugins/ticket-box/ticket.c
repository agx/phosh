/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "ticket.h"

enum {
  PROP_0,
  PROP_FILE,
  PROP_INFO,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhoshTicket {
  GObject    parent;

  GFile     *file;
  GFileInfo *info;
};
G_DEFINE_TYPE (PhoshTicket, phosh_ticket, G_TYPE_OBJECT)


static void
phosh_ticket_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  PhoshTicket *self = PHOSH_TICKET (object);

  switch (property_id) {
  case PROP_FILE:
    self->file = g_value_dup_object (value);
    break;
  case PROP_INFO:
    self->info = g_value_dup_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_ticket_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  PhoshTicket *self = PHOSH_TICKET (object);

  switch (property_id) {
  case PROP_FILE:
    g_value_set_object (value, self->file);
    break;
  case PROP_INFO:
    g_value_set_object (value, self->info);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phosh_ticket_finalize (GObject *object)
{
  PhoshTicket *self = PHOSH_TICKET (object);

  g_clear_object (&self->file);
  g_clear_object (&self->info);

  G_OBJECT_CLASS (phosh_ticket_parent_class)->finalize (object);
}


static void
phosh_ticket_class_init (PhoshTicketClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phosh_ticket_get_property;
  object_class->set_property = phosh_ticket_set_property;
  object_class->finalize = phosh_ticket_finalize;

  props[PROP_FILE] =
    g_param_spec_object ("file", "", "",
                         G_TYPE_FILE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  props[PROP_INFO] =
    g_param_spec_object ("info", "", "",
                         G_TYPE_FILE_INFO,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phosh_ticket_init (PhoshTicket *self)
{
}


PhoshTicket *
phosh_ticket_new (GFile *file, GFileInfo *info)
{
  return PHOSH_TICKET (g_object_new (PHOSH_TYPE_TICKET,
                                     "file", file,
                                     "info", info,
                                     NULL));
}


GFile *
phosh_ticket_get_file (PhoshTicket *self)
{
  g_return_val_if_fail (PHOSH_IS_TICKET (self), NULL);

  return self->file;
}


GIcon *
phosh_ticket_get_icon (PhoshTicket *self)
{
  g_return_val_if_fail (PHOSH_IS_TICKET (self), NULL);

  return g_file_info_get_symbolic_icon (self->info);
}


const char *
phosh_ticket_get_display_name (PhoshTicket *self)
{
  g_return_val_if_fail (PHOSH_IS_TICKET (self), NULL);

  return g_file_info_get_display_name (self->info);
}

GDateTime *
phosh_ticket_get_mod_time (PhoshTicket *self)
{
  g_return_val_if_fail (PHOSH_IS_TICKET (self), NULL);

  return g_file_info_get_modification_date_time (self->info);
}
