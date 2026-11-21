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

/*
 * Transitional helpers for the GTK4 port.
 *
 * GTK4 removed the generic GtkContainer API (gtk_container_add(),
 * gtk_container_set_border_width()), the GtkBox packing API and the
 * blocking gtk_dialog_run(). These shims keep the call sites readable
 * while the port is in progress; they can be inlined/removed once the
 * tree is fully native GTK4.
 */

#ifndef TRG_COMPAT_H
#define TRG_COMPAT_H

#include <gtk/gtk.h>

void trg_container_add(gpointer container, GtkWidget *child);
void trg_container_set_border_width(gpointer widget, guint border_width);
void trg_box_pack(gpointer box, GtkWidget *child, gboolean expand, gboolean fill, guint padding);
void trg_box_pack_end(gpointer box, GtkWidget *child, gboolean expand, gboolean fill,
                      guint padding);
gint trg_dialog_run(GtkDialog *dialog);
void trg_widget_destroy(GtkWidget *widget);
void trg_widget_show_all(GtkWidget *widget);
GtkWidget *trg_bin_get_child(GtkWidget *widget);
GtkWidget *trg_widget_get_toplevel(GtkWidget *widget);

/* GTK4 file chooser helpers. */
GSList *trg_file_chooser_get_filenames(GtkFileChooser *chooser);

/* GTK4 replacements for "button-press-event" / "key-press-event".
 * Handlers receive (widget, button, x, y, state, data) and
 * (widget, keyval, keycode, state, data) respectively. */
void trg_widget_add_click_handler(GtkWidget *widget, GCallback cb, gpointer data);
void trg_widget_add_key_handler(GtkWidget *widget, GCallback cb, gpointer data);

#endif /* TRG_COMPAT_H */
