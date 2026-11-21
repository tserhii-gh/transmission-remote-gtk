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

#include "trg-menu.h"

#include "trg-compat.h"

#include <gdk/gdkkeysyms.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "json.h"
#include "protocol-constants.h"
#include "remote-exec.h"
#include "requests.h"
#include "session-get.h"
#include "torrent.h"
#include "trg-client.h"
#include "util.h"

#include "trg-about-window.h"
#include "trg-files-model.h"
#include "trg-files-tree-view.h"
#include "trg-main-window.h"
#include "trg-menu-bar.h"
#include "trg-peers-model.h"
#include "trg-peers-tree-view.h"
#include "trg-preferences-dialog.h"
#include "trg-prefs.h"
#include "trg-remote-prefs-dialog.h"
#include "trg-sortable-filtered-model.h"
#include "trg-stats-dialog.h"
#include "trg-status-bar.h"
#include "trg-toolbar.h"
#include "trg-torrent-add-dialog.h"
#include "trg-torrent-add-url-dialog.h"
#include "trg-torrent-model.h"
#include "trg-torrent-move-dialog.h"
#include "trg-torrent-props-dialog.h"
#include "trg-torrent-tree-view.h"
#include "trg-trackers-model.h"
#include "trg-trackers-tree-view.h"
#include "trg-tree-view.h"
#include "upload.h"

/* The rather large main window class, which glues everything together. */

static void update_selected_torrent_notebook(TrgMainWindow *win, gint mode, gint64 id);
static void torrent_event_notification(TrgTorrentModel *model, gchar *icon_name, gchar *desc,
                                       gchar *prefKey, GtkTreeIter *iter, gpointer data);
static void on_torrent_completed(TrgTorrentModel *model, GtkTreeIter *iter, gpointer data);
static void on_torrent_added(TrgTorrentModel *model, GtkTreeIter *iter, gpointer data);
static gboolean delete_event(GtkWindow *w, gpointer data);
static void destroy_window(TrgMainWindow *win, gpointer data G_GNUC_UNUSED);
static void torrent_tv_onRowActivated(GtkTreeView *treeview, GtkTreePath *path,
                                      GtkTreeViewColumn *col, gpointer userdata);
static void add_url_cb(GtkWidget *w, gpointer data);
static void add_cb(GtkWidget *w, gpointer data);
static void disconnect_cb(GtkWidget *w, gpointer data);
static void open_local_prefs_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win);
static void open_remote_prefs_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win);
static TrgToolbar *trg_main_window_toolbar_new(TrgMainWindow *win);
static void verify_cb(GtkWidget *w, TrgMainWindow *win);
static void reannounce_cb(GtkWidget *w, TrgMainWindow *win);
static void pause_cb(GtkWidget *w, TrgMainWindow *win);
static void resume_cb(GtkWidget *w, TrgMainWindow *win);
static void remove_cb(GtkWidget *w, TrgMainWindow *win);
static void resume_all_cb(GtkWidget *w, TrgMainWindow *win);
static void pause_all_cb(GtkWidget *w, TrgMainWindow *win);
static void move_cb(GtkWidget *w, TrgMainWindow *win);
static void delete_cb(GtkWidget *w, TrgMainWindow *win);
static void open_props_cb(GtkWidget *w, TrgMainWindow *win);
static gint confirm_action_dialog(GtkWindow *gtk_win, GtkTreeSelection *selection,
                                  const gchar *action_name, const gchar *action_label);
static void view_stats_toggled_cb(GtkWidget *w, gpointer data);
static void view_notebook_toggled_cb(GtkCheckButton *w, TrgMainWindow *win);
static GtkWidget *trg_main_window_notebook_new(TrgMainWindow *win);
static gboolean on_session_get_timer(gpointer data);
static gboolean on_session_get(gpointer data);
static gboolean on_torrent_get(gpointer data, int mode);
static gboolean on_torrent_get_first(gpointer data);
static gboolean on_torrent_get_active(gpointer data);
static gboolean on_torrent_get_update(gpointer data);
static gboolean on_torrent_get_interactive(gpointer data);
static gboolean trg_session_update_timerfunc(gpointer data);
static gboolean trg_update_torrents_timerfunc(gpointer data);
static void open_about_cb(GtkWidget *w, GtkWindow *parent);
static gboolean trg_torrent_tree_view_visible_func(GtkTreeModel *model, GtkTreeIter *iter,
                                                   gpointer data);
static TrgTorrentTreeView *trg_main_window_torrent_tree_view_new(TrgMainWindow *win,
                                                                 GtkTreeModel *model);
static gboolean trg_dialog_error_handler(TrgMainWindow *win, trg_response *response);
static gboolean torrent_selection_changed(GtkTreeSelection *selection, TrgMainWindow *win);
static void trg_main_window_torrent_scrub(TrgMainWindow *win);
static void trg_main_window_conn_changed(TrgMainWindow *win, gboolean connected);
static void trg_main_window_get_property(GObject *object, guint property_id, GValue *value,
                                         GParamSpec *pspec);
static void trg_main_window_set_property(GObject *object, guint property_id, const GValue *value,
                                         GParamSpec *pspec);
static void quit_cb(GtkWidget *w, gpointer data);
static TrgMenuBar *trg_main_window_menu_bar_new(TrgMainWindow *win);
static void set_limit_cb(GtkWidget *w, TrgMainWindow *win);
static TrgMenu *limit_menu_new(TrgMainWindow *win, gchar *enabledKey, gchar *speedKey,
                               JsonArray *ids);
static void trg_torrent_tv_view_menu(GtkWidget *treeview, double x, double y, gboolean at_pointer,
                                     TrgMainWindow *win);
static void torrent_tv_button_pressed_cb(GtkWidget *treeview, int button, double x, double y,
                                         GdkModifierType state, gpointer userdata);
static gboolean is_ready_for_torrent_action(TrgMainWindow *win);

struct _TrgMainWindow {
    GtkApplicationWindow parent;
};

typedef struct {
    TrgClient *client;
    TrgToolbar *toolBar;
    TrgMenuBar *menuBar;

    TrgStatusBar *statusBar;
    GtkWidget *iconStatusItem, *iconDownloadingItem, *iconSeedingItem, *iconSepItem;
    TrgGeneralPanel *genDetails;
    GtkWidget *notebook;

    TrgTorrentModel *torrentModel;
    TrgTorrentTreeView *torrentTreeView;
    GtkTreeModel *filteredTorrentModel;
    GtkTreeModel *sortedTorrentModel;
    gint selectedTorrentId;

    TrgTrackersModel *trackersModel;
    TrgTrackersTreeView *trackersTreeView;

    TrgFilesModel *filesModel;
    TrgFilesTreeView *filesTreeView;

    TrgPeersModel *peersModel;
    TrgPeersTreeView *peersTreeView;

    GtkWidget *hpaned, *vpaned;
    GtkWidget *filterEntry;

    gboolean hidden;
    gint width, height;
    guint timerId;
    guint sessionTimerId;
    gboolean min_on_start;
    gboolean queuesEnabled;

    gchar **args;
} TrgMainWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(TrgMainWindow, trg_main_window, GTK_TYPE_WINDOW)

enum {
    PROP_0,
    PROP_CLIENT,
};

static void reset_connect_args(TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    if (priv->args) {
        g_strfreev(priv->args);
        priv->args = NULL;
    }
}

static void trg_main_window_init(TrgMainWindow *self)
{
}

gint trg_mw_get_selected_torrent_id(TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    return priv->selectedTorrentId;
}

static void update_selected_torrent_notebook(TrgMainWindow *win, gint mode, gint64 id)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgClient *client = priv->client;
    gint64 serial = trg_client_get_serial(client);
    JsonObject *t;
    GtkTreeIter iter;

    if (id >= 0 && get_torrent_data(trg_client_get_torrent_table(client), id, &t, &iter)) {
        trg_toolbar_torrent_actions_sensitive(priv->toolBar, TRUE);
        trg_menu_bar_torrent_actions_sensitive(priv->menuBar, TRUE);
        trg_general_panel_update(priv->genDetails, t, &iter);
        trg_trackers_model_update(priv->trackersModel, serial, t, mode);
        trg_files_model_update(priv->filesModel, GTK_TREE_VIEW(priv->filesTreeView), serial, t,
                               mode);
        trg_peers_model_update(priv->peersModel, TRG_TREE_VIEW(priv->peersTreeView), serial, t,
                               mode);
    } else {
        trg_main_window_torrent_scrub(win);
    }

    priv->selectedTorrentId = id;
}

