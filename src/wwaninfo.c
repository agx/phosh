/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

/* WWAN Info widget */

#define G_LOG_DOMAIN "phosh-wwaninfo"

#include "config.h"

#include "wwaninfo.h"
#include "wwan/phosh-wwan-mm.h"

#define WWAN_INFO_DEFAULT_ICON_SIZE 24
#define WWAN_INFO_WWAN_NUM_SIGNALS 4

/**
 * SECTION:phosh-wwan-info
 * @short_description: A widget to display the wwan status
 * @Title: PhoshWWanInfo
 */

typedef struct
{
  PhoshWWanMM *wwan;
  GtkStyleContext *style_context;
  gint size;
  guint wwan_signal_ids[WWAN_INFO_WWAN_NUM_SIGNALS];

} PhoshWWanInfoPrivate;


typedef struct _PhoshWWanInfo
{
  GtkImage parent;
} PhoshWWanInfo;


G_DEFINE_TYPE_WITH_PRIVATE (PhoshWWanInfo, phosh_wwan_info, GTK_TYPE_IMAGE)


static const char *
signal_quality_descriptive(guint quality)
{
  if (quality > 80)
    return "excellent";
  else if (quality > 55)
    return "good";
  else if (quality > 30)
    return "ok";
  else if (quality > 5)
    return "weak";
  else
    return "none";
}


static GdkPixbuf *
icon_to_pixbuf (PhoshWWanInfo *self,
                const gchar *name,
                GtkIconTheme *theme)
{
  PhoshWWanInfoPrivate *priv = phosh_wwan_info_get_instance_private (self);
  g_autoptr(GtkIconInfo) info = NULL;
  GdkPixbuf    *pixbuf;
  GError       *error = NULL;

  info = gtk_icon_theme_lookup_icon (theme,
                                     name,
                                     priv->size,
                                     0);
  g_return_val_if_fail (info, NULL);
  pixbuf = gtk_icon_info_load_symbolic_for_context (info,
                                                    priv->style_context,
                                                    NULL,
                                                    &error);

  if (pixbuf == NULL) {
    g_warning ("Could not load icon pixbuf: %s", error->message);
    g_clear_error (&error);
  }
  return pixbuf;
}


static GdkPixbuf *
pixbuf_overlay_access_tec (PhoshWWanInfo *self,
                           const char *access_tec,
                           GdkPixbuf *source)
{
  PhoshWWanInfoPrivate *priv = phosh_wwan_info_get_instance_private (self);
  GdkRGBA color;
  PangoLayout *layout;
  cairo_t *cr;
  GdkPixbuf *pixbuf;
  cairo_surface_t *cs;
  PangoFontDescription *desc;
  gint tw = 0, th = 0, scale;
  guint width, height;
  g_autofree char *font = NULL;

  width = gdk_pixbuf_get_width (source);
  height = gdk_pixbuf_get_height (source);

  cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create (cs);
  gdk_cairo_set_source_pixbuf (cr, source, 0, 0);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);

  gtk_style_context_get_color(priv->style_context, GTK_STATE_FLAG_NORMAL, &color);

  layout = pango_cairo_create_layout (cr);

  scale = strlen(access_tec) > 2 ? 5 : 3;
  font = g_strdup_printf("Sans Bold %d", height / scale);
  desc = pango_font_description_from_string (font);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  pango_layout_set_text (layout, access_tec, -1);
  pango_layout_get_pixel_size (layout, &tw, NULL);
  /* Use the baseline instead of text height since we don't have characters below
     the baseline. This make sure we align properly at the bottom of the icon */
  th = PANGO_PIXELS_FLOOR(pango_layout_get_baseline (layout));

  cairo_save (cr);
  /* clear rectangle behind the text */
  cairo_set_source_rgba (cr, 0, 0, 0, 0);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

  cairo_rectangle (cr,
                   (width  - 1) - tw,
                   (height - 1) - th,
                   tw, th);
  cairo_fill (cr);
  cairo_restore (cr);
  gtk_render_layout (priv->style_context, cr,
                     (width  - 1) - tw,
                     (height - 1) - th,
                     layout);

  pixbuf = gdk_pixbuf_get_from_surface (cs, 0, 0, width, height);
  cairo_destroy(cr);
  cairo_surface_destroy (cs);
  g_object_unref (layout);
  return pixbuf;
}


