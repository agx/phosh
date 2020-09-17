/*
 * Copyright (C) 2020 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-media-player"

#include "config.h"

#include "mpris-dbus.h"
#include "media-player.h"

#include <glib/gi18n.h>

#define MPRIS_OBJECT_PATH "/org/mpris/MediaPlayer2"
#define MPRIS_PREFIX "org.mpris.MediaPlayer2."

/**
 * SECTION:media-player
 * @short_description: A simple MPRIS media player widget
 * @Title: PhoshMediaPlayer
 *
 * The #PhoshMediaPlayer widget interfaces with
 * [org.mpris.MediaPlayer2](https://specifications.freedesktop.org/mpris-spec/latest/)
 * based players allowing to skip through music and raising the player.
 *
 * Whenever a player is found on the bus the #PhoshMediaPlayer:attached
 * property is set to %TRUE. This can e.g. be used with
 * #g_object_bind_property() to toggle the widget's visibility.
 *
 * # Example
 *
 * |[
 * <object class="PhoshMediaPlayer" id="media_player">
 *   <property name="visible" bind-source="media_player" bind-property="attached" bind-flags="sync-create"/>
 * </object>
 * ]|
 */

typedef enum {
  PHOSH_MEDIA_PLAYER_STATUS_STOPPED,
  PHOSH_MEDIA_PLAYER_STATUS_PAUSED,
  PHOSH_MEDIA_PLAYER_STATUS_PLAYING,
} PhoshMediaPlayerStatus;

enum {
  PROP_0,
  PROP_ATTACHED,
  PROP_PLAYABLE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

enum {
  PLAYER_RAISED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];

typedef struct _PhoshMediaPlayer {
  GtkGrid                           parent_instance;

  GtkWidget                        *btn_play;
  GtkWidget                        *btn_next;
  GtkWidget                        *btn_prev;
  GtkWidget                        *btn_details;
  GtkWidget                        *img_art;
  GtkWidget                        *lbl_title;
  GtkWidget                        *lbl_artist;

  /* Base interface to raise player */
  PhoshMprisDBusMediaPlayer2       *mpris;
  /* Actual player controls */
  PhoshMprisDBusMediaPlayer2Player *player;
  GDBusConnection                  *session_bus;
  guint                             dbus_id;
  PhoshMediaPlayerStatus            status;
  gboolean                          attached;
  gboolean                          playable;
} PhoshMediaPlayer;

G_DEFINE_TYPE (PhoshMediaPlayer, phosh_media_player, GTK_TYPE_GRID);

static void
phosh_media_player_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  PhoshMediaPlayer *self = PHOSH_MEDIA_PLAYER (object);

  switch (property_id) {
  case PROP_ATTACHED:
    g_value_set_boolean (value, self->attached);
    break;
  case PROP_PLAYABLE:
    g_value_set_boolean (value, self->playable);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
set_playable (PhoshMediaPlayer *self, gboolean playable)
{
  if (self->playable == playable)
    return;

  self->playable = playable;
  g_debug ("Playable: %d", playable);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PLAYABLE]);
}

static void
set_attached (PhoshMediaPlayer *self, gboolean attached)
{
  if (self->attached == attached)
    return;

  self->attached = attached;
  if (!attached)
    set_playable (self, FALSE);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ATTACHED]);
}

static void
on_play_pause_done (PhoshMprisDBusMediaPlayer2Player *player,
                    GAsyncResult                     *res,
                    PhoshMediaPlayer                 *self)
{
  g_autoptr (GError) err = NULL;

  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));
  g_return_if_fail (PHOSH_MPRIS_DBUS_IS_MEDIA_PLAYER2_PLAYER (player));

  if (!phosh_mpris_dbus_media_player2_player_call_play_pause_finish (player, res, &err)) {
    g_warning ("Failed to trigger play/pause: %s", err->message);
  }
}

static void
btn_play_clicked_cb (PhoshMediaPlayer *self, GtkButton *button)
{
  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));
  g_return_if_fail (PHOSH_MPRIS_DBUS_IS_MEDIA_PLAYER2_PLAYER (self->player));

  g_debug ("Play/pause");
  phosh_mpris_dbus_media_player2_player_call_play_pause (self->player,
                                                         NULL,
                                                         (GAsyncReadyCallback)on_play_pause_done,
                                                         self);
}

