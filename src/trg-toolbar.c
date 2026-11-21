/*
 * transmission-remote-gtk - A GTK RPC client to Transmission
 * Copyright (C) 2011-2013  Alan Fitton

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include "trg-compat.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "trg-main-window.h"
#include "trg-menu-bar.h"
#include "trg-menu.h"
#include "trg-prefs.h"
#include "trg-toolbar.h"

enum {
    PROP_0,
    PROP_CONNECT_BUTTON,
    PROP_DISCONNECT_BUTTON,
    PROP_ADD_BUTTON,
    PROP_ADD_URL_BUTTON,
    PROP_REMOVE_BUTTON,
    PROP_DELETE_BUTTON,
    PROP_RESUME_BUTTON,
    PROP_PAUSE_BUTTON,
    /*PROP_VERIFY_BUTTON, */
    PROP_PROPS_BUTTON,
    PROP_REMOTE_PREFS_BUTTON,
    PROP_LOCAL_PREFS_BUTTON,
    PROP_PREFS,
    PROP_MAIN_WINDOW
};

G_DEFINE_TYPE(TrgToolbar, trg_toolbar, GTK_TYPE_BOX)
#define TRG_TOOLBAR_GET_PRIVATE(o)                                                                 \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), TRG_TYPE_TOOLBAR, TrgToolbarPrivate))
typedef struct _TrgToolbarPrivate TrgToolbarPrivate;

struct _TrgToolbarPrivate {
    GtkWidget *tb_connect;
    GtkWidget *tb_connect_menu;
    GtkWidget *tb_disconnect;
    GtkWidget *tb_add;
    GtkWidget *tb_remove;
    GtkWidget *tb_delete;
    GtkWidget *tb_resume;
    GtkWidget *tb_pause;
    GtkWidget *tb_props;
    GtkWidget *tb_remote_prefs;
    GtkWidget *tb_local_prefs;
    TrgPrefs *prefs;
    TrgMainWindow *main_window;
};

static void trg_toolbar_set_property(GObject *object, guint prop_id, const GValue *value,
                                     GParamSpec *pspec G_GNUC_UNUSED)
{
    TrgToolbarPrivate *priv = TRG_TOOLBAR_GET_PRIVATE(object);

    switch (prop_id) {
    case PROP_PREFS:
        priv->prefs = g_value_get_pointer(value);
        break;
    case PROP_MAIN_WINDOW:
        priv->main_window = g_value_get_object(value);
        break;
    }
}

