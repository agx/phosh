/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Parts taken from gnome-flashback which is:
 *
 * Copyright (C) 2008 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2015 Alberts Muktupāvels
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-end-session-dialog"

#include "phosh-config.h"
#include "end-session-dialog.h"
#include "util.h"

#include <gmobile.h>

#include <glib/gi18n.h>
#include <gio/gdesktopappinfo.h>

#define SYNC_DBUS_TIMEOUT 500

/**
 * PhoshEndSessionDialog:
 *
 * A system modal prompt to authorize applications
 *
 * The #PhoshEndSessionDialog is used to confirm/decline the end of the session
 * and is spawned by the #PhoshSessionManager.
 */

/* Taken from gnome-session/gsm-inhibitor-flag.h */
typedef enum {
  PHOSH_GSM_INHIBITOR_FLAG_LOGOUT      = 1 << 0,
  PHOSH_GSM_INHIBITOR_FLAG_SWITCH_USER = 1 << 1,
  PHOSH_GSM_INHIBITOR_FLAG_SUSPEND     = 1 << 2,
  PHOSH_GSM_INHIBITOR_FLAG_IDLE        = 1 << 3,
  PHOSH_GSM_INHIBITOR_FLAG_AUTOMOUNT   = 1 << 4
} PhoshGsmInhibitorFlags;

enum {
  CLOSED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


enum {
  PROP_0,
  PROP_ACTION,
  PROP_TIMEOUT,
  PROP_INHIBITOR_PATHS,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

static void end_session_dialog_update (PhoshEndSessionDialog *self);

typedef struct _PhoshEndSessionDialog {
  PhoshSystemModalDialog parent;

  int                    action;
  gboolean               action_confirmed;
  int                    timeout;
  int                    timeout_id;
  GStrv                  inhibitor_paths;

  GtkWidget             *lbl_subtitle;
  GtkWidget             *lbl_warn;
  GtkWidget             *listbox;
  GtkWidget             *sw_inhibitors;
  GtkWidget             *btn_confirm;
  GtkWidget             *btn_cancel;

  GCancellable          *cancel;
} PhoshEndSessionDialog;


G_DEFINE_TYPE (PhoshEndSessionDialog, phosh_end_session_dialog, PHOSH_TYPE_SYSTEM_MODAL_DIALOG)

static gboolean
is_inhibited (PhoshEndSessionDialog *self)
{
  return (self->inhibitor_paths && g_strv_length (self->inhibitor_paths));
}


static char *
get_user_name (void)
{
  char *name;

  name = g_locale_to_utf8 (g_get_real_name (), -1, NULL, NULL, NULL);

  if (g_strcmp0 (name, "Unknown") == 0  || g_strcmp0 (name, "") == 0) {
    g_free (name);
    name = g_locale_to_utf8 (g_get_user_name (), -1, NULL, NULL, NULL);
  }

  if (!name)
    name = g_strdup (g_get_user_name ());

  return name;
}


static void
on_btn_confirm_clicked (PhoshEndSessionDialog *self, GtkButton *btn)
{
  self->action_confirmed = TRUE;

  g_signal_emit (self, signals[CLOSED], 0);
}


static void
on_dialog_canceled (PhoshEndSessionDialog *self)
{
  g_return_if_fail (PHOSH_IS_END_SESSION_DIALOG (self));

  g_signal_emit (self, signals[CLOSED], 0);
}


static gboolean
end_session_dialog_timeout (gpointer data)
{
  PhoshEndSessionDialog *self = PHOSH_END_SESSION_DIALOG (data);

  if (self->timeout == 0) {
    on_btn_confirm_clicked (self, GTK_BUTTON (self->btn_confirm));
    self->timeout_id = 0;
    return G_SOURCE_REMOVE;
  }

  end_session_dialog_update (self);
  self->timeout--;

  return G_SOURCE_CONTINUE;
}


static void
maybe_start_timer (PhoshEndSessionDialog *self)
{
  if (self->timeout_id == 0 && self->timeout) {
    self->timeout_id = g_timeout_add_seconds (1, end_session_dialog_timeout, self);
    g_source_set_name_by_id (self->timeout_id, "[phosh] end_session_dialog_timeout");
  }
}


static void
end_session_dialog_update (PhoshEndSessionDialog *self)
{
  gboolean inhibited;
  gint seconds;
  const char *title;
  g_autofree char *description = NULL;
  g_autofree char *user_name = NULL;

  maybe_start_timer (self);
  seconds = self->timeout;
  inhibited = is_inhibited (self);

  g_debug ("Action: %d, seconds: %d, inhibit: %d", self->action, seconds, inhibited);

  switch (self->action) {
  case PHOSH_END_SESSION_ACTION_LOGOUT:
    title = _("Log Out");

    user_name = get_user_name ();
    description = g_strdup_printf (ngettext ("%s will be logged out automatically in %d second.",
                                             "%s will be logged out automatically in %d seconds.",
                                             seconds),
                                   user_name, seconds);
    break;
  case PHOSH_END_SESSION_ACTION_SHUTDOWN:
    title = _("Power Off");
    description = g_strdup_printf (ngettext ("The system will power off automatically in %d second.",
                                             "The system will power off automatically in %d seconds.",
                                             seconds),
                                   seconds);
    break;
  case PHOSH_END_SESSION_ACTION_REBOOT:
    title = _("Restart");
    description = g_strdup_printf (ngettext ("The system will restart automatically in %d second.",
                                             "The system will restart automatically in %d seconds.",
                                             seconds),
                                   seconds);
    break;
  default:
    g_return_if_reached ();
  }

  phosh_system_modal_dialog_set_title (PHOSH_SYSTEM_MODAL_DIALOG (self), title);

  gtk_label_set_label (GTK_LABEL (self->lbl_subtitle), description);
  gtk_button_set_label (GTK_BUTTON (self->btn_confirm), title);
}


static char *
inhibitor_get_app_id (GDBusProxy *proxy)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GVariant) res = NULL;
  char *app_id;

