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
 * Popover based menus for GTK4.
 *
 * GTK4 removed GtkMenuBar/GtkMenu/GtkMenuItem and the GtkAccelGroup API.
 * The application builds its menus programmatically and keeps pointers to
 * individual items, so rather than converting everything to GMenuModel +
 * GAction we provide a small widget toolkit matching the old usage:
 * menu buttons that emit "trg-activate", check buttons that emit "toggled" and
 * nested popovers for submenus.
 */

#ifndef TRG_MENU_H
#define TRG_MENU_H

#include <gtk/gtk.h>

typedef struct _TrgMenu TrgMenu;

/* A button that emits "trg-activate" when clicked. */
#define TRG_TYPE_MENU_ITEM (trg_menu_item_get_type())
G_DECLARE_FINAL_TYPE(TrgMenuItem, trg_menu_item, TRG, MENU_ITEM, GtkButton)

GtkWidget *trg_menu_item_new(const char *label, const char *icon_name);
void trg_menu_item_set_label(GtkWidget *item, const char *label);

TrgMenu *trg_menu_new(void);
GtkWidget *trg_menu_get_widget(TrgMenu *menu);
GtkWidget *trg_menu_get_box(TrgMenu *menu);

GtkWidget *trg_menu_append_item(TrgMenu *menu, const char *label, const char *icon_name,
                                gboolean sensitive);
GtkWidget *trg_menu_check_new(const char *label, gboolean active);
GtkWidget *trg_menu_append_check(TrgMenu *menu, const char *label, gboolean active);
GtkWidget *trg_menu_append_separator(TrgMenu *menu);
GtkWidget *trg_menu_append_widget(TrgMenu *menu, GtkWidget *widget);
/* Appends a row that opens "submenu" and returns the row widget. */
GtkWidget *trg_menu_append_submenu(TrgMenu *menu, const char *label, const char *icon_name,
                                   TrgMenu *submenu);

/* Menu bar button (a GtkMenuButton showing "label" that opens "menu"). */
GtkWidget *trg_menu_bar_button_new(const char *label, TrgMenu *menu);

void trg_menu_popup_at_pointer(TrgMenu *menu, GtkWidget *relative_to, double x, double y);
void trg_menu_popup_at_widget(TrgMenu *menu, GtkWidget *relative_to);

#endif /* TRG_MENU_H */