static void trg_toolbar_get_property(GObject *object, guint property_id, GValue *value,
                                     GParamSpec *pspec)
{
    TrgToolbarPrivate *priv = TRG_TOOLBAR_GET_PRIVATE(object);

    switch (property_id) {
    case PROP_CONNECT_BUTTON:
        g_value_set_object(value, priv->tb_connect);
        break;
    case PROP_DISCONNECT_BUTTON:
        g_value_set_object(value, priv->tb_disconnect);
        break;
    case PROP_ADD_BUTTON:
        g_value_set_object(value, priv->tb_add);
        break;
    case PROP_REMOVE_BUTTON:
        g_value_set_object(value, priv->tb_remove);
        break;
    case PROP_DELETE_BUTTON:
        g_value_set_object(value, priv->tb_delete);
        break;
    case PROP_RESUME_BUTTON:
        g_value_set_object(value, priv->tb_resume);
        break;
    case PROP_PAUSE_BUTTON:
        g_value_set_object(value, priv->tb_pause);
        break;
    case PROP_PROPS_BUTTON:
        g_value_set_object(value, priv->tb_props);
        break;
    case PROP_REMOTE_PREFS_BUTTON:
        g_value_set_object(value, priv->tb_remote_prefs);
        break;
    case PROP_LOCAL_PREFS_BUTTON:
        g_value_set_object(value, priv->tb_local_prefs);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void trg_toolbar_install_widget_prop(GObjectClass *class, guint propId, const gchar *name,
                                            const gchar *nick)
{
    g_object_class_install_property(class, propId,
                                    g_param_spec_object(name, nick, nick, GTK_TYPE_WIDGET,
                                                        G_PARAM_READABLE | G_PARAM_STATIC_NAME
                                                            | G_PARAM_STATIC_NICK
                                                            | G_PARAM_STATIC_BLURB));
}

static GtkWidget *trg_toolbar_item_new(TrgToolbar *toolbar, const gchar *text, gchar *icon,
                                       gboolean sensitive)
{
    GtkWidget *button = gtk_button_new();
    GtkWidget *image = gtk_image_new_from_icon_name(icon);

    gtk_image_set_pixel_size(GTK_IMAGE(image), 16);
    gtk_button_set_child(GTK_BUTTON(button), image);
    gtk_widget_add_css_class(button, "flat");
    gtk_widget_set_sensitive(button, sensitive);
    gtk_widget_set_tooltip_text(button, text);
    gtk_box_append(GTK_BOX(toolbar), button);

    return button;
}

static void trg_toolbar_refresh_menu(GtkWidget *w, gpointer data)
{
    TrgToolbarPrivate *priv = TRG_TOOLBAR_GET_PRIVATE(data);
    TrgMenu *new_menu = trg_menu_bar_file_connect_menu_new(priv->main_window, priv->prefs);
    GtkPopover *old = gtk_menu_button_get_popover(GTK_MENU_BUTTON(priv->tb_connect_menu));

    (void)w;

    if (old != NULL && gtk_widget_get_visible(GTK_WIDGET(old)))
        gtk_popover_popdown(old);

    gtk_menu_button_set_popover(GTK_MENU_BUTTON(priv->tb_connect_menu),
                                GTK_WIDGET(trg_menu_get_widget(new_menu)));
}

static GObject *trg_toolbar_constructor(GType type, guint n_construct_properties,
                                        GObjectConstructParam *construct_params)
{
    GObject *obj = G_OBJECT_CLASS(trg_toolbar_parent_class)
                       ->constructor(type, n_construct_properties, construct_params);
    TrgToolbarPrivate *priv = TRG_TOOLBAR_GET_PRIVATE(obj);
    GtkWidget *connect_box, *connect_button, *connect_menu_button, *image;
    TrgMenu *menu;

    gtk_orientable_set_orientation(GTK_ORIENTABLE(obj), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_set_spacing(GTK_BOX(obj), 4);
    gtk_widget_add_css_class(GTK_WIDGET(obj), "toolbar");

    /* Connect: a regular button plus a dropdown arrow for the profile menu. */
    connect_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(connect_box, "linked");

    priv->tb_connect = connect_button = gtk_button_new();
    image = gtk_image_new_from_icon_name("network-wired-symbolic");
    gtk_image_set_pixel_size(GTK_IMAGE(image), 16);
    gtk_button_set_child(GTK_BUTTON(connect_button), image);
    gtk_widget_set_tooltip_text(connect_button, _("Connect"));
    gtk_widget_add_css_class(connect_button, "flat");
    gtk_box_append(GTK_BOX(connect_box), connect_button);

    priv->tb_connect_menu = connect_menu_button = gtk_menu_button_new();
    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(connect_menu_button), "pan-down-symbolic");
    gtk_widget_set_tooltip_text(connect_menu_button, _("Connect"));
    gtk_widget_add_css_class(connect_menu_button, "flat");
    menu = trg_menu_bar_file_connect_menu_new(priv->main_window, priv->prefs);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(connect_menu_button),
                                GTK_WIDGET(trg_menu_get_widget(menu)));
    gtk_box_append(GTK_BOX(connect_box), connect_menu_button);

    gtk_box_append(GTK_BOX(obj), connect_box);

    priv->tb_disconnect = trg_toolbar_item_new(TRG_TOOLBAR(obj), _("Disconnect"),
                                               "network-wired-acquiring-symbolic", FALSE);
    priv->tb_add = trg_toolbar_item_new(TRG_TOOLBAR(obj), _("Add"), "list-add-symbolic", FALSE);

    gtk_box_append(GTK_BOX(obj),
                   gtk_separator_new(GTK_ORIENTATION_VERTICAL));

    priv->tb_resume = trg_toolbar_item_new(TRG_TOOLBAR(obj), _("Resume"),
                                           "media-playback-start-symbolic", FALSE);
    priv->tb_pause = trg_toolbar_item_new(TRG_TOOLBAR(obj), _("Pause"),
                                          "media-playback-pause-symbolic", FALSE);
    priv->tb_props = trg_toolbar_item_new(TRG_TOOLBAR(obj), _("Properties"),
                                          "text-x-generic-symbolic", FALSE);
    priv->tb_remove = trg_toolbar_item_new(TRG_TOOLBAR(obj), _("Remove"), "list-remove-symbolic",
                                           FALSE);
    priv->tb_delete = trg_toolbar_item_new(TRG_TOOLBAR(obj), _("Remove and delete data"),
                                           "user-trash-symbolic", FALSE);

    gtk_box_append(GTK_BOX(obj),
                   gtk_separator_new(GTK_ORIENTATION_VERTICAL));

    priv->tb_local_prefs = trg_toolbar_item_new(TRG_TOOLBAR(obj), _("Local Preferences"),
                                                "system-run-symbolic", TRUE);
    priv->tb_remote_prefs = trg_toolbar_item_new(TRG_TOOLBAR(obj), _("Remote Preferences"),
                                                 "network-workgroup-symbolic", FALSE);

    g_signal_connect(G_OBJECT(priv->prefs), "pref-profile-changed",
                     G_CALLBACK(trg_toolbar_refresh_menu), obj);

    return obj;
}

static void trg_toolbar_class_init(TrgToolbarClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = trg_toolbar_get_property;
    object_class->set_property = trg_toolbar_set_property;
    object_class->constructor = trg_toolbar_constructor;

    g_object_class_install_property(
        object_class, PROP_PREFS,
        g_param_spec_pointer("prefs", "Prefs", "Prefs",
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_NAME
                                 | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));

    g_object_class_install_property(
        object_class, PROP_MAIN_WINDOW,
        g_param_spec_object("mainwindow", "mainwindow", "mainwindow", TRG_TYPE_MAIN_WINDOW,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME
                                | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));

    trg_toolbar_install_widget_prop(object_class, PROP_CONNECT_BUTTON, "connect-button",
                                    "Connect Button");
    trg_toolbar_install_widget_prop(object_class, PROP_DISCONNECT_BUTTON, "disconnect-button",
                                    "Disconnect Button");
    trg_toolbar_install_widget_prop(object_class, PROP_ADD_BUTTON, "add-button", "Add Button");
    trg_toolbar_install_widget_prop(object_class, PROP_ADD_URL_BUTTON, "add-url-button",
                                    "Add URL Button");
    trg_toolbar_install_widget_prop(object_class, PROP_REMOVE_BUTTON, "remove-button",
                                    "Remove Button");
    trg_toolbar_install_widget_prop(object_class, PROP_DELETE_BUTTON, "delete-button",
                                    "Delete Button");
    trg_toolbar_install_widget_prop(object_class, PROP_RESUME_BUTTON, "resume-button",
                                    "Resume Button");
    trg_toolbar_install_widget_prop(object_class, PROP_PAUSE_BUTTON, "pause-button",
                                    "Pause Button");
    trg_toolbar_install_widget_prop(object_class, PROP_PROPS_BUTTON, "props-button",
                                    "Props Button");
    trg_toolbar_install_widget_prop(object_class, PROP_REMOTE_PREFS_BUTTON, "remote-prefs-button",
                                    "Remote Prefs Button");
    trg_toolbar_install_widget_prop(object_class, PROP_LOCAL_PREFS_BUTTON, "local-prefs-button",
                                    "Local Prefs Button");

    g_type_class_add_private(klass, sizeof(TrgToolbarPrivate));
}

void trg_toolbar_connected_change(TrgToolbar *tb, gboolean connected)
{
    TrgToolbarPrivate *priv = TRG_TOOLBAR_GET_PRIVATE(tb);

    gtk_widget_set_sensitive(priv->tb_add, connected);
    gtk_widget_set_sensitive(priv->tb_disconnect, connected);
    gtk_widget_set_sensitive(priv->tb_remote_prefs, connected);
}

void trg_toolbar_torrent_actions_sensitive(TrgToolbar *tb, gboolean sensitive)
{
    TrgToolbarPrivate *priv = TRG_TOOLBAR_GET_PRIVATE(tb);

    gtk_widget_set_sensitive(priv->tb_props, sensitive);
    gtk_widget_set_sensitive(priv->tb_remove, sensitive);
    gtk_widget_set_sensitive(priv->tb_delete, sensitive);
    gtk_widget_set_sensitive(priv->tb_resume, sensitive);
    gtk_widget_set_sensitive(priv->tb_pause, sensitive);
}

static void trg_toolbar_init(TrgToolbar *self)
{
    (void)self;
}

TrgToolbar *trg_toolbar_new(TrgMainWindow *win, TrgPrefs *prefs)
{
    return g_object_new(TRG_TYPE_TOOLBAR, "prefs", prefs, "mainwindow", win, NULL);
}