static void torrent_event_notification(TrgTorrentModel *model, gchar *icon_name, gchar *desc,
                                       gchar *prefKey, GtkTreeIter *iter, gpointer data)
{
    TrgMainWindow *win = TRG_MAIN_WINDOW(data);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgPrefs *prefs = trg_client_get_prefs(priv->client);
    gchar *name;
    GIcon *icon;
    GNotification *notify;

    if (!trg_prefs_get_bool(prefs, prefKey, TRG_PREFS_NOFLAGS))
        return;

    gtk_tree_model_get(GTK_TREE_MODEL(model), iter, TORRENT_COLUMN_NAME, &name, -1);

    icon = g_themed_icon_new(icon_name);
    notify = g_notification_new(name);
    g_notification_set_body(notify, desc);
    g_notification_set_icon(notify, icon);
    g_notification_set_priority(notify, G_NOTIFICATION_PRIORITY_LOW);
    g_application_send_notification(g_application_get_default(), name, notify);

    g_object_unref(notify);
    g_object_unref(icon);
    g_free(name);
}

static void on_torrent_completed(TrgTorrentModel *model, GtkTreeIter *iter, gpointer data)
{
    torrent_event_notification(model, "gtk-apply", _("This torrent has completed."),
                               TRG_PREFS_KEY_COMPLETE_NOTIFY, iter, data);
}

static void on_torrent_added(TrgTorrentModel *model, GtkTreeIter *iter, gpointer data)
{
    torrent_event_notification(model, "list-add", _("This torrent has been added."),
                               TRG_PREFS_KEY_ADD_NOTIFY, iter, data);
}

static gboolean delete_event(GtkWindow *w G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
    return FALSE;
}

static void destroy_window(TrgMainWindow *win, gpointer data G_GNUC_UNUSED)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgPrefs *prefs = trg_client_get_prefs(priv->client);

    priv->width = gtk_widget_get_width(GTK_WIDGET(win));
    priv->height = gtk_widget_get_height(GTK_WIDGET(win));

    trg_prefs_set_int(prefs, TRG_PREFS_KEY_WINDOW_HEIGHT, priv->height, TRG_PREFS_GLOBAL);
    trg_prefs_set_int(prefs, TRG_PREFS_KEY_WINDOW_WIDTH, priv->width, TRG_PREFS_GLOBAL);
    trg_prefs_set_int(prefs, TRG_PREFS_KEY_NOTEBOOK_PANED_POS,
                      gtk_paned_get_position(GTK_PANED(priv->vpaned)), TRG_PREFS_GLOBAL);

    trg_tree_view_persist(TRG_TREE_VIEW(priv->peersTreeView),
                          TRG_TREE_VIEW_PERSIST_SORT | TRG_TREE_VIEW_PERSIST_LAYOUT);
    trg_tree_view_persist(TRG_TREE_VIEW(priv->filesTreeView),
                          TRG_TREE_VIEW_PERSIST_SORT | TRG_TREE_VIEW_PERSIST_LAYOUT);
    trg_tree_view_persist(
        TRG_TREE_VIEW(priv->torrentTreeView),
        TRG_TREE_VIEW_PERSIST_SORT | TRG_TREE_VIEW_SORTABLE_PARENT
            | (trg_prefs_get_int(prefs, TRG_PREFS_KEY_STYLE, TRG_PREFS_GLOBAL) == TRG_STYLE_CLASSIC
                   ? TRG_TREE_VIEW_PERSIST_LAYOUT
                   : 0));
    trg_tree_view_persist(TRG_TREE_VIEW(priv->trackersTreeView),
                          TRG_TREE_VIEW_PERSIST_SORT | TRG_TREE_VIEW_PERSIST_LAYOUT);
    trg_prefs_save(prefs);

    g_application_quit(g_application_get_default());
}

static void open_props_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgTorrentPropsDialog *dialog;

    if (priv->selectedTorrentId < 0)
        return;

    dialog = trg_torrent_props_dialog_new(GTK_WINDOW(win), priv->torrentTreeView,
                                          priv->torrentModel, priv->client);

    trg_widget_show_all(GTK_WIDGET(dialog));
}

static void copy_magnetlink_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    JsonObject *json = NULL;
    GdkClipboard *clip = gdk_display_get_clipboard(gdk_display_get_default());

    if (priv->selectedTorrentId < 0)
        return;

    if (get_torrent_data(trg_client_get_torrent_table(priv->client), priv->selectedTorrentId, &json,
                         NULL))
        gdk_clipboard_set_text(clip, torrent_get_magnetlink(json));
}

static void torrent_tv_onRowActivated(GtkTreeView *treeview, GtkTreePath *path G_GNUC_UNUSED,
                                      GtkTreeViewColumn *col G_GNUC_UNUSED, gpointer userdata)
{
    open_props_cb(GTK_WIDGET(treeview), userdata);
}

static void add_url_cb(GtkWidget *w G_GNUC_UNUSED, gpointer data)
{
    TrgMainWindow *win = TRG_MAIN_WINDOW(data);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    TrgTorrentAddUrlDialog *dlg = trg_torrent_add_url_dialog_new(win, priv->client);
    trg_widget_show_all(GTK_WIDGET(dlg));
}

static void add_cb(GtkWidget *w G_GNUC_UNUSED, gpointer data)
{
    TrgMainWindow *win = TRG_MAIN_WINDOW(data);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (trg_client_is_connected(priv->client))
        trg_torrent_add_dialog(win, priv->client);
}

static void pause_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (trg_client_is_connected(priv->client))
        dispatch_rpc_async(priv->client, torrent_pause(build_json_id_array(priv->torrentTreeView)),
                           on_generic_interactive_action_response, win);
}

static void pause_all_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (trg_client_is_connected(priv->client))
        dispatch_rpc_async(priv->client, torrent_pause(NULL),
                           on_generic_interactive_action_response, win);
}

gint trg_add_from_filename(TrgMainWindow *win, gchar **uris)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgClient *client = priv->client;
    TrgPrefs *prefs = trg_client_get_prefs(client);
    GSList *filesList = NULL;
    int i;

    if (!trg_client_is_connected(client)) {
        g_strfreev(uris);
        return EXIT_SUCCESS;
    }

    if (uris) {
        for (i = 0; uris[i]; i++) {
            if (uris[i])
                filesList = g_slist_append(filesList, uris[i]);
        }
    }

    g_free(uris);

    if (!filesList)
        return EXIT_SUCCESS;

    if (trg_prefs_get_bool(prefs, TRG_PREFS_KEY_ADD_OPTIONS_DIALOG, TRG_PREFS_GLOBAL)) {
        TrgTorrentAddDialog *dialog
            = trg_torrent_add_dialog_new_from_filenames(win, client, filesList);

        trg_widget_show_all(GTK_WIDGET(dialog));
        gtk_window_present(GTK_WINDOW(dialog));
    } else {
        trg_upload *upload = g_new0(trg_upload, 1);

        upload->list = filesList;
        upload->main_window = win;
        upload->client = client;
        upload->extra_args = FALSE;
        upload->flags = trg_prefs_get_add_flags(prefs);

        trg_do_upload(upload);
    }

    return EXIT_SUCCESS;
}

static void resume_all_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (trg_client_is_connected(priv->client))
        dispatch_rpc_async(priv->client, torrent_start(NULL),
                           on_generic_interactive_action_response, win);
}

static void resume_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (trg_client_is_connected(priv->client))
        dispatch_rpc_async(priv->client, torrent_start(build_json_id_array(priv->torrentTreeView)),
                           on_generic_interactive_action_response, win);
}

static void disconnect_cb(GtkWidget *w G_GNUC_UNUSED, gpointer data)
{
    TrgMainWindow *win = TRG_MAIN_WINDOW(data);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    trg_client_inc_connid(priv->client);
    trg_main_window_conn_changed(TRG_MAIN_WINDOW(data), FALSE);
    trg_status_bar_reset(priv->statusBar);
}

void connect_cb(GtkWidget *w, gpointer data)
{
    TrgMainWindow *win = TRG_MAIN_WINDOW(data);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    TrgPrefs *prefs = trg_client_get_prefs(priv->client);
    JsonObject *currentProfile = trg_prefs_get_profile(prefs);
    JsonObject *profile = NULL;
    GtkWidget *dialog;
    g_autofree gchar *err_msg = NULL;

    if (w)
        profile = (JsonObject *)g_object_get_data(G_OBJECT(w), "profile");

    if (trg_client_is_connected(priv->client))
        disconnect_cb(NULL, data);

    if (profile && currentProfile != profile)
        trg_prefs_set_profile(prefs, profile);
    else
        trg_prefs_profile_change_emit_signal(prefs);

    if (!trg_client_parse_settings(priv->client, &err_msg)) {
        dialog = gtk_message_dialog_new(GTK_WINDOW(data), GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", err_msg);
        trg_dialog_run(GTK_DIALOG(dialog));
        trg_widget_destroy(dialog);
        reset_connect_args(TRG_MAIN_WINDOW(data));

        open_local_prefs_cb(NULL, win);

        return;
    }

    trg_status_bar_push_connection_msg(priv->statusBar, _("Connecting..."));
    trg_client_inc_connid(priv->client);
    dispatch_rpc_async(priv->client, session_get(), on_session_get, data);
}

static void open_local_prefs_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    GtkWidget *dlg = trg_preferences_dialog_get_instance(win, priv->client);
    trg_widget_show_all(dlg);
}