  res = g_dbus_proxy_call_sync (proxy, "GetAppId", NULL,
                                0, SYNC_DBUS_TIMEOUT, NULL, &error);
  if (!res) {
    g_warning ("Failed to get Inhibitor app id: %s", error->message);
    return NULL;
  }
  g_variant_get (res, "(s)", &app_id);

  return app_id;
}


static PhoshGsmInhibitorFlags
inhibitor_get_flags (GDBusProxy *proxy)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GVariant) res = NULL;
  guint flags;

  res = g_dbus_proxy_call_sync (proxy, "GetFlags", NULL,
                                0, SYNC_DBUS_TIMEOUT, NULL, &error);
  if (!res) {
    g_warning ("Failed to get Inhibitor flags: %s", error->message);
    return 0;
  }
  g_variant_get (res, "(u)", &flags);

  return flags;
}


static char *
inhibitor_get_reason (GDBusProxy *proxy)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GVariant) res = NULL;
  char *reason;

  res = g_dbus_proxy_call_sync (proxy, "GetReason", NULL,
                                0, SYNC_DBUS_TIMEOUT, NULL, &error);
  if (!res) {
    g_warning ("Failed to get inhibit reason: %s", error->message);
    return NULL;
  }
  g_variant_get (res, "(s)", &reason);

  return reason;
}


