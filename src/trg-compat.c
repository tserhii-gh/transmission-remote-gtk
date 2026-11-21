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

#include <gtk/gtk.h>

#include "trg-compat.h"

void trg_container_add(gpointer container, GtkWidget *child)
{
    g_return_if_fail(GTK_IS_WIDGET(container));
    g_return_if_fail(GTK_IS_WIDGET(child));

    if (GTK_IS_BOX(container)) {
        gtk_box_append(GTK_BOX(container), child);
    } else if (GTK_IS_FRAME(container)) {
        gtk_frame_set_child(GTK_FRAME(container), child);
    } else if (GTK_IS_SCROLLED_WINDOW(container)) {
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(container), child);
    } else if (GTK_IS_WINDOW(container)) {
        gtk_window_set_child(GTK_WINDOW(container), child);
    } else if (GTK_IS_PANED(container)) {
        if (gtk_paned_get_start_child(GTK_PANED(container)) == NULL)
            gtk_paned_set_start_child(GTK_PANED(container), child);
        else
            gtk_paned_set_end_child(GTK_PANED(container), child);
    } else if (GTK_IS_LIST_BOX(container)) {
        gtk_list_box_append(GTK_LIST_BOX(container), child);
    } else if (GTK_IS_OVERLAY(container)) {
        gtk_overlay_add_overlay(GTK_OVERLAY(container), child);
    } else if (GTK_IS_NOTEBOOK(container)) {
        gtk_notebook_append_page(GTK_NOTEBOOK(container), child, NULL);
    } else if (GTK_IS_GRID(container)) {
        guint row = 0;
        gtk_grid_attach_next_to(GTK_GRID(container), child, NULL, GTK_POS_BOTTOM, 1, 1);
        (void)row;
    } else if (GTK_IS_HEADER_BAR(container)) {
        gtk_header_bar_pack_start(GTK_HEADER_BAR(container), child);
    } else {
        g_warning("trg_container_add: unsupported container type %s",
                  G_OBJECT_TYPE_NAME(container));
    }
}

void trg_container_set_border_width(gpointer widget, guint border_width)
{
    gtk_widget_set_margin_top(widget, border_width);
    gtk_widget_set_margin_bottom(widget, border_width);
    gtk_widget_set_margin_start(widget, border_width);
    gtk_widget_set_margin_end(widget, border_width);
}

static void trg_box_pack_internal(GtkWidget *box, GtkWidget *child, gboolean expand,
                                  guint padding)
{
    GtkOrientation orientation = gtk_orientable_get_orientation(GTK_ORIENTABLE(box));

    gtk_box_append(GTK_BOX(box), child);

    if (expand) {
        if (orientation == GTK_ORIENTATION_HORIZONTAL)
            gtk_widget_set_hexpand(child, TRUE);
        else
            gtk_widget_set_vexpand(child, TRUE);
    }

    if (padding > 0) {
        if (orientation == GTK_ORIENTATION_HORIZONTAL) {
            gtk_widget_set_margin_start(child, padding);
            gtk_widget_set_margin_end(child, padding);
        } else {
            gtk_widget_set_margin_top(child, padding);
            gtk_widget_set_margin_bottom(child, padding);
        }
    }
}

void trg_box_pack(gpointer box, GtkWidget *child, gboolean expand, gboolean fill, guint padding)
{
    (void)fill;
    trg_box_pack_internal(box, child, expand, padding);
}

void trg_box_pack_end(gpointer box, GtkWidget *child, gboolean expand, gboolean fill,
                      guint padding)
{
    (void)fill;
    trg_box_pack_internal(box, child, expand, padding);
}

typedef struct {
    GMainLoop *loop;
    gint response;
} TrgDialogRunData;

static void trg_dialog_run_response(GtkDialog *dialog, gint response, gpointer user_data)
{
    TrgDialogRunData *data = user_data;
    (void)dialog;
    data->response = response;
    g_main_loop_quit(data->loop);
}

static gboolean trg_dialog_run_close(GtkWindow *window, gpointer user_data)
{
    TrgDialogRunData *data = user_data;
    (void)window;
    if (data->response == GTK_RESPONSE_NONE)
        data->response = GTK_RESPONSE_DELETE_EVENT;
    g_main_loop_quit(data->loop);
    return FALSE;
}

gint trg_dialog_run(GtkDialog *dialog)
{
    TrgDialogRunData data = { g_main_loop_new(NULL, FALSE), GTK_RESPONSE_NONE };
    gulong response_id = g_signal_connect(dialog, "response",
                                          G_CALLBACK(trg_dialog_run_response), &data);
    gulong close_id = g_signal_connect(dialog, "close-request",
                                       G_CALLBACK(trg_dialog_run_close), &data);

    gtk_window_present(GTK_WINDOW(dialog));
    g_main_loop_run(data.loop);

    g_signal_handler_disconnect(dialog, response_id);
    g_signal_handler_disconnect(dialog, close_id);
    g_main_loop_unref(data.loop);

    return data.response;
}

