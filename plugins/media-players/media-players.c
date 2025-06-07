/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "media-player.h"
#include "media-players.h"
#include "mpris-manager.h"
#include "plugin-shell.h"

/**
 * PhoshMediaPlayers
 *
 * Show all currently running media players
 */
struct _PhoshMediaPlayers {
  GtkBox      parent;

  GListModel *known_players;

  GtkListBox *media_players_list_box;
  GtkStack   *media_players_stack;
};

G_DEFINE_TYPE (PhoshMediaPlayers, phosh_media_players, GTK_TYPE_BOX);


static void
on_n_items_changed (PhoshMediaPlayers *self)
{
  const char *stack_page = "no-media-players";
  guint n_players;

  n_players = g_list_model_get_n_items (self->known_players);
  g_debug ("Tracking %u players", n_players);

  if (n_players)
    stack_page = "media-players";

  gtk_stack_set_visible_child_name (self->media_players_stack, stack_page);
}


static void
phosh_media_players_finalize (GObject *object)
{
  PhoshMediaPlayers *self = PHOSH_MEDIA_PLAYERS (object);

  g_clear_object (&self->known_players);

  G_OBJECT_CLASS (phosh_media_players_parent_class)->finalize (object);
}


static void
phosh_media_players_class_init (PhoshMediaPlayersClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_media_players_finalize;

  g_type_ensure (PHOSH_TYPE_MEDIA_PLAYER);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/plugins/media-players/media-players.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshMediaPlayers, media_players_list_box);
  gtk_widget_class_bind_template_child (widget_class, PhoshMediaPlayers, media_players_stack);

  gtk_widget_class_set_css_name (widget_class, "phosh-media-players");
}


static GtkWidget *
create_media_player_row (gpointer item, gpointer user_data)
{
  PhoshMprisDBusMediaPlayer2Player *mpris_player = PHOSH_MPRIS_DBUS_MEDIA_PLAYER2_PLAYER (item);
  GtkWidget *media_player = phosh_media_player_new ();

  g_debug ("Adding new player %s", g_dbus_proxy_get_name (G_DBUS_PROXY (mpris_player)));

  phosh_media_player_set_player (PHOSH_MEDIA_PLAYER (media_player), mpris_player);

  return media_player;
}


static void
phosh_media_players_init (PhoshMediaPlayers *self)
{
  PhoshMprisManager *manager = phosh_shell_get_mpris_manager (phosh_shell_get_default ());
  g_autoptr (GtkCssProvider) css_provider = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->known_players = g_object_ref (phosh_mpris_manager_get_known_players (manager));
  gtk_list_box_bind_model (self->media_players_list_box,
                           G_LIST_MODEL (self->known_players),
                           create_media_player_row,
                           NULL,
                           NULL);
  g_signal_connect_object (self->known_players,
                           "items-changed",
                           G_CALLBACK (on_n_items_changed),
                           self,
                           G_CONNECT_SWAPPED);
  on_n_items_changed (self);

  css_provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (css_provider,
                                       "/mobi/phosh/plugins/media-players/stylesheet/common.css");
  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (self)),
                                  GTK_STYLE_PROVIDER (css_provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}