static void open_remote_prefs_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (trg_client_is_connected(priv->client))
        trg_widget_show_all(GTK_WIDGET(trg_remote_prefs_dialog_get_instance(win, priv->client)));
}

static TrgToolbar *trg_main_window_toolbar_new(TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgPrefs *prefs = trg_client_get_prefs(priv->client);

    GObject *b_connect, *b_disconnect, *b_add, *b_resume, *b_pause;
    GObject *b_remove, *b_delete, *b_props, *b_local_prefs, *b_remote_prefs;

    TrgToolbar *toolBar = trg_toolbar_new(win, prefs);

    g_object_get(toolBar, "connect-button", &b_connect, "disconnect-button", &b_disconnect,
                 "add-button", &b_add, "resume-button", &b_resume, "pause-button", &b_pause,
                 "delete-button", &b_delete, "remove-button", &b_remove, "props-button", &b_props,
                 "remote-prefs-button", &b_remote_prefs, "local-prefs-button", &b_local_prefs,
                 NULL);

    g_signal_connect(b_connect, "clicked", G_CALLBACK(connect_cb), win);
    g_signal_connect(b_disconnect, "clicked", G_CALLBACK(disconnect_cb), win);
    g_signal_connect(b_add, "clicked", G_CALLBACK(add_cb), win);
    g_signal_connect(b_resume, "clicked", G_CALLBACK(resume_cb), win);
    g_signal_connect(b_pause, "clicked", G_CALLBACK(pause_cb), win);
    g_signal_connect(b_delete, "clicked", G_CALLBACK(delete_cb), win);
    g_signal_connect(b_remove, "clicked", G_CALLBACK(remove_cb), win);
    g_signal_connect(b_props, "clicked", G_CALLBACK(open_props_cb), win);
    g_signal_connect(b_local_prefs, "clicked", G_CALLBACK(open_local_prefs_cb), win);
    g_signal_connect(b_remote_prefs, "clicked", G_CALLBACK(open_remote_prefs_cb), win);

    return toolBar;
}

static void reannounce_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (trg_client_is_connected(priv->client))
        dispatch_rpc_async(priv->client,
                           torrent_reannounce(build_json_id_array(priv->torrentTreeView)),
                           on_generic_interactive_action_response, win);
}

static void verify_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (is_ready_for_torrent_action(win))
        dispatch_rpc_async(priv->client, torrent_verify(build_json_id_array(priv->torrentTreeView)),
                           on_generic_interactive_action_response, win);
}

static void start_now_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (is_ready_for_torrent_action(win))
        dispatch_rpc_async(priv->client,
                           torrent_start_now(build_json_id_array(priv->torrentTreeView)),
                           on_generic_interactive_action_response, win);
}

static void up_queue_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (priv->queuesEnabled && is_ready_for_torrent_action(win))
        dispatch_rpc_async(priv->client,
                           torrent_queue_move_up(build_json_id_array(priv->torrentTreeView)),
                           on_generic_interactive_action_response, win);
}

static void top_queue_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (priv->queuesEnabled && is_ready_for_torrent_action(win))
        dispatch_rpc_async(priv->client,
                           torrent_queue_move_top(build_json_id_array(priv->torrentTreeView)),
                           on_generic_interactive_action_response, win);
}

static void bottom_queue_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (priv->queuesEnabled && is_ready_for_torrent_action(win))
        dispatch_rpc_async(priv->client,
                           torrent_queue_move_bottom(build_json_id_array(priv->torrentTreeView)),
                           on_generic_interactive_action_response, win);
}

static void down_queue_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (priv->queuesEnabled && is_ready_for_torrent_action(win))
        dispatch_rpc_async(priv->client,
                           torrent_queue_move_down(build_json_id_array(priv->torrentTreeView)),
                           on_generic_interactive_action_response, win);
}

static gint confirm_action_dialog(GtkWindow *gtk_win, GtkTreeSelection *selection,
                                  const gchar *action_name, const gchar *action_label)
{
    TrgMainWindow *win = TRG_MAIN_WINDOW(gtk_win);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    gint selectCount;
    gint response;
    GtkWidget *dialog = NULL;

    selectCount = gtk_tree_selection_count_selected_rows(selection);

    if (selectCount == 1) {
        GList *list;
        GList *firstNode;
        GtkTreeIter firstIter;
        gchar *name = NULL;

        list = gtk_tree_selection_get_selected_rows(selection, NULL);
        firstNode = g_list_first(list);

        gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->filteredTorrentModel), &firstIter,
                                firstNode->data);
        gtk_tree_model_get(GTK_TREE_MODEL(priv->filteredTorrentModel), &firstIter,
                           TORRENT_COLUMN_NAME, &name, -1);
        g_list_free_full(list, (GDestroyNotify)gtk_tree_path_free);

        dialog = gtk_message_dialog_new_with_markup(
            GTK_WINDOW(win), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
            _("<big><b>%s \"%s\"?</b></big>"), action_name, name);
        g_free(name);
    } else if (selectCount > 1) {
        dialog = gtk_message_dialog_new_with_markup(
            GTK_WINDOW(win), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
            _("<big><b>%s %d torrents?</b></big>"), action_name, selectCount);

    } else {
        return 0;
    }

    gtk_dialog_add_buttons(GTK_DIALOG(dialog), _("_Cancel"), GTK_RESPONSE_CANCEL, action_label,
                           GTK_RESPONSE_ACCEPT, NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);

    response = trg_dialog_run(GTK_DIALOG(dialog));
    trg_widget_destroy(dialog);
    return response;
}

static gboolean is_ready_for_torrent_action(TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    return priv->selectedTorrentId >= 0 && trg_client_is_connected(priv->client);
}

static void move_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (is_ready_for_torrent_action(win))
        trg_widget_show_all(
            GTK_WIDGET(trg_torrent_move_dialog_new(win, priv->client, priv->torrentTreeView)));
}

static void remove_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    GtkTreeSelection *selection;
    JsonArray *ids;

    if (!is_ready_for_torrent_action(win))
        return;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->torrentTreeView));
    ids = build_json_id_array(priv->torrentTreeView);

    if (confirm_action_dialog(GTK_WINDOW(win), selection, _("Remove"), _("_Remove"))
        == GTK_RESPONSE_ACCEPT)
        dispatch_rpc_async(priv->client, torrent_remove(ids, FALSE),
                           on_generic_interactive_action_response, win);
    else
        json_array_unref(ids);
}

static void delete_cb(GtkWidget *w G_GNUC_UNUSED, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    GtkTreeSelection *selection;
    JsonArray *ids;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->torrentTreeView));
    ids = build_json_id_array(priv->torrentTreeView);

    if (!is_ready_for_torrent_action(win))
        return;

    if (confirm_action_dialog(GTK_WINDOW(win), selection, _("Remove and delete"), _("_Delete"))
        == GTK_RESPONSE_ACCEPT)
        dispatch_rpc_async(priv->client, torrent_remove(ids, TRUE), on_delete_complete, win);
    else
        json_array_unref(ids);
}

static void view_stats_toggled_cb(GtkWidget *w, gpointer data)
{
    TrgMainWindow *win = TRG_MAIN_WINDOW(data);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (trg_client_is_connected(priv->client)) {
        TrgStatsDialog *dlg = trg_stats_dialog_get_instance(TRG_MAIN_WINDOW(data), priv->client);

        trg_widget_show_all(GTK_WIDGET(dlg));
    }
}

static void view_notebook_toggled_cb(GtkCheckButton *w, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    trg_widget_set_visible(priv->notebook, gtk_check_button_get_active(w));
}

void trg_main_window_notebook_set_visible(TrgMainWindow *win, gboolean visible)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    trg_widget_set_visible(priv->notebook, visible);
}