static void
add_inhibitor (PhoshEndSessionDialog *self, GDBusProxy *inhibitor)
{
  g_autofree char *app_id = NULL;
  g_autofree char *reason = NULL;
  g_autoptr (GDesktopAppInfo) app_info = NULL;
  PhoshGsmInhibitorFlags flags;
  const char *icon_name = NULL;
  const char *name = NULL;
  GIcon *icon = NULL;
  GtkWidget *box;
  GtkWidget *box_text;
  GtkWidget *label;
  GtkWidget *lbl_reason;
  GtkWidget *img;

  app_id = inhibitor_get_app_id (inhibitor);
  reason = inhibitor_get_reason (inhibitor);
  flags = inhibitor_get_flags (inhibitor);

  if (!(flags & PHOSH_GSM_INHIBITOR_FLAG_LOGOUT))
    return;

  if (gm_str_is_null_or_empty (app_id))
    return;

  app_info = phosh_get_desktop_app_info_for_app_id (app_id);
  if (app_info) {
    icon = g_app_info_get_icon (G_APP_INFO (app_info));
    name = g_app_info_get_display_name (G_APP_INFO (app_info));
  }

  if (!name)
    name = app_id;

  if (!icon)
    icon_name = "app-icon-unknown";

  img = g_object_new (GTK_TYPE_IMAGE,
                      "visible", TRUE,
                      "can-focus", FALSE,
                      "gicon", icon,
                      "halign", GTK_ALIGN_START,
                      "pixel_size", 64,
                      NULL);
  if (!icon)
    g_object_set (img, "icon-name", icon_name, NULL);

  box_text = g_object_new (GTK_TYPE_BOX,
                           "visible", TRUE,
                           "can-focus", FALSE,
                           "halign", GTK_ALIGN_START,
                           "homogeneous", TRUE,
                           "orientation", GTK_ORIENTATION_VERTICAL,
                           "spacing", 0,
                           NULL);

  label = g_object_new (GTK_TYPE_LABEL,
                        "visible", TRUE,
                        "can-focus", FALSE,
                        "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
                        "halign", GTK_ALIGN_START,
                        "label", name,
                        "valign", GTK_ALIGN_END,
                        NULL);
  gtk_box_pack_start (GTK_BOX (box_text), label, TRUE, TRUE, 0);

  if (reason) {
    lbl_reason = g_object_new (GTK_TYPE_LABEL,
                               "visible", TRUE,
                               "can-focus", FALSE,
                               "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
                               "halign", GTK_ALIGN_START,
                               "label", reason,
                               "valign", GTK_ALIGN_START,
                               NULL);
    gtk_box_pack_end (GTK_BOX (box_text), lbl_reason, TRUE, TRUE, 0);
  } else {
    gtk_widget_set_valign (label, GTK_ALIGN_FILL);
  }

  box = g_object_new (GTK_TYPE_BOX,
                      "visible", TRUE,
                      "can-focus", FALSE,
                      "halign", GTK_ALIGN_START,
                      "orientation", GTK_ORIENTATION_HORIZONTAL,
                      "spacing", 12,
                      NULL);

  gtk_box_pack_start (GTK_BOX (box), img, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (box), box_text, FALSE, FALSE, 0);

  gtk_list_box_insert (GTK_LIST_BOX (self->listbox), GTK_WIDGET (box), -1);
  gtk_widget_set_visible (GTK_WIDGET (self->sw_inhibitors), TRUE);
}


static void
on_inhibitor_created (GObject      *source,
                      GAsyncResult *res,
                      gpointer      user_data)
{
  PhoshEndSessionDialog *self;
  g_autoptr (GError) error = NULL;
  g_autoptr (GDBusProxy) proxy = NULL;

  proxy = g_dbus_proxy_new_for_bus_finish (res, &error);

  if (!proxy) {
    g_warning ("Failed to create Inhibitor proxy: %s", error->message);
    return;
  }

  self = PHOSH_END_SESSION_DIALOG (user_data);
  add_inhibitor (self, proxy);
}


static void
clear_inhibitors (PhoshEndSessionDialog *self)
{
  g_autoptr (GList) children = NULL;

  g_return_if_fail (GTK_IS_LIST_BOX (self->listbox));

  children = gtk_container_get_children (GTK_CONTAINER (self->listbox));
  for (GList *child = children; child; child = child->next)
    gtk_container_remove (GTK_CONTAINER (self->listbox), child->data);

  gtk_widget_set_visible (self->sw_inhibitors, FALSE);
}


static void
end_session_dialog_update_inhibitors (PhoshEndSessionDialog *self, GStrv paths)
{
  g_strfreev (self->inhibitor_paths);
  self->inhibitor_paths = g_strdupv ((char **) paths);

  clear_inhibitors (self);

  if (!self->inhibitor_paths)
    return;

  for (int i = 0; self->inhibitor_paths[i]; i++) {
    g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION, 0, NULL,
                              "org.gnome.SessionManager",
                              self->inhibitor_paths[i],
                              "org.gnome.SessionManager.Inhibitor",
                              self->cancel, on_inhibitor_created, self);
  }
}


