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

#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "trg-main-window.h"
#include "trg-menu-bar.h"
#include "trg-menu.h"
#include "trg-prefs.h"
#include "trg-torrent-tree-view.h"
#include "trg-tree-view.h"

enum {
    PROP_0,
    PROP_CONNECT_BUTTON,
    PROP_DISCONNECT_BUTTON,
    PROP_ADD_BUTTON,
    PROP_ADD_URL_BUTTON,
    PROP_REMOVE_BUTTON,
    PROP_DELETE_BUTTON,
    PROP_RESUME_BUTTON,
    PROP_RESUME_ALL_BUTTON,
    PROP_PAUSE_BUTTON,
    PROP_PAUSE_ALL_BUTTON,
    PROP_VERIFY_BUTTON,
    PROP_REANNOUNCE_BUTTON,
    PROP_PROPS_BUTTON,
    PROP_COPYMAGNET_BUTTON,
    PROP_MOVE_BUTTON,
    PROP_REMOTE_PREFS_BUTTON,
    PROP_LOCAL_PREFS_BUTTON,
    PROP_ABOUT_BUTTON,
    PROP_VIEW_STATS_BUTTON,
    PROP_VIEW_STATES_BUTTON,
    PROP_VIEW_NOTEBOOK_BUTTON,
    PROP_QUIT,
    PROP_PREFS,
    PROP_MAIN_WINDOW,
    PROP_TORRENT_TREE_VIEW,
    PROP_ACCEL_GROUP,
    PROP_MOVE_DOWN_QUEUE,
    PROP_MOVE_UP_QUEUE,
    PROP_MOVE_BOTTOM_QUEUE,
    PROP_MOVE_TOP_QUEUE,
    PROP_START_NOW
};

#define G_DATAKEY_CONF_KEY   "conf-key"
#define G_DATAKEY_PREF_VALUE "pref-index"

G_DEFINE_TYPE(TrgMenuBar, trg_menu_bar, GTK_TYPE_BOX)
#define TRG_MENU_BAR_GET_PRIVATE(o)                                                                \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), TRG_TYPE_MENU_BAR, TrgMenuBarPrivate))
typedef struct _TrgMenuBarPrivate TrgMenuBarPrivate;

struct _TrgMenuBarPrivate {
    GtkWidget *mb_connect;
    GtkWidget *mb_disconnect;
    GtkWidget *mb_add;
    GtkWidget *mb_add_url;
    GtkWidget *mb_move;
    GtkWidget *mb_remove;
    GtkWidget *mb_delete;
    GtkWidget *mb_resume;
    GtkWidget *mb_pause;
    GtkWidget *mb_resume_all;
    GtkWidget *mb_pause_all;
    GtkWidget *mb_verify;
    GtkWidget *mb_reannounce;
    GtkWidget *mb_props;
    GtkWidget *mb_copy_magnetlink;
    GtkWidget *mb_local_prefs;
    GtkWidget *mb_remote_prefs;
    GtkWidget *mb_view_states;
    GtkWidget *mb_view_notebook;
    GtkWidget *mb_view_stats;
    GtkWidget *mb_about;
    GtkWidget *mb_quit;
    GtkWidget *mb_down_queue;
    GtkWidget *mb_up_queue;
    GtkWidget *mb_bottom_queue;
    GtkWidget *mb_top_queue;
    GtkWidget *mb_start_now;
    GtkWidget *mb_queues_seperator;
    GtkEventController *accel_controller;
    TrgPrefs *prefs;
    TrgMainWindow *main_window;
    TrgTorrentTreeView *torrent_tree_view;
    TrgMenu *file_menu;
    TrgMenu *torrent_menu;
    TrgMenu *options_menu;
    TrgMenu *view_menu;
    TrgMenu *help_menu;
};