static GtkWidget *trg_main_window_notebook_new(TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgPrefs *prefs = trg_client_get_prefs(priv->client);

    GtkWidget *notebook = priv->notebook = gtk_notebook_new();
    GtkWidget *genScrolledWin = gtk_scrolled_window_new();

    priv->genDetails = trg_general_panel_new(GTK_TREE_MODEL(priv->torrentModel), priv->client);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(genScrolledWin), GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    trg_container_add(genScrolledWin, GTK_WIDGET(priv->genDetails));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), genScrolledWin, gtk_label_new(_("General")));

    priv->trackersModel = trg_trackers_model_new();
    priv->trackersTreeView
        = trg_trackers_tree_view_new(priv->trackersModel, priv->client, win, NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                             my_scrolledwin_new(GTK_WIDGET(priv->trackersTreeView)),
                             gtk_label_new(_("Trackers")));

    priv->filesModel = trg_files_model_new();
    priv->filesTreeView = trg_files_tree_view_new(priv->filesModel, win, priv->client, NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                             my_scrolledwin_new(GTK_WIDGET(priv->filesTreeView)),
                             gtk_label_new(_("Files")));

    priv->peersModel = trg_peers_model_new();
    priv->peersTreeView = trg_peers_tree_view_new(prefs, priv->peersModel, NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                             my_scrolledwin_new(GTK_WIDGET(priv->peersTreeView)),
                             gtk_label_new(_("Peers")));

    return notebook;
}

gboolean on_session_set(gpointer data)
{
    trg_response *response = (trg_response *)data;
    TrgMainWindow *win = TRG_MAIN_WINDOW(response->cb_data);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (response->status == SOUP_STATUS_OK || response->status == FAIL_RESULT_UNSUCCESSFUL)
        trg_client_update_session(priv->client, on_session_get, response->cb_data);

    trg_dialog_error_handler(TRG_MAIN_WINDOW(response->cb_data), response);
    trg_response_free(response);

    return FALSE;
}

static gboolean hasEnabledChanged(JsonObject *a, JsonObject *b, const gchar *key)
{
    return json_object_get_boolean_member(a, key) != json_object_get_boolean_member(b, key);
}

static gboolean on_session_get_timer(gpointer data)
{
    trg_response *response = (trg_response *)data;
    TrgMainWindow *win = TRG_MAIN_WINDOW(response->cb_data);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgPrefs *prefs = trg_client_get_prefs(priv->client);

    on_session_get(data);

    priv->sessionTimerId = g_timeout_add_seconds(
        trg_prefs_get_int(prefs, TRG_PREFS_KEY_SESSION_UPDATE_INTERVAL, TRG_PREFS_CONNECTION),
        trg_session_update_timerfunc, win);

    return FALSE;
}

static gboolean on_session_get(gpointer data)
{
    trg_response *response = (trg_response *)data;
    TrgMainWindow *win = TRG_MAIN_WINDOW(response->cb_data);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    TrgClient *client = priv->client;
    gboolean isConnected = trg_client_is_connected(client);
    JsonObject *lastSession = trg_client_get_session(client);
    JsonObject *newSession = NULL;

    if (response->obj)
        newSession = get_arguments(response->obj);

    if (!isConnected) {
        gdouble version;

        if (trg_dialog_error_handler(win, response)) {
            trg_response_free(response);
            reset_connect_args(win);
            return FALSE;
        }

        if ((version = session_get_version(newSession)) < TRANSMISSION_MIN_SUPPORTED) {
            gchar *msg = g_strdup_printf(
                _("This application supports Transmission %g and later, you have %g."),
                TRANSMISSION_MIN_SUPPORTED, version);
            GtkWidget *dialog = gtk_message_dialog_new(
                GTK_WINDOW(win), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", msg);
            gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
            trg_dialog_run(GTK_DIALOG(dialog));
            trg_widget_destroy(dialog);
            g_free(msg);
            trg_response_free(response);
            reset_connect_args(win);
            return FALSE;
        }

        trg_status_bar_connect(priv->statusBar, newSession, client);
    }

    if (newSession) {
        gboolean reloadAliases = lastSession
            && g_strcmp0(session_get_download_dir(lastSession),
                         session_get_download_dir(newSession));
        gboolean refreshSpeed = lastSession
            && (hasEnabledChanged(lastSession, newSession, SGET_ALT_SPEED_ENABLED)
                || hasEnabledChanged(lastSession, newSession, SGET_SPEED_LIMIT_DOWN_ENABLED)
                || hasEnabledChanged(lastSession, newSession, SGET_SPEED_LIMIT_UP_ENABLED));

        trg_client_set_session(client, newSession);

        if (reloadAliases)
            trg_main_window_reload_dir_aliases(win);

        if (refreshSpeed)
            trg_status_bar_update_speed(
                priv->statusBar, trg_torrent_model_get_stats(priv->torrentModel), priv->client);
    }

    if (!isConnected) {
        trg_main_window_conn_changed(win, TRUE);
        trg_trackers_tree_view_new_connection(priv->trackersTreeView, client);
        dispatch_rpc_async(client, torrent_get(TORRENT_GET_TAG_MODE_FULL), on_torrent_get_first,
                           win);
    }

    trg_response_free(response);

    return FALSE;
}

/*
 * The callback for a torrent-get response.
 */

static gboolean on_torrent_get(gpointer data, int mode)
{
    trg_response *response = (trg_response *)data;
    TrgMainWindow *win = TRG_MAIN_WINDOW(response->cb_data);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgClient *client = priv->client;
    TrgPrefs *prefs = trg_client_get_prefs(client);
    trg_torrent_model_update_stats *stats;
    guint interval;
    gint old_sort_id;
    GtkSortType old_order;

    /* Disconnected between request and response callback */
    if (!trg_client_is_connected(client)) {
        trg_response_free(response);
        return FALSE;
    }

    interval = trg_prefs_get_int(prefs, TRG_PREFS_KEY_UPDATE_INTERVAL, TRG_PREFS_CONNECTION);
    if (interval < 1)
        interval = TRG_INTERVAL_DEFAULT;

    if (response->status != SOUP_STATUS_OK) {
        gint64 max_retries = trg_prefs_get_int(prefs, TRG_PREFS_KEY_RETRIES, TRG_PREFS_CONNECTION);

        if (trg_client_inc_failcount(client) >= max_retries) {
            trg_main_window_conn_changed(win, FALSE);
            trg_dialog_error_handler(win, response);
        } else {
            gchar *msg = make_error_message(response->obj, response->status, response->err_msg);
            gchar *statusBarMsg
                = g_strdup_printf(_("Request %d/%d failed: %s"), trg_client_get_failcount(client),
                                  (gint)max_retries, msg);
            trg_status_bar_push_connection_msg(priv->statusBar, statusBarMsg);
            g_free(msg);
            g_free(statusBarMsg);
            priv->timerId = g_timeout_add_seconds(interval, trg_update_torrents_timerfunc, win);
        }

        trg_response_free(response);

        return FALSE;
    }

    trg_client_reset_failcount(client);
    trg_client_inc_serial(client);

    gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(priv->sortedTorrentModel), &old_sort_id,
                                         &old_order);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(priv->sortedTorrentModel),
                                         GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                         GTK_SORT_ASCENDING);

    stats = trg_torrent_model_update(priv->torrentModel, client, response->obj, mode);

    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(priv->sortedTorrentModel), old_sort_id,
                                         old_order);

    update_selected_torrent_notebook(win, mode, priv->selectedTorrentId);
    trg_status_bar_update(priv->statusBar, stats, client);

    if (mode != TORRENT_GET_MODE_INTERACTION)
        priv->timerId = g_timeout_add_seconds(interval, trg_update_torrents_timerfunc, win);

    trg_response_free(response);
    return FALSE;
}

static gboolean on_torrent_get_active(gpointer data)
{
    return on_torrent_get(data, TORRENT_GET_MODE_ACTIVE);
}

static gboolean on_torrent_get_first(gpointer data)
{
    trg_response *response = (trg_response *)data;
    TrgMainWindow *win = TRG_MAIN_WINDOW(response->cb_data);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    gboolean result = on_torrent_get(data, TORRENT_GET_MODE_FIRST);

    if (priv->args) {
        trg_add_from_filename(win, priv->args);
        priv->args = NULL;
    }

    return result;
}

static gboolean on_torrent_get_interactive(gpointer data)
{
    return on_torrent_get(data, TORRENT_GET_MODE_INTERACTION);
}

static gboolean on_torrent_get_update(gpointer data)
{
    return on_torrent_get(data, TORRENT_GET_MODE_UPDATE);
}

static gboolean trg_session_update_timerfunc(gpointer data)
{
    TrgMainWindow *win = TRG_MAIN_WINDOW(data);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    trg_client_update_session(priv->client, on_session_get_timer, win);

    return FALSE;
}