static void
on_next_done (PhoshMprisDBusMediaPlayer2Player *player, GAsyncResult *res, PhoshMediaPlayer *self)
{
  g_autoptr (GError) err = NULL;

  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));
  g_return_if_fail (PHOSH_MPRIS_DBUS_IS_MEDIA_PLAYER2_PLAYER (player));

  if (!phosh_mpris_dbus_media_player2_player_call_next_finish (player, res, &err))
    g_warning ("Failed to trigger next: %s", err->message);
}

static void
btn_next_clicked_cb (PhoshMediaPlayer *self, GtkButton *button)
{
  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));
  g_return_if_fail (PHOSH_MPRIS_DBUS_IS_MEDIA_PLAYER2_PLAYER (self->player));

  g_debug ("next");
  phosh_mpris_dbus_media_player2_player_call_next (self->player,
                                                   NULL,
                                                   (GAsyncReadyCallback)on_next_done,
                                                   self);
}

static void
on_previous_done (PhoshMprisDBusMediaPlayer2Player *player, GAsyncResult *res, PhoshMediaPlayer *self)
{
  g_autoptr (GError) err = NULL;

  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));
  g_return_if_fail (PHOSH_MPRIS_DBUS_IS_MEDIA_PLAYER2_PLAYER (player));

  if (!phosh_mpris_dbus_media_player2_player_call_previous_finish (player, res, &err))
    g_warning ("Failed to trigger prev: %s", err->message);
}

static void
btn_prev_clicked_cb (PhoshMediaPlayer *self, GtkButton *button)
{
  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));
  g_return_if_fail (PHOSH_MPRIS_DBUS_IS_MEDIA_PLAYER2_PLAYER (self->player));

  g_debug ("prev");
  phosh_mpris_dbus_media_player2_player_call_previous (self->player,
                                                       NULL,
                                                       (GAsyncReadyCallback)on_previous_done,
                                                       self);
}

static void
on_raise_done (PhoshMprisDBusMediaPlayer2 *mpris, GAsyncResult *res, PhoshMediaPlayer *self)
{
  g_autoptr (GError) err = NULL;

  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));
  g_return_if_fail (PHOSH_MPRIS_DBUS_IS_MEDIA_PLAYER2 (mpris));

  if (!phosh_mpris_dbus_media_player2_call_raise_finish (mpris, res, &err)) {
    g_warning ("Failed to raise player: %s", err->message);
    return;
  }

  g_signal_emit (self, signals[PLAYER_RAISED], 0);
}

static void
btn_details_clicked_cb (PhoshMediaPlayer *self, GtkButton *button)
{
  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));
  g_return_if_fail (PHOSH_MPRIS_DBUS_IS_MEDIA_PLAYER2 (self->mpris));

  g_debug ("details");

  if (!phosh_mpris_dbus_media_player2_get_can_raise (self->mpris))
    return;

  phosh_mpris_dbus_media_player2_call_raise (self->mpris,
                                             NULL,
                                             (GAsyncReadyCallback)on_raise_done,
                                             self);
}

static void
on_metadata_changed (PhoshMediaPlayer *self, GParamSpec *psepc, PhoshMprisDBusMediaPlayer2Player *player)
{
  GVariant *metadata;
  gchar *title = NULL;
  gchar *url = NULL;

  g_auto (GStrv) artist = NULL;
  g_auto (GVariantDict) dict = G_VARIANT_DICT_INIT (NULL);

  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));
  g_debug ("Updating metadata");

  metadata = phosh_mpris_dbus_media_player2_player_get_metadata (player);
  if (!metadata)
    return;
  g_variant_dict_init (&dict, metadata);

  g_variant_dict_lookup (&dict, "xesam:title", "&s", &title);
  if (title) {
    gtk_label_set_label (GTK_LABEL (self->lbl_title), title);
  } else {
    /* Translators: Used when the title of a song is unknown */
    gtk_label_set_label (GTK_LABEL (self->lbl_title), _("Unknown Title"));
  }

  g_variant_dict_lookup (&dict, "xesam:artist", "^as", &artist);
  if (artist && g_strv_length (artist) > 0) {
    g_autofree gchar *artists = g_strjoinv (", ", artist);
    gtk_label_set_label (GTK_LABEL (self->lbl_artist), artists);
  } else {
    /* Translators: Used when the artist of a song is unknown */
    gtk_label_set_label (GTK_LABEL (self->lbl_artist), _("Unknown Artist"));
  }

  g_variant_dict_lookup (&dict, "mpris:artUrl", "&s", &url);
  if (url) {
    g_autoptr (GIcon) icon = NULL;
    g_autoptr (GFile) file = g_file_new_for_uri (url);

    icon = g_file_icon_new (file);
    gtk_image_set_from_gicon (GTK_IMAGE (self->img_art), icon, GTK_ICON_SIZE_DIALOG);
  } else {
    gtk_image_set_from_icon_name (GTK_IMAGE (self->img_art),
                                  "audio-x-generic-symbolic",
                                  GTK_ICON_SIZE_DIALOG);
  }
}