void trg_menu_bar_set_supports_queues(TrgMenuBar *mb, gboolean supportsQueues)
{
    TrgMenuBarPrivate *priv = TRG_MENU_BAR_GET_PRIVATE(mb);

    gtk_widget_set_visible(priv->mb_down_queue, supportsQueues);
    gtk_widget_set_visible(priv->mb_up_queue, supportsQueues);
    gtk_widget_set_visible(priv->mb_top_queue, supportsQueues);
    gtk_widget_set_visible(priv->mb_bottom_queue, supportsQueues);
    gtk_widget_set_visible(priv->mb_queues_seperator, supportsQueues);
    gtk_widget_set_visible(priv->mb_start_now, supportsQueues);
}

void trg_menu_bar_connected_change(TrgMenuBar *mb, gboolean connected)
{
    TrgMenuBarPrivate *priv = TRG_MENU_BAR_GET_PRIVATE(mb);

    gtk_widget_set_sensitive(priv->mb_add, connected);
    gtk_widget_set_sensitive(priv->mb_add_url, connected);
    gtk_widget_set_sensitive(priv->mb_disconnect, connected);
    gtk_widget_set_sensitive(priv->mb_remote_prefs, connected);
    gtk_widget_set_sensitive(priv->mb_view_stats, connected);
    gtk_widget_set_sensitive(priv->mb_resume_all, connected);
    gtk_widget_set_sensitive(priv->mb_pause_all, connected);
}

void trg_menu_bar_torrent_actions_sensitive(TrgMenuBar *mb, gboolean sensitive)
{
    TrgMenuBarPrivate *priv = TRG_MENU_BAR_GET_PRIVATE(mb);

    gtk_widget_set_sensitive(priv->mb_props, sensitive);
    gtk_widget_set_sensitive(priv->mb_copy_magnetlink, sensitive);
    gtk_widget_set_sensitive(priv->mb_remove, sensitive);
    gtk_widget_set_sensitive(priv->mb_delete, sensitive);
    gtk_widget_set_sensitive(priv->mb_resume, sensitive);
    gtk_widget_set_sensitive(priv->mb_pause, sensitive);
    gtk_widget_set_sensitive(priv->mb_verify, sensitive);
    gtk_widget_set_sensitive(priv->mb_reannounce, sensitive);
    gtk_widget_set_sensitive(priv->mb_move, sensitive);
    gtk_widget_set_sensitive(priv->mb_start_now, sensitive);
    gtk_widget_set_sensitive(priv->mb_up_queue, sensitive);
    gtk_widget_set_sensitive(priv->mb_down_queue, sensitive);
    gtk_widget_set_sensitive(priv->mb_top_queue, sensitive);
    gtk_widget_set_sensitive(priv->mb_bottom_queue, sensitive);
}

static void trg_menu_bar_set_property(GObject *object, guint prop_id, const GValue *value,
                                      GParamSpec *pspec G_GNUC_UNUSED)
{
    TrgMenuBarPrivate *priv = TRG_MENU_BAR_GET_PRIVATE(object);

    switch (prop_id) {
    case PROP_PREFS:
        priv->prefs = g_value_get_object(value);
        break;
    case PROP_MAIN_WINDOW:
        priv->main_window = g_value_get_object(value);
        break;
    case PROP_TORRENT_TREE_VIEW:
        priv->torrent_tree_view = g_value_get_object(value);
        break;
    default:
        break;
    }
}