static gboolean trg_update_torrents_timerfunc(gpointer data)
{
    /* Check if the TrgMainWindow* has already been destroyed
     * and, in that case, stop polling the server. */
    if (!TRG_IS_MAIN_WINDOW(data))
        return FALSE;

    TrgMainWindow *win = TRG_MAIN_WINDOW(data);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgClient *tc = priv->client;
    TrgPrefs *prefs = trg_client_get_prefs(tc);

    if (trg_client_is_connected(tc)) {
        gboolean activeOnly
            = trg_prefs_get_bool(prefs, TRG_PREFS_KEY_UPDATE_ACTIVE_ONLY, TRG_PREFS_CONNECTION)
            && (!trg_prefs_get_bool(prefs, TRG_PREFS_ACTIVEONLY_FULLSYNC_ENABLED,
                                    TRG_PREFS_CONNECTION)
                || (trg_client_get_serial(tc)
                        % trg_prefs_get_int(prefs, TRG_PREFS_ACTIVEONLY_FULLSYNC_EVERY,
                                            TRG_PREFS_CONNECTION)
                    != 0));
        dispatch_rpc_async(
            tc, torrent_get(activeOnly ? TORRENT_GET_TAG_MODE_UPDATE : TORRENT_GET_TAG_MODE_FULL),
            activeOnly ? on_torrent_get_active : on_torrent_get_update, data);
    }

    return FALSE;
}

static void open_about_cb(GtkWidget *w G_GNUC_UNUSED, GtkWindow *parent)
{
    GtkWidget *aboutDialog = trg_about_window_new(parent);

    trg_dialog_run(GTK_DIALOG(aboutDialog));
    trg_widget_destroy(aboutDialog);
}

static gboolean trg_torrent_tree_view_visible_func(GtkTreeModel *model, GtkTreeIter *iter,
                                                   gpointer data)
{
    gboolean visible;
    visible = TRUE;
    return visible;
}

void trg_main_window_reload_dir_aliases(TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    trg_torrent_model_reload_dir_aliases(priv->client, GTK_TREE_MODEL(priv->torrentModel));
}

static TrgTorrentTreeView *trg_main_window_torrent_tree_view_new(TrgMainWindow *win,
                                                                 GtkTreeModel *model)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgTorrentTreeView *torrentTreeView = trg_torrent_tree_view_new(priv->client, model);

    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(torrentTreeView));

    g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(torrent_selection_changed), win);

    return torrentTreeView;
}

static gboolean trg_dialog_error_handler(TrgMainWindow *win, trg_response *response)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    if (response->status != SOUP_STATUS_OK) {
        GtkWidget *dialog;
        const gchar *msg;

        msg = make_error_message(response->obj, response->status, response->err_msg);
        trg_status_bar_clear_indicators(priv->statusBar);
        trg_status_bar_push_connection_msg(priv->statusBar, msg);
        dialog = gtk_message_dialog_new(GTK_WINDOW(win), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK, "%s", msg);
        gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
        trg_dialog_run(GTK_DIALOG(dialog));
        trg_widget_destroy(dialog);
        g_free((gpointer)msg);
        return TRUE;
    } else {
        return FALSE;
    }
}

static gboolean torrent_selection_changed(GtkTreeSelection *selection, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    GList *selectionList;
    GList *firstNode;
    gint64 id;

    if (trg_torrent_model_is_remove_in_progress(priv->torrentModel)) {
        trg_main_window_torrent_scrub(win);
        return TRUE;
    }

    selectionList = gtk_tree_selection_get_selected_rows(selection, NULL);
    firstNode = g_list_first(selectionList);
    id = -1;

    if (firstNode) {
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(priv->filteredTorrentModel, &iter,
                                    (GtkTreePath *)firstNode->data)) {
            gtk_tree_model_get(priv->filteredTorrentModel, &iter, TORRENT_COLUMN_ID, &id, -1);
        }
    }

    g_list_free_full(selectionList, (GDestroyNotify)gtk_tree_path_free);

    update_selected_torrent_notebook(win, TORRENT_GET_MODE_FIRST, id);

    return TRUE;
}

gboolean on_delete_complete(gpointer data)
{
    trg_response *response = (trg_response *)data;
    TrgMainWindow *win = TRG_MAIN_WINDOW(response->cb_data);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgClient *tc = priv->client;

    if (trg_client_is_connected(tc) && response->status == SOUP_STATUS_OK)
        trg_client_update_session(priv->client, on_session_get, response->cb_data);

    return on_generic_interactive_action_response(data);
}

void on_generic_interactive_action(TrgMainWindow *win, trg_response *response)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgClient *tc = priv->client;

    if (trg_client_is_connected(tc)) {
        trg_dialog_error_handler(win, response);

        if (response->status == SOUP_STATUS_OK) {
            gint64 id;
            if (json_object_has_member(response->obj, PARAM_TAG))
                id = json_object_get_int_member(response->obj, PARAM_TAG);
            else
                id = TORRENT_GET_TAG_MODE_FULL;

            dispatch_rpc_async(tc, torrent_get(id), on_torrent_get_interactive, win);
        }
    }

    trg_response_free(response);
}

gboolean on_generic_interactive_action_response(gpointer data)
{
    trg_response *response = (trg_response *)data;
    TrgMainWindow *win = TRG_MAIN_WINDOW(response->cb_data);

    on_generic_interactive_action(win, response);

    return FALSE;
}

static void trg_main_window_torrent_scrub(TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    gtk_tree_store_clear(GTK_TREE_STORE(priv->filesModel));
    gtk_list_store_clear(GTK_LIST_STORE(priv->trackersModel));
    gtk_list_store_clear(GTK_LIST_STORE(priv->peersModel));
    trg_general_panel_clear(priv->genDetails);
    trg_trackers_model_set_no_selection(TRG_TRACKERS_MODEL(priv->trackersModel));

    trg_toolbar_torrent_actions_sensitive(priv->toolBar, FALSE);
    trg_menu_bar_torrent_actions_sensitive(priv->menuBar, FALSE);
}

static void trg_main_window_conn_changed(TrgMainWindow *win, gboolean connected)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgClient *tc = priv->client;

    trg_toolbar_connected_change(priv->toolBar, connected);
    trg_menu_bar_connected_change(priv->menuBar, connected);

    gtk_widget_set_sensitive(GTK_WIDGET(priv->torrentTreeView), connected);
    gtk_widget_set_sensitive(GTK_WIDGET(priv->peersTreeView), connected);
    gtk_widget_set_sensitive(GTK_WIDGET(priv->filesTreeView), connected);
    gtk_widget_set_sensitive(GTK_WIDGET(priv->trackersTreeView), connected);
    gtk_widget_set_sensitive(GTK_WIDGET(priv->genDetails), connected);

    if (connected) {
        TrgPrefs *prefs = trg_client_get_prefs(priv->client);
        priv->sessionTimerId = g_timeout_add_seconds(
            trg_prefs_get_int(prefs, TRG_PREFS_KEY_SESSION_UPDATE_INTERVAL, TRG_PREFS_CONNECTION),
            trg_session_update_timerfunc, win);
    } else {
        trg_main_window_torrent_scrub(win);
        trg_torrent_model_remove_all(priv->torrentModel);
        g_clear_handle_id(&priv->timerId, g_source_remove);
        g_clear_handle_id(&priv->sessionTimerId, g_source_remove);
    }

    trg_client_status_change(tc, connected);
}