static void
on_playback_status_changed (PhoshMediaPlayer                 *self,
                            GParamSpec                       *psepc,
                            PhoshMprisDBusMediaPlayer2Player *player)
{
  const char *status, *icon = "media-playback-start-symbolic";
  PhoshMediaPlayerStatus current;

  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));

  status = phosh_mpris_dbus_media_player2_player_get_playback_status (player);

  /* No mpris running, widget will not be shown */
  if (status == NULL)
    return;

  g_debug ("Status: '%s'", status);
  current = self->status;
  if (!g_strcmp0 ("Playing", status)) {
    self->status = PHOSH_MEDIA_PLAYER_STATUS_PLAYING;
    icon = "media-playback-pause-symbolic";
    set_playable (self, TRUE);
  } else if (!g_strcmp0 ("Paused", status)) {
    self->status = PHOSH_MEDIA_PLAYER_STATUS_PAUSED;
    set_playable (self, TRUE);
  } else if (!g_strcmp0 ("Stopped", status)) {
    self->status = PHOSH_MEDIA_PLAYER_STATUS_STOPPED;
    gtk_label_set_label (GTK_LABEL (self->lbl_artist), "");
    gtk_label_set_label (GTK_LABEL (self->lbl_title), "");
    set_playable (self, FALSE);
  } else {
    g_warning ("Unknown status %s", status);
    g_warn_if_reached ();
  }

  if (self->status != current) {
    GtkWidget *image = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_button_set_image (GTK_BUTTON (self->btn_play), image);
  }
}


static void
on_can_go_next_changed (PhoshMediaPlayer                 *self,
                        GParamSpec                       *psepc,
                        PhoshMprisDBusMediaPlayer2Player *player)
{
  gboolean sensitive;

  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));
  sensitive = phosh_mpris_dbus_media_player2_player_get_can_go_next (player);
  g_debug ("Can go next: %d", sensitive);
  gtk_widget_set_sensitive (self->btn_next, sensitive);
}

static void
on_can_go_previous_changed (PhoshMediaPlayer                 *self,
                            GParamSpec                       *psepc,
                            PhoshMprisDBusMediaPlayer2Player *player)
{
  gboolean sensitive;

  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));
  sensitive = phosh_mpris_dbus_media_player2_player_get_can_go_previous (player);
  g_debug ("Can go prev: %d", sensitive);
  gtk_widget_set_sensitive (self->btn_prev, sensitive);
}

static void
on_can_play (PhoshMediaPlayer                 *self,
             GParamSpec                       *psepc,
             PhoshMprisDBusMediaPlayer2Player *player)
{
  gboolean sensitive;

  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));
  sensitive = phosh_mpris_dbus_media_player2_player_get_can_play (player);
  g_debug ("Can play: %d", sensitive);
  gtk_widget_set_sensitive (self->btn_play, sensitive);
}

static void
phosh_media_player_dispose (GObject *object)
{
  PhoshMediaPlayer *self = PHOSH_MEDIA_PLAYER (object);

  if (self->dbus_id) {
    g_dbus_connection_signal_unsubscribe (self->session_bus, self->dbus_id);
    self->dbus_id = 0;
    g_clear_object (&self->session_bus);
  }
  g_clear_object (&self->mpris);
  g_clear_object (&self->player);

  G_OBJECT_CLASS (phosh_media_player_parent_class)->dispose (object);
}