static void trg_menu_bar_get_property(GObject *object, guint property_id, GValue *value,
                                      GParamSpec *pspec)
{
    TrgMenuBarPrivate *priv = TRG_MENU_BAR_GET_PRIVATE(object);
    switch (property_id) {
    case PROP_CONNECT_BUTTON:
        g_value_set_object(value, priv->mb_connect);
        break;
    case PROP_DISCONNECT_BUTTON:
        g_value_set_object(value, priv->mb_disconnect);
        break;
    case PROP_ADD_BUTTON:
        g_value_set_object(value, priv->mb_add);
        break;
    case PROP_ADD_URL_BUTTON:
        g_value_set_object(value, priv->mb_add_url);
        break;
    case PROP_REMOVE_BUTTON:
        g_value_set_object(value, priv->mb_remove);
        break;
    case PROP_DELETE_BUTTON:
        g_value_set_object(value, priv->mb_delete);
        break;
    case PROP_MOVE_UP_QUEUE:
        g_value_set_object(value, priv->mb_up_queue);
        break;
    case PROP_MOVE_DOWN_QUEUE:
        g_value_set_object(value, priv->mb_down_queue);
        break;
    case PROP_MOVE_TOP_QUEUE:
        g_value_set_object(value, priv->mb_top_queue);
        break;
    case PROP_MOVE_BOTTOM_QUEUE:
        g_value_set_object(value, priv->mb_bottom_queue);
        break;
    case PROP_START_NOW:
        g_value_set_object(value, priv->mb_start_now);
        break;
    case PROP_MOVE_BUTTON:
        g_value_set_object(value, priv->mb_move);
        break;
    case PROP_RESUME_BUTTON:
        g_value_set_object(value, priv->mb_resume);
        break;
    case PROP_RESUME_ALL_BUTTON:
        g_value_set_object(value, priv->mb_resume_all);
        break;
    case PROP_PAUSE_BUTTON:
        g_value_set_object(value, priv->mb_pause);
        break;
    case PROP_PAUSE_ALL_BUTTON:
        g_value_set_object(value, priv->mb_pause_all);
        break;
    case PROP_VERIFY_BUTTON:
        g_value_set_object(value, priv->mb_verify);
        break;
    case PROP_REANNOUNCE_BUTTON:
        g_value_set_object(value, priv->mb_reannounce);
        break;
    case PROP_PROPS_BUTTON:
        g_value_set_object(value, priv->mb_props);
        break;
    case PROP_COPYMAGNET_BUTTON:
        g_value_set_object(value, priv->mb_copy_magnetlink);
        break;
    case PROP_REMOTE_PREFS_BUTTON:
        g_value_set_object(value, priv->mb_remote_prefs);
        break;
    case PROP_LOCAL_PREFS_BUTTON:
        g_value_set_object(value, priv->mb_local_prefs);
        break;
    case PROP_ABOUT_BUTTON:
        g_value_set_object(value, priv->mb_about);
        break;
    case PROP_VIEW_NOTEBOOK_BUTTON:
        g_value_set_object(value, priv->mb_view_notebook);
        break;
    case PROP_VIEW_STATS_BUTTON:
        g_value_set_object(value, priv->mb_view_stats);
        break;
    case PROP_QUIT:
        g_value_set_object(value, priv->mb_quit);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void trg_menu_bar_install_widget_prop(GObjectClass *class, guint propId, const gchar *name,
                                             const gchar *nick)
{
    g_object_class_install_property(class, propId,
                                    g_param_spec_object(name, nick, nick, GTK_TYPE_WIDGET,
                                                        G_PARAM_READABLE | G_PARAM_STATIC_NAME
                                                            | G_PARAM_STATIC_NICK
                                                            | G_PARAM_STATIC_BLURB));
}

GtkWidget *trg_menu_bar_item_new(TrgMenu *menu, const gchar *text, gboolean sensitive)
{
    return trg_menu_append_item(menu, text, NULL, sensitive);
}

static gboolean trg_menu_bar_accel_cb(GtkWidget *widget, GVariant *args, gpointer user_data)
{
    GtkWidget *item = GTK_WIDGET(user_data);

    (void)widget;
    (void)args;

    if (GTK_IS_CHECK_BUTTON(item))
        gtk_check_button_set_active(GTK_CHECK_BUTTON(item),
                                    !gtk_check_button_get_active(GTK_CHECK_BUTTON(item)));
    else
        g_signal_emit_by_name(item, "trg-activate");

    return TRUE;
}

static void trg_menu_bar_accel_add(TrgMenuBar *menu, GtkWidget *item, guint key,
                                   GdkModifierType mods)
{
    TrgMenuBarPrivate *priv = TRG_MENU_BAR_GET_PRIVATE(menu);
    GtkShortcut *shortcut = gtk_shortcut_new(gtk_keyval_trigger_new(key, mods),
                                             gtk_callback_action_new(trg_menu_bar_accel_cb, item,
                                                                     NULL));

    gtk_shortcut_controller_add_shortcut(GTK_SHORTCUT_CONTROLLER(priv->accel_controller), shortcut);
}

static void view_menu_item_toggled_cb(GtkCheckButton *w, gpointer data)
{
    TrgPrefs *p = TRG_PREFS(data);
    const gchar *key = (gchar *)g_object_get_data(G_OBJECT(w), G_DATAKEY_CONF_KEY);

    trg_prefs_set_bool(p, key, gtk_check_button_get_active(w), TRG_PREFS_GLOBAL);
}

static void view_menu_bar_toggled_dependency_cb(GtkCheckButton *w, gpointer data)
{
    if (GTK_IS_WIDGET(data))
        gtk_widget_set_sensitive(GTK_WIDGET(data), gtk_check_button_get_active(w));
}

static void trg_menu_bar_view_item_update(TrgPrefs *p, const gchar *updatedKey, gpointer data)
{
    const gchar *key = (gchar *)g_object_get_data(G_OBJECT(data), G_DATAKEY_CONF_KEY);
    if (!g_strcmp0(updatedKey, key) && GTK_IS_CHECK_BUTTON(data))
        gtk_check_button_set_active(GTK_CHECK_BUTTON(data),
                                    trg_prefs_get_bool(p, key, TRG_PREFS_GLOBAL));
}

static GtkWidget *trg_menu_bar_view_item_new(TrgPrefs *prefs, const gchar *key, const gchar *label,
                                             GtkWidget *dependency)
{
    GtkWidget *w = trg_menu_check_new(label, trg_prefs_get_bool(prefs, key, TRG_PREFS_GLOBAL));
    g_object_set_data_full(G_OBJECT(w), G_DATAKEY_CONF_KEY, g_strdup(key), g_free);

    if (dependency) {
        gtk_widget_set_sensitive(w, gtk_check_button_get_active(GTK_CHECK_BUTTON(dependency)));
        g_signal_connect(dependency, "toggled", G_CALLBACK(view_menu_bar_toggled_dependency_cb), w);
    }

    g_signal_connect(w, "toggled", G_CALLBACK(view_menu_item_toggled_cb), prefs);
    g_signal_connect(prefs, "pref-changed", G_CALLBACK(trg_menu_bar_view_item_update), w);

    return w;
}

static TrgMenu *trg_menu_bar_view_menu_new(TrgMenuBar *mb)
{
    TrgMenuBarPrivate *priv = TRG_MENU_BAR_GET_PRIVATE(mb);
    TrgMenu *viewMenu = trg_menu_new();
    TrgMenu *sortMenu = trg_tree_view_sort_menu(TRG_TREE_VIEW(priv->torrent_tree_view));

    trg_menu_append_submenu(viewMenu, _("Sort"), NULL, sortMenu);

    priv->mb_view_notebook = trg_menu_bar_view_item_new(priv->prefs, TRG_PREFS_KEY_SHOW_NOTEBOOK,
                                                        _("Torrent Details"), NULL);
    trg_menu_bar_accel_add(mb, priv->mb_view_notebook, GDK_KEY_F5, 0);
    trg_menu_append_widget(viewMenu, priv->mb_view_notebook);

    priv->mb_view_stats = trg_menu_item_new(_("_Statistics"), NULL);
    trg_menu_bar_accel_add(mb, priv->mb_view_stats, GDK_KEY_F7, 0);
    gtk_widget_set_sensitive(priv->mb_view_stats, FALSE);
    trg_menu_append_widget(viewMenu, priv->mb_view_stats);

    return viewMenu;
}

static TrgMenu *trg_menu_bar_options_menu_new(TrgMenuBar *menu)
{
    TrgMenuBarPrivate *priv = TRG_MENU_BAR_GET_PRIVATE(menu);
    TrgMenu *optsMenu = trg_menu_new();

    priv->mb_local_prefs = trg_menu_append_item(optsMenu, _("_Local Preferences"), NULL, TRUE);
    trg_menu_bar_accel_add(menu, priv->mb_local_prefs, GDK_KEY_s, GDK_CONTROL_MASK);

    priv->mb_remote_prefs = trg_menu_append_item(optsMenu, _("_Remote Preferences"), NULL, FALSE);
    trg_menu_bar_accel_add(menu, priv->mb_remote_prefs, GDK_KEY_s, GDK_ALT_MASK);

    return optsMenu;
}

static void trg_menu_bar_file_connect_item_new(TrgMainWindow *win, TrgMenu *menu,
                                               const gchar *text, gboolean checked,
                                               JsonObject *profile)
{
    GtkWidget *item = trg_menu_append_check(menu, text, checked);

    g_object_set_data(G_OBJECT(item), "profile", profile);
    g_signal_connect(G_OBJECT(item), "toggled", G_CALLBACK(connect_cb), win);
}

TrgMenu *trg_menu_bar_file_connect_menu_new(TrgMainWindow *win, TrgPrefs *p)
{
    TrgMenu *menu = trg_menu_new();
    GList *profiles = json_array_get_elements(trg_prefs_get_profiles(p));
    JsonObject *currentProfile = trg_prefs_get_profile(p);
    GList *li;

    for (li = profiles; li; li = g_list_next(li)) {
        JsonObject *profile = json_node_get_object((JsonNode *)li->data);
        const gchar *name_value;

        if (json_object_has_member(profile, TRG_PREFS_KEY_PROFILE_NAME)) {
            name_value = json_object_get_string_member(profile, TRG_PREFS_KEY_PROFILE_NAME);
        } else {
            name_value = _(TRG_PROFILE_NAME_DEFAULT);
        }

        trg_menu_bar_file_connect_item_new(win, menu, name_value, profile == currentProfile,
                                           profile);
    }

    g_list_free(profiles);

    return menu;
}

static TrgMenu *trg_menu_bar_file_file_menu_new(TrgMenuBar *menu)
{
    TrgMenuBarPrivate *priv = TRG_MENU_BAR_GET_PRIVATE(menu);
    TrgMenu *fileMenu = trg_menu_new();
    TrgMenu *connectMenu = trg_menu_bar_file_connect_menu_new(priv->main_window, priv->prefs);

    priv->mb_connect = trg_menu_append_submenu(fileMenu, _("Connect"), NULL, connectMenu);

    priv->mb_disconnect = trg_menu_append_item(fileMenu, _("_Disconnect"), NULL, FALSE);
    trg_menu_bar_accel_add(menu, priv->mb_disconnect, GDK_KEY_d, GDK_CONTROL_MASK);

    priv->mb_add = trg_menu_append_item(fileMenu, _("_Add"), NULL, FALSE);
    trg_menu_bar_accel_add(menu, priv->mb_add, GDK_KEY_o, GDK_CONTROL_MASK);

    priv->mb_add_url = trg_menu_append_item(fileMenu, _("Add from _URL"), NULL, FALSE);
    trg_menu_bar_accel_add(menu, priv->mb_add_url, GDK_KEY_u, GDK_CONTROL_MASK);

    priv->mb_quit = trg_menu_append_item(fileMenu, _("_Quit"), NULL, TRUE);
    trg_menu_bar_accel_add(menu, priv->mb_quit, GDK_KEY_q, GDK_CONTROL_MASK);

    return fileMenu;
}

static TrgMenu *trg_menu_bar_torrent_menu_new(TrgMenuBar *menu)
{
    TrgMenuBarPrivate *priv = TRG_MENU_BAR_GET_PRIVATE(menu);
    TrgMenu *torrentMenu = trg_menu_new();

    priv->mb_props = trg_menu_append_item(torrentMenu, _("Properties"), NULL, FALSE);
    trg_menu_bar_accel_add(menu, priv->mb_props, GDK_KEY_i, GDK_CONTROL_MASK);

    priv->mb_copy_magnetlink
        = trg_menu_append_item(torrentMenu, _("_Copy Magnet Link"), NULL, FALSE);
    trg_menu_bar_accel_add(menu, priv->mb_copy_magnetlink, GDK_KEY_c, GDK_CONTROL_MASK);

    priv->mb_resume = trg_menu_append_item(torrentMenu, _("_Resume"), NULL, FALSE);
    trg_menu_bar_accel_add(menu, priv->mb_resume, GDK_KEY_r, GDK_CONTROL_MASK);

    priv->mb_pause = trg_menu_append_item(torrentMenu, _("_Pause"), NULL, FALSE);
    trg_menu_bar_accel_add(menu, priv->mb_pause, GDK_KEY_p, GDK_CONTROL_MASK);

    priv->mb_verify = trg_menu_append_item(torrentMenu, _("_Verify"), NULL, FALSE);
    trg_menu_bar_accel_add(menu, priv->mb_verify, GDK_KEY_h, GDK_CONTROL_MASK);

    priv->mb_reannounce = trg_menu_append_item(torrentMenu, _("Re-_announce"), NULL, FALSE);
    trg_menu_bar_accel_add(menu, priv->mb_reannounce, GDK_KEY_q, GDK_CONTROL_MASK);

    priv->mb_move = trg_menu_append_item(torrentMenu, _("_Move"), NULL, FALSE);
    trg_menu_bar_accel_add(menu, priv->mb_move, GDK_KEY_m, GDK_CONTROL_MASK);

    priv->mb_remove = trg_menu_append_item(torrentMenu, _("Remove"), NULL, FALSE);
    trg_menu_bar_accel_add(menu, priv->mb_remove, GDK_KEY_Delete, 0);

    priv->mb_delete = trg_menu_append_item(torrentMenu, _("Remove and delete data"), NULL, FALSE);
    trg_menu_bar_accel_add(menu, priv->mb_delete, GDK_KEY_Delete, GDK_SHIFT_MASK);

    priv->mb_queues_seperator = trg_menu_append_separator(torrentMenu);

    priv->mb_start_now = trg_menu_append_item(torrentMenu, _("Start Now"), NULL, FALSE);
    priv->mb_up_queue = trg_menu_append_item(torrentMenu, _("Move Up Queue"), NULL, FALSE);
    priv->mb_down_queue = trg_menu_append_item(torrentMenu, _("Move Down Queue"), NULL, FALSE);
    priv->mb_bottom_queue = trg_menu_append_item(torrentMenu, _("Bottom Of Queue"), NULL, FALSE);
    priv->mb_top_queue = trg_menu_append_item(torrentMenu, _("Top Of Queue"), NULL, FALSE);

    trg_menu_append_separator(torrentMenu);

    priv->mb_resume_all = trg_menu_append_item(torrentMenu, _("_Resume All"), NULL, FALSE);
    trg_menu_bar_accel_add(menu, priv->mb_resume_all, GDK_KEY_r, GDK_SHIFT_MASK | GDK_CONTROL_MASK);

    priv->mb_pause_all = trg_menu_append_item(torrentMenu, _("_Pause All"), NULL, FALSE);
    trg_menu_bar_accel_add(menu, priv->mb_pause_all, GDK_KEY_p, GDK_SHIFT_MASK | GDK_CONTROL_MASK);

    return torrentMenu;
}

static TrgMenu *trg_menu_bar_help_menu_new(TrgMenuBar *menuBar)
{
    TrgMenuBarPrivate *priv = TRG_MENU_BAR_GET_PRIVATE(menuBar);
    TrgMenu *helpMenu = trg_menu_new();

    priv->mb_about = trg_menu_append_item(helpMenu, _("_About"), NULL, TRUE);

    return helpMenu;
}

static TrgMenu *trg_menu_bar_app_menu_new(TrgMenuBar *menuBar)
{
    TrgMenuBarPrivate *priv = TRG_MENU_BAR_GET_PRIVATE(menuBar);
    TrgMenu *appMenu = trg_menu_new();

    trg_menu_append_submenu(appMenu, _("_File"), NULL, priv->file_menu);
    trg_menu_append_submenu(appMenu, _("_Torrent"), NULL, priv->torrent_menu);
    trg_menu_append_submenu(appMenu, _("_Options"), NULL, priv->options_menu);
    trg_menu_append_submenu(appMenu, _("_View"), NULL, priv->view_menu);
    trg_menu_append_submenu(appMenu, _("_Help"), NULL, priv->help_menu);

    return appMenu;
}

GtkWidget *trg_menu_bar_app_menu_button_new(TrgMenuBar *menuBar)
{
    TrgMenu *appMenu = trg_menu_bar_app_menu_new(menuBar);
    GtkWidget *button = gtk_menu_button_new();
    GtkWidget *image = gtk_image_new_from_icon_name("open-menu-symbolic");

    gtk_image_set_pixel_size(GTK_IMAGE(image), 16);
    gtk_menu_button_set_child(GTK_MENU_BUTTON(button), image);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(button), trg_menu_get_widget(appMenu));
    gtk_widget_add_css_class(button, "flat");
    gtk_widget_set_tooltip_text(button, _("Application Menu"));

    return button;
}

static void menu_bar_refresh_menu(GtkWidget *w, gpointer data)
{
    TrgMenuBarPrivate *priv = TRG_MENU_BAR_GET_PRIVATE(data);
    TrgMenu *new_menu = trg_menu_bar_file_connect_menu_new(priv->main_window, priv->prefs);
    GtkPopover *old = gtk_menu_button_get_popover(GTK_MENU_BUTTON(priv->mb_connect));

    (void)w;

    if (old != NULL && gtk_widget_get_visible(GTK_WIDGET(old)))
        gtk_popover_popdown(old);

    gtk_menu_button_set_popover(GTK_MENU_BUTTON(priv->mb_connect),
                                trg_menu_get_widget(new_menu));
}

static GObject *trg_menu_bar_constructor(GType type, guint n_construct_properties,
                                         GObjectConstructParam *construct_params)
{
    GObject *object;
    TrgMenuBarPrivate *priv;
    TrgMenuBar *menu;

    object = G_OBJECT_CLASS(trg_menu_bar_parent_class)
                 ->constructor(type, n_construct_properties, construct_params);
    menu = TRG_MENU_BAR(object);
    priv = TRG_MENU_BAR_GET_PRIVATE(object);

    gtk_orientable_set_orientation(GTK_ORIENTABLE(object), GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_add_css_class(GTK_WIDGET(object), "menubar");

    priv->accel_controller = gtk_shortcut_controller_new();
    gtk_shortcut_controller_set_scope(GTK_SHORTCUT_CONTROLLER(priv->accel_controller),
                                      GTK_SHORTCUT_SCOPE_GLOBAL);
    gtk_widget_add_controller(GTK_WIDGET(object), priv->accel_controller);

    priv->file_menu = trg_menu_bar_file_file_menu_new(menu);
    priv->torrent_menu = trg_menu_bar_torrent_menu_new(menu);
    priv->options_menu = trg_menu_bar_options_menu_new(menu);
    priv->view_menu = trg_menu_bar_view_menu_new(menu);
    priv->help_menu = trg_menu_bar_help_menu_new(menu);

    /* Menu bar is hidden, so don't add buttons to it. Menus are used by app menu button instead. */

    g_signal_connect(G_OBJECT(priv->prefs), "pref-profile-changed",
                     G_CALLBACK(menu_bar_refresh_menu), object);

    return object;
}

static void trg_menu_bar_class_init(TrgMenuBarClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = trg_menu_bar_get_property;
    object_class->set_property = trg_menu_bar_set_property;
    object_class->constructor = trg_menu_bar_constructor;

    g_type_class_add_private(klass, sizeof(TrgMenuBarPrivate));

    trg_menu_bar_install_widget_prop(object_class, PROP_CONNECT_BUTTON, "connect-button",
                                     "Connect Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_DISCONNECT_BUTTON, "disconnect-button",
                                     "Disconnect Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_ADD_BUTTON, "add-button", "Add Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_ADD_URL_BUTTON, "add-url-button",
                                     "Add URL Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_REMOVE_BUTTON, "remove-button",
                                     "Remove Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_MOVE_BUTTON, "move-button", "Move Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_DELETE_BUTTON, "delete-button",
                                     "Delete Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_RESUME_BUTTON, "resume-button",
                                     "Resume Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_RESUME_ALL_BUTTON, "resume-all-button",
                                     "Resume All Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_VERIFY_BUTTON, "verify-button",
                                     "Verify Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_REANNOUNCE_BUTTON, "reannounce-button",
                                     "Re-announce Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_PAUSE_ALL_BUTTON, "pause-all-button",
                                     "Pause All Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_PAUSE_BUTTON, "pause-button",
                                     "Pause Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_PROPS_BUTTON, "props-button",
                                     "Props Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_COPYMAGNET_BUTTON, "copymagnet-button",
                                     "Copy-magnet Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_ABOUT_BUTTON, "about-button",
                                     "About Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_VIEW_STATS_BUTTON, "view-stats-button",
                                     "View stats button");
    trg_menu_bar_install_widget_prop(object_class, PROP_VIEW_NOTEBOOK_BUTTON,
                                     "view-notebook-button", "View notebook Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_REMOTE_PREFS_BUTTON, "remote-prefs-button",
                                     "Remote Prefs Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_LOCAL_PREFS_BUTTON, "local-prefs-button",
                                     "Local Prefs Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_QUIT, "quit-button", "Quit Button");
    trg_menu_bar_install_widget_prop(object_class, PROP_MOVE_DOWN_QUEUE, "down-queue",
                                     "Down Queue");
    trg_menu_bar_install_widget_prop(object_class, PROP_MOVE_UP_QUEUE, "up-queue", "Up Queue");
    trg_menu_bar_install_widget_prop(object_class, PROP_MOVE_BOTTOM_QUEUE, "bottom-queue",
                                     "Bottom Queue");
    trg_menu_bar_install_widget_prop(object_class, PROP_MOVE_TOP_QUEUE, "top-queue", "Top Queue");
    trg_menu_bar_install_widget_prop(object_class, PROP_START_NOW, "start-now", "Start Now");

    g_object_class_install_property(
        object_class, PROP_PREFS,
        g_param_spec_object("prefs", "prefs", "Prefs", TRG_TYPE_PREFS,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME
                                | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));

    g_object_class_install_property(
        object_class, PROP_MAIN_WINDOW,
        g_param_spec_object("mainwin", "mainwin", "mainwin", TRG_TYPE_MAIN_WINDOW,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME
                                | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));

    g_object_class_install_property(
        object_class, PROP_TORRENT_TREE_VIEW,
        g_param_spec_object("torrent-tree-view", "torrent-tree-view", "torrent-tree-view",
                            TRG_TYPE_TORRENT_TREE_VIEW,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME
                                | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
}

static void trg_menu_bar_init(TrgMenuBar *self)
{
}

TrgMenuBar *trg_menu_bar_new(TrgMainWindow *win, TrgPrefs *prefs, TrgTorrentTreeView *ttv)
{
    return g_object_new(TRG_TYPE_MENU_BAR, "torrent-tree-view", ttv, "prefs", prefs, "mainwin", win,
                        NULL);
}