static void trg_main_window_get_property(GObject *object, guint property_id, GValue *value,
                                         GParamSpec *pspec)
{
    TrgMainWindow *win = TRG_MAIN_WINDOW(object);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    switch (property_id) {
    case PROP_CLIENT:
        g_value_set_pointer(value, priv->client);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void trg_main_window_set_property(GObject *object, guint property_id, const GValue *value,
                                         GParamSpec *pspec)
{
    TrgMainWindow *win = TRG_MAIN_WINDOW(object);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    switch (property_id) {
    case PROP_CLIENT:
        priv->client = g_value_get_pointer(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void quit_cb(GtkWidget *w G_GNUC_UNUSED, gpointer data)
{
    trg_widget_destroy(GTK_WIDGET(data));
}

static TrgMenuBar *trg_main_window_menu_bar_new(TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    GObject *b_disconnect, *b_add, *b_resume, *b_pause, *b_verify, *b_remove, *b_delete, *b_props,
        *b_local_prefs, *b_remote_prefs, *b_about, *b_view_notebook, *b_view_stats, *b_add_url,
        *b_quit, *b_move, *b_reannounce, *b_pause_all, *b_resume_all, *b_up_queue, *b_down_queue,
        *b_top_queue, *b_bottom_queue, *b_start_now, *b_copy_magnetlink;

    TrgMenuBar *menuBar;

    menuBar = trg_menu_bar_new(win, trg_client_get_prefs(priv->client), priv->torrentTreeView);

    g_object_get(menuBar, "disconnect-button", &b_disconnect, "add-button", &b_add,
                 "add-url-button", &b_add_url, "resume-button", &b_resume, "resume-all-button",
                 &b_resume_all, "pause-button", &b_pause, "pause-all-button", &b_pause_all,
                 "delete-button", &b_delete, "remove-button", &b_remove, "move-button", &b_move,
                 "verify-button", &b_verify, "reannounce-button", &b_reannounce, "props-button",
                 &b_props, "remote-prefs-button", &b_remote_prefs, "local-prefs-button",
                 &b_local_prefs, "view-notebook-button", &b_view_notebook, "view-stats-button",
                 &b_view_stats, "about-button", &b_about, "quit-button", &b_quit, "up-queue",
                 &b_up_queue, "down-queue", &b_down_queue, "top-queue", &b_top_queue,
                 "bottom-queue", &b_bottom_queue, "start-now", &b_start_now, "copymagnet-button",
                 &b_copy_magnetlink, NULL);

    g_signal_connect(b_disconnect, "trg-activate", G_CALLBACK(disconnect_cb), win);
    g_signal_connect(b_add, "trg-activate", G_CALLBACK(add_cb), win);
    g_signal_connect(b_add_url, "trg-activate", G_CALLBACK(add_url_cb), win);
    g_signal_connect(b_resume, "trg-activate", G_CALLBACK(resume_cb), win);
    g_signal_connect(b_resume_all, "trg-activate", G_CALLBACK(resume_all_cb), win);
    g_signal_connect(b_pause, "trg-activate", G_CALLBACK(pause_cb), win);
    g_signal_connect(b_pause_all, "trg-activate", G_CALLBACK(pause_all_cb), win);
    g_signal_connect(b_verify, "trg-activate", G_CALLBACK(verify_cb), win);
    g_signal_connect(b_reannounce, "trg-activate", G_CALLBACK(reannounce_cb), win);
    g_signal_connect(b_delete, "trg-activate", G_CALLBACK(delete_cb), win);
    g_signal_connect(b_remove, "trg-activate", G_CALLBACK(remove_cb), win);
    g_signal_connect(b_up_queue, "trg-activate", G_CALLBACK(up_queue_cb), win);
    g_signal_connect(b_down_queue, "trg-activate", G_CALLBACK(down_queue_cb), win);
    g_signal_connect(b_top_queue, "trg-activate", G_CALLBACK(top_queue_cb), win);
    g_signal_connect(b_bottom_queue, "trg-activate", G_CALLBACK(bottom_queue_cb), win);
    g_signal_connect(b_start_now, "trg-activate", G_CALLBACK(start_now_cb), win);
    g_signal_connect(b_move, "trg-activate", G_CALLBACK(move_cb), win);
    g_signal_connect(b_about, "trg-activate", G_CALLBACK(open_about_cb), win);
    g_signal_connect(b_local_prefs, "trg-activate", G_CALLBACK(open_local_prefs_cb), win);
    g_signal_connect(b_remote_prefs, "trg-activate", G_CALLBACK(open_remote_prefs_cb), win);
    g_signal_connect(b_view_notebook, "toggled", G_CALLBACK(view_notebook_toggled_cb), win);
    g_signal_connect(b_view_stats, "trg-activate", G_CALLBACK(view_stats_toggled_cb), win);
    g_signal_connect(b_props, "trg-activate", G_CALLBACK(open_props_cb), win);
    g_signal_connect(b_copy_magnetlink, "trg-activate", G_CALLBACK(copy_magnetlink_cb), win);
    g_signal_connect(b_quit, "trg-activate", G_CALLBACK(quit_cb), win);

    return menuBar;
}

static GtkWidget *trg_menu_item_with_cb(TrgMenu *menu, const gchar *text, const char *icon_name,
                                       gboolean sensitive, GCallback cb, gpointer cbdata)
{
    GtkWidget *item = trg_menu_append_item(menu, text, icon_name, sensitive);

    g_signal_connect(item, "trg-activate", cb, cbdata);
    return item;
}

static void json_array_unref_notify(gpointer data)
{
    json_array_unref((JsonArray *)data);
}

static void set_limit_cb(GtkWidget *w, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    gint speed = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "limit"));
    gchar *speedKey = g_object_get_data(G_OBJECT(w), "speedKey");
    gchar *enabledKey = g_object_get_data(G_OBJECT(w), "enabledKey");
    gpointer limitIds = g_object_get_data(G_OBJECT(w), "limit-ids");

    JsonNode *req = NULL;
    JsonObject *args;

    if (limitIds)
        req = torrent_set((JsonArray *)limitIds);
    else
        req = session_set();

    args = node_get_arguments(req);

    if (speed >= 0)
        json_object_set_int_member(args, speedKey, speed);

    json_object_set_boolean_member(args, enabledKey, speed >= 0);

    if (limitIds)
        dispatch_rpc_async(priv->client, req, on_generic_interactive_action_response, win);
    else
        dispatch_rpc_async(priv->client, req, on_session_set, win);
}

static void set_priority_cb(GtkWidget *w, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    gint priority = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "priority"));
    gpointer limitIds = g_object_get_data(G_OBJECT(w), "pri-ids");

    JsonNode *req = NULL;
    JsonObject *args;

    req = torrent_set((JsonArray *)limitIds);

    args = node_get_arguments(req);

    json_object_set_int_member(args, FIELD_BANDWIDTH_PRIORITY, priority);

    dispatch_rpc_async(priv->client, req, on_generic_interactive_action_response, win);
}

static void limit_item_new(TrgMainWindow *win, TrgMenu *menu, gint64 currentLimit, gfloat limit,
                           const gchar *enabledKey, const gchar *speedKey, JsonArray *ids)
{
    char speed[32];
    GtkWidget *item;
    gboolean active = limit < 0 ? FALSE : (currentLimit == (gint64)limit);

    trg_strlspeed(speed, limit);

    item = trg_menu_append_check(menu, speed, active);

    g_object_set_data(G_OBJECT(item), "limit", GINT_TO_POINTER((gint)limit));
    g_object_set_data_full(G_OBJECT(item), "speedKey", g_strdup(speedKey), g_free);
    g_object_set_data_full(G_OBJECT(item), "enabledKey", g_strdup(enabledKey), g_free);
    g_object_set_data_full(G_OBJECT(item), "limit-ids", json_array_ref(ids), json_array_unref_notify);
    g_signal_connect(item, "toggled", G_CALLBACK(set_limit_cb), win);
}

static void priority_menu_item_new(TrgMainWindow *win, TrgMenu *menu, const gchar *label,
                                   gint value, gint current_value, JsonArray *ids)
{
    GtkWidget *item = trg_menu_append_check(menu, label, value == current_value);

    g_object_set_data(G_OBJECT(item), "priority", GINT_TO_POINTER(value));
    g_object_set_data_full(G_OBJECT(item), "pri-ids", json_array_ref(ids), json_array_unref_notify);
    g_signal_connect(item, "toggled", G_CALLBACK(set_priority_cb), win);
}

static TrgMenu *priority_menu_new(TrgMainWindow *win, JsonArray *ids)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgClient *client = priv->client;
    JsonObject *t = NULL;
    gint selected_pri = TR_PRI_UNSET;
    TrgMenu *menu = trg_menu_new();

    if (get_torrent_data(trg_client_get_torrent_table(client), priv->selectedTorrentId, &t, NULL))
        selected_pri = torrent_get_bandwidth_priority(t);

    priority_menu_item_new(win, menu, _("High"), TR_PRI_HIGH, selected_pri, ids);
    priority_menu_item_new(win, menu, _("Normal"), TR_PRI_NORMAL, selected_pri, ids);
    priority_menu_item_new(win, menu, _("Low"), TR_PRI_LOW, selected_pri, ids);

    return menu;
}