static void
phosh_media_player_class_init (PhoshMediaPlayerClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = phosh_media_player_dispose;
  object_class->get_property = phosh_media_player_get_property;

  props[PROP_ATTACHED] =
    g_param_spec_boolean ("attached",
                          "Player attached",
                          "Whether a player is attached",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PhoshMediaPlayer:playable
   *
   * Whether the player has a playable track. This is mostly
   * useful to ignore states where the player does not know
   * about any track and so no sensible information can be
   * shown.
   */
  props[PROP_PLAYABLE] =
    g_param_spec_boolean ("playable",
                          "Playable",
                          "Whether the player has a playable track",
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  /**
   * PhoshMediaPlayer::player-raised:
   *
   * The player was raised to the user
   */
  signals[PLAYER_RAISED] = g_signal_new ("player-raised",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0,
                                         NULL,
                                         NULL,
                                         NULL,
                                         G_TYPE_NONE,
                                         0);

  gtk_widget_class_set_css_name (widget_class, "phosh-media-player");
  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/media-player.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshMediaPlayer, btn_next);
  gtk_widget_class_bind_template_child (widget_class, PhoshMediaPlayer, btn_play);
  gtk_widget_class_bind_template_child (widget_class, PhoshMediaPlayer, btn_prev);
  gtk_widget_class_bind_template_child (widget_class, PhoshMediaPlayer, btn_details);
  gtk_widget_class_bind_template_child (widget_class, PhoshMediaPlayer, img_art);
  gtk_widget_class_bind_template_child (widget_class, PhoshMediaPlayer, lbl_artist);
  gtk_widget_class_bind_template_child (widget_class, PhoshMediaPlayer, lbl_title);
  gtk_widget_class_bind_template_callback (widget_class, btn_play_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, btn_next_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, btn_prev_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, btn_details_clicked_cb);
}

static void
attach_player_cb (GObject          *source_object,
                  GAsyncResult     *res,
                  PhoshMediaPlayer *self)
{
  g_autoptr (GError) err = NULL;

  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));

  self->player = phosh_mpris_dbus_media_player2_player_proxy_new_for_bus_finish (
    res, &err);

  if (!self->player) {
    g_warning ("Failed to get player: %s", err->message);
    set_attached (self, FALSE);
    goto out;
  }

  g_object_connect (self->player,
                    "swapped_object_signal::notify::metadata",
                    G_CALLBACK (on_metadata_changed),
                    self,
                    "swapped_object_signal::notify::playback-status",
                    G_CALLBACK (on_playback_status_changed),
                    self,
                    "swapped_object_signal::notify::can-go-next",
                    G_CALLBACK (on_can_go_next_changed),
                    self,
                    "swapped_object_signal::notify::can-go-previous",
                    G_CALLBACK (on_can_go_previous_changed),
                    self,
                    "swapped_object_signal::notify::can-play",
                    G_CALLBACK (on_can_play),
                    self,
                    NULL);

  g_object_notify (G_OBJECT (self->player), "metadata");
  g_object_notify (G_OBJECT (self->player), "playback-status");
  g_object_notify (G_OBJECT (self->player), "can-go-next");
  g_object_notify (G_OBJECT (self->player), "can-go-previous");
  g_object_notify (G_OBJECT (self->player), "can-play");

  g_debug ("Connected player");
  set_attached (self, TRUE);
out:
  g_object_unref (self);
}

static void
attach_mpris_cb (GObject          *source_object,
                 GAsyncResult     *res,
                 PhoshMediaPlayer *self)
{
  g_autoptr (GError) err = NULL;
  gboolean sensitive;

  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));

  self->mpris = phosh_mpris_dbus_media_player2_proxy_new_for_bus_finish (
    res, &err);

  /* Missing mpris interface is not fatal */
  if (!self->mpris)
    g_warning ("no mpris: %s", err->message);

  sensitive = phosh_mpris_dbus_media_player2_get_can_raise (self->mpris);
  gtk_widget_set_sensitive (self->btn_details, sensitive);

  g_object_unref (self);
}

static void
attach_player (PhoshMediaPlayer *self, const gchar *name)
{
  g_autoptr (GError) err = NULL;

  g_clear_object (&self->player);
  g_clear_object (&self->mpris);

  g_debug ("Trying to attach player for %s", name);

  /* The player interface with the controls */
  phosh_mpris_dbus_media_player2_player_proxy_new_for_bus (
    G_BUS_TYPE_SESSION,
    G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
    name,
    MPRIS_OBJECT_PATH,
    NULL,
    (GAsyncReadyCallback)attach_player_cb,
    g_object_ref (self));

  /* The player base interface to e.g. raise the player */
  phosh_mpris_dbus_media_player2_proxy_new_for_bus (
    G_BUS_TYPE_SESSION,
    G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
    name,
    MPRIS_OBJECT_PATH,
    NULL,
    (GAsyncReadyCallback)attach_mpris_cb,
    g_object_ref (self));
}