static void
update_icon_data(PhoshWWanInfo *self, PhoshWWanMM *wwan, gpointer unused)
{
  PhoshWWanInfoPrivate *priv;
  guint quality;
  GtkIconTheme *icon_theme;
  g_autoptr(GdkPixbuf) src = NULL, dest = NULL;
  g_autofree gchar *icon_name = NULL;
  const char *access_tec;

  g_debug ("Updating wwan icon");
  g_return_if_fail (PHOSH_IS_WWAN_INFO (self));
  priv = phosh_wwan_info_get_instance_private (self);

  icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET(self)));
  /* SIM missing */
  if (!phosh_wwan_has_sim (PHOSH_WWAN (priv->wwan))) {
    src = icon_to_pixbuf (self, "auth-sim-missing-symbolic", icon_theme);
    gtk_image_set_from_pixbuf (GTK_IMAGE (self), src);
    return;
  }

  /* SIM unlock required */
  if (!phosh_wwan_is_unlocked (PHOSH_WWAN (priv->wwan))) {
    src = icon_to_pixbuf (self, "auth-sim-locked-symbolic", icon_theme);
    gtk_image_set_from_pixbuf (GTK_IMAGE (self), src);
    return;
  }

  /* Signal quality */
  quality = phosh_wwan_get_signal_quality (PHOSH_WWAN (priv->wwan));
  icon_name = g_strdup_printf ("network-cellular-signal-%s-symbolic",
                               signal_quality_descriptive (quality));

  src = icon_to_pixbuf (self, icon_name, icon_theme);
  g_return_if_fail (src);

  /* Access technology */
  access_tec = phosh_wwan_get_access_tec (PHOSH_WWAN (priv->wwan));
  if (access_tec)
    dest = pixbuf_overlay_access_tec (self, access_tec, src);
  else
    dest = pixbuf_overlay_access_tec (self, "?", src);

  gtk_image_set_from_pixbuf (GTK_IMAGE (self), dest);
}


static void
phosh_wwan_info_constructed (GObject *object)
{
  PhoshWWanInfo *self = PHOSH_WWAN_INFO (object);
  PhoshWWanInfoPrivate *priv = phosh_wwan_info_get_instance_private (self);
  GStrv signals = (char *[]) {"notify::signal-quality",
                              "notify::access-tec",
                              "notify::unlocked",
                              "notify::sim",
                              NULL,
  };

  G_OBJECT_CLASS (phosh_wwan_info_parent_class)->constructed (object);

  priv->style_context = gtk_widget_get_style_context (GTK_WIDGET (self));
  priv->wwan = phosh_wwan_mm_new();

  g_assert (WWAN_INFO_WWAN_NUM_SIGNALS == g_strv_length(signals));
  for (int i = 0; i < WWAN_INFO_WWAN_NUM_SIGNALS; i++) {
    priv->wwan_signal_ids[i] = g_signal_connect_swapped (priv->wwan,
                                                         signals[i],
                                                         G_CALLBACK (update_icon_data),
                                                         self);
  }

  update_icon_data (self, NULL, NULL);
}


static void
phosh_wwan_info_dispose (GObject *object)
{
  PhoshWWanInfoPrivate *priv = phosh_wwan_info_get_instance_private (PHOSH_WWAN_INFO(object));

  if (priv->wwan) {
    for (int i=0; i < WWAN_INFO_WWAN_NUM_SIGNALS; i++) {
      if (priv->wwan_signal_ids[i] > 0) {
        g_signal_handler_disconnect (priv->wwan, priv->wwan_signal_ids[i]);
        priv->wwan_signal_ids[i] = 0;
      }
    }
    g_clear_object (&priv->wwan);
  }

  G_OBJECT_CLASS (phosh_wwan_info_parent_class)->dispose (object);
}


static void
phosh_wwan_info_class_init (PhoshWWanInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_wwan_info_constructed;
  object_class->dispose = phosh_wwan_info_dispose;
}


static void
phosh_wwan_info_init (PhoshWWanInfo *self)
{
  PhoshWWanInfoPrivate *priv = phosh_wwan_info_get_instance_private (self);

  /* TODO: make scalable? */
  priv->size = WWAN_INFO_DEFAULT_ICON_SIZE;
}


GtkWidget *
phosh_wwan_info_new (void)
{
  return g_object_new (PHOSH_TYPE_WWAN_INFO, NULL);
}