static TrgMenu *limit_menu_new(TrgMainWindow *win, gchar *enabledKey, gchar *speedKey, JsonArray *ids)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgClient *client = priv->client;
    JsonObject *current = NULL;
    GtkTreeIter iter;
    TrgMenu *menu = trg_menu_new();
    GtkWidget *item;
    gint64 limit;

    if (ids)
        get_torrent_data(trg_client_get_torrent_table(client), priv->selectedTorrentId, &current,
                         &iter);
    else
        current = trg_client_get_session(client);

    limit = json_object_get_boolean_member(current, enabledKey)
        ? json_object_get_int_member(current, speedKey)
        : -1;

    item = trg_menu_append_check(menu, _("No Limit"), limit < 0);
    g_object_set_data(G_OBJECT(item), "limit", GINT_TO_POINTER(-1));
    g_object_set_data_full(G_OBJECT(item), "speedKey", g_strdup(speedKey), g_free);
    g_object_set_data_full(G_OBJECT(item), "enabledKey", g_strdup(enabledKey), g_free);
    g_object_set_data_full(G_OBJECT(item), "limit-ids", json_array_ref(ids), json_array_unref_notify);
    g_signal_connect(item, "toggled", G_CALLBACK(set_limit_cb), win);

    trg_menu_append_separator(menu);

    limit_item_new(win, menu, limit, 100, enabledKey, speedKey, ids);
    limit_item_new(win, menu, limit, 200, enabledKey, speedKey, ids);
    limit_item_new(win, menu, limit, 500, enabledKey, speedKey, ids);
    limit_item_new(win, menu, limit, 750, enabledKey, speedKey, ids);
    limit_item_new(win, menu, limit, 1024, enabledKey, speedKey, ids);
    limit_item_new(win, menu, limit, 1536, enabledKey, speedKey, ids);
    limit_item_new(win, menu, limit, 2048, enabledKey, speedKey, ids);
    limit_item_new(win, menu, limit, 3072, enabledKey, speedKey, ids);
    limit_item_new(win, menu, limit, 4096, enabledKey, speedKey, ids);
    limit_item_new(win, menu, limit, 5120, enabledKey, speedKey, ids);

    return menu;
}

static void exec_cmd_cb(GtkWidget *w, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    JsonObject *cmd_obj = (JsonObject *)g_object_get_data(G_OBJECT(w), "cmd-object");
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->torrentTreeView));
    GtkTreeModel *model;
    GList *selectedRows = gtk_tree_selection_get_selected_rows(selection, &model);
    GError *cmd_error = NULL;
    gchar *cmd_line = NULL;
    gchar **argv = NULL;

    cmd_line = build_remote_exec_cmd(
        priv->client, model, selectedRows,
        json_object_get_string_member(cmd_obj, TRG_PREFS_KEY_EXEC_COMMANDS_SUBKEY_CMD));

    g_debug("Exec: %s", cmd_line);

    if (!cmd_line)
        return;

    g_shell_parse_argv(cmd_line, NULL, &argv, NULL);
    g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &cmd_error);

    g_list_free_full(selectedRows, (GDestroyNotify)gtk_tree_path_free);

    if (argv)
        g_strfreev(argv);

    g_free(cmd_line);

    if (cmd_error) {
        GtkWidget *dialog
            = gtk_message_dialog_new(GTK_WINDOW(win), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_OK, "%s", cmd_error->message);
        gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
        trg_dialog_run(GTK_DIALOG(dialog));
        trg_widget_destroy(dialog);
        g_error_free(cmd_error);
    }
}

static void trg_torrent_tv_view_menu(GtkWidget *treeview, double x, double y, gboolean at_pointer,
                                     TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgPrefs *prefs = trg_client_get_prefs(priv->client);
    TrgMenu *menu = trg_menu_new();
    gint n_cmds;
    JsonArray *ids;
    JsonArray *cmds;

    ids = build_json_id_array(TRG_TORRENT_TREE_VIEW(treeview));

    trg_menu_item_with_cb(menu, _("Properties"), "document-properties", TRUE,
                          G_CALLBACK(open_props_cb), win);
    trg_menu_item_with_cb(menu, _("Copy Magnet Link"), "edit-copy", TRUE,
                          G_CALLBACK(copy_magnetlink_cb), win);
    trg_menu_item_with_cb(menu, _("Resume"), "media-playback-start", TRUE, G_CALLBACK(resume_cb),
                          win);
    trg_menu_item_with_cb(menu, _("Pause"), "media-playback-pause", TRUE, G_CALLBACK(pause_cb),
                          win);
    trg_menu_item_with_cb(menu, _("Verify"), "view-refresh", TRUE, G_CALLBACK(verify_cb), win);
    trg_menu_item_with_cb(menu, _("Re-announce"), "network-server", TRUE,
                          G_CALLBACK(reannounce_cb), win);
    trg_menu_item_with_cb(menu, _("Move"), "drive-harddisk", TRUE, G_CALLBACK(move_cb), win);
    trg_menu_item_with_cb(menu, _("Remove"), "list-remove", TRUE, G_CALLBACK(remove_cb), win);
    trg_menu_item_with_cb(menu, _("Remove and delete data"), "edit-delete", TRUE,
                          G_CALLBACK(delete_cb), win);

    cmds = trg_prefs_get_array(prefs, TRG_PREFS_KEY_EXEC_COMMANDS, TRG_PREFS_CONNECTION);
    n_cmds = json_array_get_length(cmds);

    if (n_cmds > 0) {
        GList *cmds_list = json_array_get_elements(cmds);
        TrgMenu *cmds_menu;
        GList *cmds_li;

        trg_menu_append_separator(menu);

        if (n_cmds < 3) {
            cmds_menu = menu;
        } else {
            cmds_menu = trg_menu_new();
            trg_menu_append_submenu(menu, _("Execute"), "system-run", cmds_menu);
        }

        for (cmds_li = cmds_list; cmds_li; cmds_li = g_list_next(cmds_li)) {
            JsonObject *cmd_obj = json_node_get_object((JsonNode *)cmds_li->data);
            const gchar *cmd_label = json_object_get_string_member(cmd_obj, "label");
            GtkWidget *item = trg_menu_item_with_cb(cmds_menu, cmd_label, "system-run", TRUE,
                                                    G_CALLBACK(exec_cmd_cb), win);
            g_object_set_data(G_OBJECT(item), "cmd-object", cmd_obj);
        }

        g_list_free(cmds_list);
    }

    trg_menu_append_separator(menu);

    if (priv->queuesEnabled) {
        trg_menu_item_with_cb(menu, _("Start Now"), "media-playback-start", TRUE,
                              G_CALLBACK(start_now_cb), win);
        trg_menu_item_with_cb(menu, _("Move Up Queue"), "go-up", TRUE, G_CALLBACK(up_queue_cb), win);
        trg_menu_item_with_cb(menu, _("Move Down Queue"), "go-down", TRUE, G_CALLBACK(down_queue_cb),
                              win);
        trg_menu_item_with_cb(menu, _("Bottom Of Queue"), "go-bottom", TRUE,
                              G_CALLBACK(bottom_queue_cb), win);
        trg_menu_item_with_cb(menu, _("Top Of Queue"), "go-top", TRUE, G_CALLBACK(top_queue_cb), win);
        trg_menu_append_separator(menu);
    }

    trg_menu_append_submenu(menu, _("Down Limit"), "network-workgroup",
                            limit_menu_new(win, FIELD_DOWNLOAD_LIMITED, FIELD_DOWNLOAD_LIMIT, ids));
    trg_menu_append_submenu(menu, _("Up Limit"), "network-workgroup",
                            limit_menu_new(win, FIELD_UPLOAD_LIMITED, FIELD_UPLOAD_LIMIT, ids));
    trg_menu_append_submenu(menu, _("Priority"), "network-workgroup", priority_menu_new(win, ids));

    json_array_unref(ids);

    if (at_pointer)
        trg_menu_popup_at_pointer(menu, treeview, x, y);
    else
        trg_menu_popup_at_widget(menu, treeview);
}

static void torrent_tv_button_pressed_cb(GtkWidget *treeview, int button, double x, double y,
                                         GdkModifierType state, gpointer userdata)
{
    GtkTreeSelection *selection;
    GtkTreePath *path;

    (void)state;

    if (button == 3) {
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview), (int)x, (int)y, &path, NULL,
                                          NULL, NULL)) {
            if (!gtk_tree_selection_path_is_selected(selection, path)) {
                gtk_tree_selection_unselect_all(selection);
                gtk_tree_selection_select_path(selection, path);
            }

            gtk_tree_path_free(path);

            trg_torrent_tv_view_menu(treeview, x, y, TRUE, userdata);
        }
    }
}

static void trg_client_session_updated_cb(TrgClient *tc, JsonObject *session, TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    gboolean queuesEnabled;

    trg_status_bar_session_update(priv->statusBar, session);

    if (json_object_has_member(session, SGET_DOWNLOAD_QUEUE_ENABLED)) {
        queuesEnabled = json_object_get_boolean_member(session, SGET_DOWNLOAD_QUEUE_ENABLED)
            || json_object_get_boolean_member(session, SGET_SEED_QUEUE_ENABLED);
    } else {
        queuesEnabled = FALSE;
    }

    if (priv->queuesEnabled != queuesEnabled) {
        trg_menu_bar_set_supports_queues(priv->menuBar, queuesEnabled);
    }

    priv->queuesEnabled = queuesEnabled;
}