void trg_widget_destroy(GtkWidget *widget)
{
    if (GTK_IS_WINDOW(widget))
        gtk_window_destroy(GTK_WINDOW(widget));
    else if (gtk_widget_get_parent(widget) != NULL)
        gtk_widget_unparent(widget);
    else
        g_object_unref(widget);
}

static void trg_widget_show_all_recursive(GtkWidget *widget)
{
    GtkWidget *child;

    /* Popovers are managed by their buttons and must stay hidden until
     * they are explicitly opened. */
    if (GTK_IS_POPOVER(widget))
        return;

    gtk_widget_set_visible(widget, TRUE);

    for (child = gtk_widget_get_first_child(widget); child != NULL;
         child = gtk_widget_get_next_sibling(child))
        trg_widget_show_all_recursive(child);
}

void trg_widget_show_all(GtkWidget *widget)
{
    trg_widget_show_all_recursive(widget);
}

GtkWidget *trg_bin_get_child(GtkWidget *widget)
{
    if (GTK_IS_WINDOW(widget))
        return gtk_window_get_child(GTK_WINDOW(widget));
    if (GTK_IS_DIALOG(widget))
        return gtk_dialog_get_content_area(GTK_DIALOG(widget));
    if (GTK_IS_FRAME(widget))
        return gtk_frame_get_child(GTK_FRAME(widget));
    if (GTK_IS_SCROLLED_WINDOW(widget))
        return gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(widget));
    if (GTK_IS_BUTTON(widget))
        return gtk_button_get_child(GTK_BUTTON(widget));
    if (GTK_IS_PANED(widget)) {
        GtkWidget *child = gtk_paned_get_start_child(GTK_PANED(widget));
        return child ? child : gtk_paned_get_end_child(GTK_PANED(widget));
    }
    if (GTK_IS_POPOVER(widget))
        return gtk_popover_get_child(GTK_POPOVER(widget));

    g_warning("trg_bin_get_child: unsupported type %s", G_OBJECT_TYPE_NAME(widget));
    return NULL;
}

GtkWidget *trg_widget_get_toplevel(GtkWidget *widget)
{
    GtkRoot *root = gtk_widget_get_root(widget);
    return root ? GTK_WIDGET(root) : NULL;
}

typedef struct {
    GtkWidget *widget;
    GCallback callback;
    gpointer data;
} TrgEventData;

static void trg_event_data_free(gpointer user_data)
{
    g_free(user_data);
}

static void trg_click_handler(GtkGestureClick *gesture, int n_press, double x, double y,
                              gpointer user_data)
{
    TrgEventData *d = user_data;
    GdkModifierType state
        = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));
    int button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));

    (void)n_press;
    ((void (*)(GtkWidget *, int, double, double, GdkModifierType, gpointer))d->callback)(
        d->widget, button, x, y, state, d->data);
}

void trg_widget_add_click_handler(GtkWidget *widget, GCallback cb, gpointer data)
{
    GtkGesture *gesture;

    if (widget == NULL)
        return;

    gesture = gtk_gesture_click_new();
    TrgEventData *d = g_new0(TrgEventData, 1);

    d->widget = widget;
    d->callback = cb;
    d->data = data;

    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 0);
    gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(gesture));
    g_object_set_data_full(G_OBJECT(gesture), "trg-event-data", d, trg_event_data_free);
    g_signal_connect(gesture, "pressed", G_CALLBACK(trg_click_handler), d);
}

static gboolean trg_key_handler(GtkEventControllerKey *controller, guint keyval, guint keycode,
                                GdkModifierType state, gpointer user_data)
{
    TrgEventData *d = user_data;

    return ((gboolean (*)(GtkWidget *, guint, guint, GdkModifierType, gpointer))d->callback)(
        d->widget, keyval, keycode, state, d->data);
}

void trg_widget_add_key_handler(GtkWidget *widget, GCallback cb, gpointer data)
{
    GtkEventController *controller;

    if (widget == NULL)
        return;

    controller = gtk_event_controller_key_new();
    TrgEventData *d = g_new0(TrgEventData, 1);

    d->widget = widget;
    d->callback = cb;
    d->data = data;

    gtk_widget_add_controller(widget, controller);
    g_object_set_data_full(G_OBJECT(controller), "trg-event-data", d, trg_event_data_free);
    g_signal_connect(controller, "key-pressed", G_CALLBACK(trg_key_handler), d);
}

GSList *trg_file_chooser_get_filenames(GtkFileChooser *chooser)
{
    GListModel *files = gtk_file_chooser_get_files(chooser);
    GSList *filenames = NULL;
    guint n, i;

    if (files == NULL)
        return NULL;

    n = g_list_model_get_n_items(files);

    for (i = 0; i < n; i++) {
        GFile *file = g_list_model_get_item(files, i);
        gchar *path = g_file_get_path(file);

        if (path != NULL)
            filenames = g_slist_append(filenames, path);

        g_object_unref(file);
    }

    return filenames;
}