static void
phosh_end_session_dialog_set_property (GObject      *obj,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  PhoshEndSessionDialog *self = PHOSH_END_SESSION_DIALOG (obj);

  switch (prop_id) {
  case PROP_ACTION:
    self->action = g_value_get_int (value);
    end_session_dialog_update (self);
    break;
  case PROP_TIMEOUT:
    self->timeout = g_value_get_int (value);
    end_session_dialog_update (self);
    break;
  case PROP_INHIBITOR_PATHS:
    end_session_dialog_update_inhibitors (self, g_value_get_boxed (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    break;
  }
}


static void
phosh_end_session_dialog_get_property (GObject    *obj,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  PhoshEndSessionDialog *self = PHOSH_END_SESSION_DIALOG (obj);

  switch (prop_id) {
  case PROP_ACTION:
    g_value_set_int (value, self->action);
    break;
  case PROP_TIMEOUT:
    g_value_set_int (value, self->timeout);
    break;
  case PROP_INHIBITOR_PATHS:
    g_value_set_boxed (value, self->inhibitor_paths);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
    break;
  }
}


static void
phosh_end_session_dialog_dispose (GObject *obj)
{
  PhoshEndSessionDialog *self = PHOSH_END_SESSION_DIALOG (obj);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);

  if (self->listbox)
    clear_inhibitors (self);

  G_OBJECT_CLASS (phosh_end_session_dialog_parent_class)->dispose (obj);
}


static void
phosh_end_session_dialog_finalize (GObject *obj)
{
  PhoshEndSessionDialog *self = PHOSH_END_SESSION_DIALOG (obj);

  g_clear_handle_id (&self->timeout_id, g_source_remove);
  g_clear_pointer (&self->inhibitor_paths, g_strfreev);

  G_OBJECT_CLASS (phosh_end_session_dialog_parent_class)->finalize (obj);
}


static void
phosh_end_session_dialog_class_init (PhoshEndSessionDialogClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = phosh_end_session_dialog_get_property;
  object_class->set_property = phosh_end_session_dialog_set_property;
  object_class->dispose = phosh_end_session_dialog_dispose;
  object_class->finalize = phosh_end_session_dialog_finalize;

  props[PROP_ACTION] =
    g_param_spec_int ("action",
                      "Action",
                      "The requested action",
                      -1,
                      G_MAXINT,
                      -1,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_TIMEOUT] =
    g_param_spec_int ("timeout",
                      "Timeout",
                      "Timeout in seconds after which the action is performed",
                      -1,
                      G_MAXINT,
                      -1,
                      G_PARAM_READWRITE  | G_PARAM_STATIC_STRINGS);

  props[PROP_INHIBITOR_PATHS] =
    g_param_spec_boxed ("inhibitor-paths",
                        "Inhibitor paths",
                        "Paths to inhibitors that prevent atction",
                        G_TYPE_STRV,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  signals[CLOSED] = g_signal_new ("closed",
                                  G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                  NULL, G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/end-session-dialog.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshEndSessionDialog, lbl_subtitle);
  gtk_widget_class_bind_template_child (widget_class, PhoshEndSessionDialog, lbl_warn);
  gtk_widget_class_bind_template_child (widget_class, PhoshEndSessionDialog, listbox);
  gtk_widget_class_bind_template_child (widget_class, PhoshEndSessionDialog, sw_inhibitors);
  gtk_widget_class_bind_template_child (widget_class, PhoshEndSessionDialog, btn_confirm);
  gtk_widget_class_bind_template_child (widget_class, PhoshEndSessionDialog, btn_cancel);
  gtk_widget_class_bind_template_callback (widget_class, on_btn_confirm_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_dialog_canceled);
}


static void
phosh_end_session_dialog_init (PhoshEndSessionDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_end_session_dialog_new (gint                action,
                              gint                timeout,
                              const char *const *inhibitor_paths)
{
  return g_object_new (PHOSH_TYPE_END_SESSION_DIALOG,
                       "action", action,
                       "timeout", timeout,
                       "inhibitor-paths", inhibitor_paths,
                       NULL);
}


gboolean
phosh_end_session_dialog_get_action_confirmed (PhoshEndSessionDialog *self)
{
  g_return_val_if_fail (PHOSH_END_SESSION_DIALOG (self), FALSE);

  return self->action_confirmed;
}


gboolean
phosh_end_session_dialog_get_action (PhoshEndSessionDialog *self)
{
  g_return_val_if_fail (PHOSH_END_SESSION_DIALOG (self), 0);

  return self->action;
}