/* Drag & Drop support */
static gboolean on_dropped_file(GtkDropTarget *target, const GValue *value, double x, double y,
                                gpointer user_data)
{
    TrgMainWindow *win = user_data;
    GSList *files = g_value_get_boxed(value);
    guint num_files = g_slist_length(files);
    gchar **file_list;

    (void)target;
    (void)x;
    (void)y;

    if (num_files == 0)
        return FALSE;

    file_list = g_new0(gchar *, num_files + 1);

    for (guint i = 0; i < num_files; i++) {
        GFile *file = g_slist_nth_data(files, i);
        file_list[i] = g_file_get_path(file);
    }

    trg_add_from_filename(win, file_list);

    return TRUE;
}

static void trg_main_window_add_drop_target(TrgMainWindow *win)
{
    GtkDropTarget *target = gtk_drop_target_new(GDK_TYPE_FILE_LIST, GDK_ACTION_COPY);

    g_signal_connect(target, "drop", G_CALLBACK(on_dropped_file), win);
    gtk_widget_add_controller(GTK_WIDGET(win), GTK_EVENT_CONTROLLER(target));
}

static gboolean window_key_press_handler(GtkWidget *widget, guint keyval, guint keycode,
                                         GdkModifierType state, gpointer user_data)
{
    TrgMainWindow *win = TRG_MAIN_WINDOW(widget);
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);

    (void)keycode;
    (void)user_data;

    if ((state & GDK_CONTROL_MASK) && keyval == GDK_KEY_k) {
        gtk_widget_grab_focus(priv->filterEntry);
        return TRUE;
    }

    return FALSE;
}

static GObject *trg_main_window_constructor(GType type, guint n_construct_properties,
                                            GObjectConstructParam *construct_params)
{
    TrgMainWindow *self
        = TRG_MAIN_WINDOW(G_OBJECT_CLASS(trg_main_window_parent_class)
                              ->constructor(type, n_construct_properties, construct_params));
    TrgMainWindowPrivate *priv
        = G_TYPE_INSTANCE_GET_PRIVATE(self, TRG_TYPE_MAIN_WINDOW, TrgMainWindowPrivate);
    GtkWidget *outerVbox;
    GtkWidget *toolbarHbox;
    gint width, height, pos;
    TrgPrefs *prefs;

    priv->queuesEnabled = TRUE;

    prefs = trg_client_get_prefs(priv->client);

    gtk_window_set_default_icon_name(PACKAGE_NAME);

    gtk_window_set_title(GTK_WINDOW(self), _("Transmission Remote"));
    gtk_window_set_default_size(GTK_WINDOW(self), 1000, 600);

    g_signal_connect(G_OBJECT(self), "close-request", G_CALLBACK(delete_event), NULL);
    g_signal_connect(G_OBJECT(self), "destroy", G_CALLBACK(destroy_window), NULL);
    trg_widget_add_key_handler(GTK_WIDGET(self), G_CALLBACK(window_key_press_handler), NULL);

    priv->torrentModel = trg_torrent_model_new();
    trg_client_set_torrent_table(priv->client, get_torrent_table(priv->torrentModel));

    g_signal_connect(priv->torrentModel, "torrent-completed", G_CALLBACK(on_torrent_completed),
                     self);
    g_signal_connect(priv->torrentModel, "torrent-added", G_CALLBACK(on_torrent_added), self);

    priv->sortedTorrentModel
        = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(priv->torrentModel));

    priv->filteredTorrentModel
        = trg_sortable_filtered_model_new(GTK_TREE_SORTABLE(priv->sortedTorrentModel), NULL);
    gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(priv->filteredTorrentModel),
                                           trg_torrent_tree_view_visible_func, self, NULL);

    priv->torrentTreeView = trg_main_window_torrent_tree_view_new(self, priv->filteredTorrentModel);
    trg_widget_add_click_handler(GTK_WIDGET(priv->torrentTreeView),
                                 G_CALLBACK(torrent_tv_button_pressed_cb), self);
    g_signal_connect(priv->torrentTreeView, "row-activated", G_CALLBACK(torrent_tv_onRowActivated),
                     self);

    outerVbox = trg_vbox_new(FALSE, 0);

    trg_container_add(self, outerVbox);

    priv->menuBar = trg_main_window_menu_bar_new(self);
    trg_box_pack(outerVbox, GTK_WIDGET(priv->menuBar), FALSE, FALSE, 0);

    toolbarHbox = trg_hbox_new(FALSE, 0);
    priv->toolBar = trg_main_window_toolbar_new(self);
    trg_box_pack(toolbarHbox, GTK_WIDGET(priv->toolBar), TRUE, TRUE, 0);

    trg_box_pack(outerVbox, GTK_WIDGET(toolbarHbox), FALSE, FALSE, 0);

    priv->hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    priv->vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);

    trg_box_pack(outerVbox, priv->vpaned, TRUE, TRUE, 0);
    gtk_paned_set_start_child(GTK_PANED(priv->vpaned), priv->hpaned);
    gtk_paned_set_resize_start_child(GTK_PANED(priv->vpaned), TRUE);
    gtk_paned_set_shrink_start_child(GTK_PANED(priv->vpaned), FALSE);

    gtk_paned_set_end_child(GTK_PANED(priv->hpaned),
                            my_scrolledwin_new(GTK_WIDGET(priv->torrentTreeView)));
    gtk_paned_set_resize_end_child(GTK_PANED(priv->hpaned), TRUE);

    priv->notebook = trg_main_window_notebook_new(self);
    gtk_paned_set_end_child(GTK_PANED(priv->vpaned), priv->notebook);
    gtk_paned_set_resize_end_child(GTK_PANED(priv->vpaned), FALSE);
    priv->statusBar = trg_status_bar_new(self, priv->client);
    g_signal_connect(priv->client, "session-updated", G_CALLBACK(trg_client_session_updated_cb),
                     self);

    trg_box_pack(outerVbox, GTK_WIDGET(priv->statusBar), FALSE, FALSE, 2);

    width = trg_prefs_get_int(prefs, TRG_PREFS_KEY_WINDOW_WIDTH, TRG_PREFS_GLOBAL);
    height = trg_prefs_get_int(prefs, TRG_PREFS_KEY_WINDOW_HEIGHT, TRG_PREFS_GLOBAL);

    pos = trg_prefs_get_int(prefs, TRG_PREFS_KEY_NOTEBOOK_PANED_POS, TRG_PREFS_GLOBAL);

    if (width > 0 && height > 0)
        gtk_window_set_default_size(GTK_WINDOW(self), width, height);
    else if (pos < 1)
        gtk_paned_set_position(GTK_PANED(priv->vpaned), 300);

    if (pos > 0)
        gtk_paned_set_position(GTK_PANED(priv->vpaned), pos);

    trg_widget_set_visible(
        priv->notebook, trg_prefs_get_bool(prefs, TRG_PREFS_KEY_SHOW_NOTEBOOK, TRG_PREFS_GLOBAL));

    pos = trg_prefs_get_int(prefs, TRG_PREFS_KEY_STATES_PANED_POS, TRG_PREFS_GLOBAL);
    if (pos > 0)
        gtk_paned_set_position(GTK_PANED(priv->hpaned), pos);

    /* Drag and Drop */
    trg_main_window_add_drop_target(self);

    return G_OBJECT(self);
}

static void trg_main_window_class_init(TrgMainWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->constructor = trg_main_window_constructor;
    object_class->get_property = trg_main_window_get_property;
    object_class->set_property = trg_main_window_set_property;

    g_object_class_install_property(
        object_class, PROP_CLIENT,
        g_param_spec_pointer("trg-client", "TClient", "Client",
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
}

void trg_main_window_set_start_args(TrgMainWindow *win, gchar **args)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    priv->args = args;
}

void auto_connect_if_required(TrgMainWindow *win)
{
    TrgMainWindowPrivate *priv = trg_main_window_get_instance_private(win);
    TrgPrefs *prefs = trg_client_get_prefs(priv->client);
    gchar *host = trg_prefs_get_string(prefs, TRG_PREFS_KEY_HOSTNAME, TRG_PREFS_PROFILE);

    if (host) {
        gint len = strlen(host);
        g_free(host);
        if (len > 0 && trg_prefs_get_bool(prefs, TRG_PREFS_KEY_AUTO_CONNECT, TRG_PREFS_PROFILE)) {
            connect_cb(NULL, win);
        }
    }
}

TrgMainWindow *trg_main_window_new(TrgClient *tc)
{
    return g_object_new(TRG_TYPE_MAIN_WINDOW, "trg-client", tc, NULL);
}