static gboolean
is_valid_player (const gchar *bus_name)
{
  if (!g_str_has_prefix (bus_name, MPRIS_PREFIX))
    return FALSE;

  if (strlen (bus_name) < G_N_ELEMENTS (MPRIS_PREFIX))
    return FALSE;

  return TRUE;
}

static void
find_player_cb (GObject          *source_object,
                GAsyncResult     *res,
                PhoshMediaPlayer *self)
{
  g_autoptr (GVariant) result = NULL;
  g_autoptr (GVariant) names = NULL;
  g_autoptr (GError) err = NULL;
  const gchar *name;
  GVariantIter iter;
  gboolean found = FALSE;

  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));
  g_return_if_fail (G_IS_DBUS_CONNECTION (self->session_bus));

  result = g_dbus_connection_call_finish (self->session_bus, res, &err);
  if (!result) {
    g_warning ("Failed to list bus names to find "
               "mpris player: %s", err->message);
    set_attached (self, FALSE);
    goto out;
  }

  names = g_variant_get_child_value (result, 0);
  g_variant_iter_init (&iter, names);
  while (g_variant_iter_loop (&iter, "&s", &name)) {

    if (!is_valid_player (name))
      continue;

    g_debug ("Found player: %s", name);
    attach_player (self, name);
    found = TRUE;
    break;
  }

  if (!found) {
    g_debug ("No player found");
    set_attached (self, FALSE);
  }
out:
  g_object_unref (self);
}

static void
find_player_async (PhoshMediaPlayer *self)
{
  g_return_if_fail (G_IS_DBUS_CONNECTION (self->session_bus));

  g_dbus_connection_call (self->session_bus,
                          "org.freedesktop.DBus",
                          "/org/freedesktop/DBus",
                          "org.freedesktop.DBus",
                          "ListNames",
                          NULL,
                          G_VARIANT_TYPE ("(as)"),
                          G_DBUS_CALL_FLAGS_NO_AUTO_START,
                          1000,
                          NULL,
                          (GAsyncReadyCallback)find_player_cb,
                          g_object_ref (self));
}

static void
on_dbus_name_owner_changed (GDBusConnection  *connection,
                            const gchar      *sender_name,
                            const gchar      *object_path,
                            const gchar      *interface_name,
                            const gchar      *signal_name,
                            GVariant         *parameters,
                            PhoshMediaPlayer *self)
{
  g_autofree char *name, *from, *to;

  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));

  g_variant_get (parameters, "(sss)", &name, &from, &to);
  g_debug ("mpris player name owner change: '%s' '%s' '%s'", name, from, to);

  if (!is_valid_player (name))
    return;

  /* Current player vanished, look for another one, already running */
  if (!g_strcmp0 (to, "")) {
    set_attached (self, FALSE);
    find_player_async (self);
    return;
  }

  /* New player showed up, pick up */
  attach_player (self, name);
}

static void
on_bus_get_finished (GObject          *source_object,
                     GAsyncResult     *res,
                     PhoshMediaPlayer *self)
{
  g_autoptr (GError) err = NULL;

  g_return_if_fail (PHOSH_IS_MEDIA_PLAYER (self));

  self->session_bus = g_bus_get_finish (res, &err);
  if (!self->session_bus) {
    g_warning ("Failed to attach to session bus: %s", err->message);
    goto out;
  }

  /* Listen for name owner changes to detect new mpris players */
  self->dbus_id = g_dbus_connection_signal_subscribe (self->session_bus,
                                                      "org.freedesktop.DBus",
                                                      "org.freedesktop.DBus",
                                                      "NameOwnerChanged",
                                                      "/org/freedesktop/DBus",
                                                      NULL,
                                                      G_DBUS_SIGNAL_FLAGS_NONE,
                                                      (GDBusSignalCallback)on_dbus_name_owner_changed,
                                                      self, NULL);
  /* Find player initially */
  find_player_async (self);

out:
  g_object_unref (self);
}

static gboolean
on_idle (PhoshMediaPlayer *self)
{
  g_bus_get (G_BUS_TYPE_SESSION,
             NULL,
             (GAsyncReadyCallback)on_bus_get_finished,
             g_object_ref (self));
  return G_SOURCE_REMOVE;
}

static void
phosh_media_player_init (PhoshMediaPlayer *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  /* Perform DBus setup when idle */
  g_idle_add ((GSourceFunc)on_idle, self);
}


GtkWidget *
phosh_media_player_new (void)
{
  return g_object_new (PHOSH_TYPE_MEDIA_PLAYER, NULL);
}
