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

#include "trg-menu.h"

struct _TrgMenuItem {
    GtkButton parent;
};

G_DEFINE_TYPE(TrgMenuItem, trg_menu_item, GTK_TYPE_BUTTON)

static void trg_menu_item_clicked(GtkButton *button, gpointer user_data)
{
    (void)user_data;

    g_signal_emit_by_name(button, "trg-activate");
}

static void trg_menu_item_class_init(TrgMenuItemClass *klass)
{
    g_signal_new("trg-activate", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                 G_TYPE_NONE, 0);
}

static void trg_menu_item_init(TrgMenuItem *self)
{
    gtk_widget_add_css_class(GTK_WIDGET(self), "flat");
    gtk_widget_set_halign(GTK_WIDGET(self), GTK_ALIGN_FILL);
    g_signal_connect(self, "clicked", G_CALLBACK(trg_menu_item_clicked), NULL);
}

GtkWidget *trg_menu_item_new(const char *label, const char *icon_name)
{
    GtkWidget *item = g_object_new(TRG_TYPE_MENU_ITEM, NULL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    if (icon_name != NULL) {
        GtkWidget *image = gtk_image_new_from_icon_name(icon_name);
        gtk_image_set_pixel_size(GTK_IMAGE(image), 16);
        gtk_box_append(GTK_BOX(box), image);
    }

    if (label != NULL) {
        GtkWidget *lbl = gtk_label_new(label);
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        gtk_label_set_use_underline(GTK_LABEL(lbl), TRUE);
        gtk_widget_set_hexpand(lbl, TRUE);
        gtk_box_append(GTK_BOX(box), lbl);
    }

    gtk_button_set_child(GTK_BUTTON(item), box);
    gtk_widget_set_halign(item, GTK_ALIGN_FILL);

    return item;
}

void trg_menu_item_set_label(GtkWidget *item, const char *label)
{
    GtkWidget *box = gtk_button_get_child(GTK_BUTTON(item));
    GtkWidget *lbl;

    if (box == NULL)
        return;

    lbl = gtk_widget_get_last_child(box);
    if (GTK_IS_LABEL(lbl))
        gtk_label_set_text(GTK_LABEL(lbl), label);
}

struct _TrgMenu {
    GtkWidget *popover;
    GtkWidget *box;
};

static void trg_menu_popdown(GtkWidget *widget)
{
    GtkWidget *popover;

    /* Close the immediate menu and any parent menus it is nested in. */
    while ((popover = gtk_widget_get_ancestor(widget, GTK_TYPE_POPOVER)) != NULL) {
        gtk_popover_popdown(GTK_POPOVER(popover));
        widget = gtk_widget_get_parent(popover);

        if (widget == NULL)
            break;
    }
}

static void trg_menu_item_clicked_close(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;
    trg_menu_popdown(widget);
}

static void trg_menu_free(gpointer data)
{
    TrgMenu *menu = data;

    g_free(menu);
}

TrgMenu *trg_menu_new(void)
{
    TrgMenu *menu = g_new0(TrgMenu, 1);

    menu->popover = gtk_popover_new();
    gtk_popover_set_has_arrow(GTK_POPOVER(menu->popover), FALSE);
    gtk_widget_add_css_class(menu->popover, "menu");

    menu->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_add_css_class(menu->box, "menu");
    gtk_popover_set_child(GTK_POPOVER(menu->popover), menu->box);

    g_object_set_data_full(G_OBJECT(menu->popover), "trg-menu", menu, trg_menu_free);

    return menu;
}

GtkWidget *trg_menu_get_widget(TrgMenu *menu)
{
    return menu->popover;
}

GtkWidget *trg_menu_get_box(TrgMenu *menu)
{
    return menu->box;
}

GtkWidget *trg_menu_append_widget(TrgMenu *menu, GtkWidget *widget)
{
    gtk_box_append(GTK_BOX(menu->box), widget);
    return widget;
}

GtkWidget *trg_menu_append_item(TrgMenu *menu, const char *label, const char *icon_name,
                                gboolean sensitive)
{
    GtkWidget *item = trg_menu_item_new(label, icon_name);

    gtk_widget_set_sensitive(item, sensitive);
    g_signal_connect(item, "trg-activate", G_CALLBACK(trg_menu_item_clicked_close), NULL);
    trg_menu_append_widget(menu, item);

    return item;
}

GtkWidget *trg_menu_check_new(const char *label, gboolean active)
{
    GtkWidget *check = gtk_check_button_new_with_label(label);

    gtk_check_button_set_active(GTK_CHECK_BUTTON(check), active);
    gtk_widget_set_halign(check, GTK_ALIGN_FILL);
    g_signal_connect_after(check, "toggled", G_CALLBACK(trg_menu_item_clicked_close), NULL);

    return check;
}

GtkWidget *trg_menu_append_check(TrgMenu *menu, const char *label, gboolean active)
{
    GtkWidget *check = trg_menu_check_new(label, active);

    gtk_box_append(GTK_BOX(menu->box), check);

    return check;
}

GtkWidget *trg_menu_append_separator(TrgMenu *menu)
{
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);

    gtk_box_append(GTK_BOX(menu->box), sep);
    return sep;
}

GtkWidget *trg_menu_append_submenu(TrgMenu *menu, const char *label, const char *icon_name,
                                   TrgMenu *submenu)
{
    GtkWidget *button = gtk_menu_button_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    if (icon_name != NULL) {
        GtkWidget *image = gtk_image_new_from_icon_name(icon_name);
        gtk_image_set_pixel_size(GTK_IMAGE(image), 16);
        gtk_box_append(GTK_BOX(box), image);
    }

    GtkWidget *lbl = gtk_label_new(label);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
    gtk_label_set_use_underline(GTK_LABEL(lbl), TRUE);
    gtk_widget_set_hexpand(lbl, TRUE);
    gtk_box_append(GTK_BOX(box), lbl);

    gtk_menu_button_set_child(GTK_MENU_BUTTON(button), box);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(button), submenu->popover);
    gtk_widget_set_halign(button, GTK_ALIGN_FILL);
    gtk_widget_add_css_class(button, "flat");

    gtk_box_append(GTK_BOX(menu->box), button);

    return button;
}

GtkWidget *trg_menu_bar_button_new(const char *label, TrgMenu *menu)
{
    GtkWidget *button = gtk_menu_button_new();
    GtkWidget *lbl = gtk_label_new(label);

    gtk_label_set_use_underline(GTK_LABEL(lbl), TRUE);
    gtk_menu_button_set_child(GTK_MENU_BUTTON(button), lbl);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(button), menu->popover);
    gtk_widget_add_css_class(button, "flat");

    return button;
}

static void trg_menu_popover_closed(GtkPopover *popover, gpointer data)
{
    (void)data;

    if (gtk_widget_get_parent(GTK_WIDGET(popover)) != NULL)
        gtk_widget_unparent(GTK_WIDGET(popover));
}

void trg_menu_popup_at_pointer(TrgMenu *menu, GtkWidget *relative_to, double x, double y)
{
    GdkRectangle bounds = { (int)x, (int)y, 1, 1 };

    gtk_widget_set_parent(menu->popover, relative_to);
    g_signal_connect(menu->popover, "closed", G_CALLBACK(trg_menu_popover_closed), NULL);
    gtk_popover_set_pointing_to(GTK_POPOVER(menu->popover), &bounds);
    gtk_popover_popup(GTK_POPOVER(menu->popover));
}

void trg_menu_popup_at_widget(TrgMenu *menu, GtkWidget *relative_to)
{
    gtk_widget_set_parent(menu->popover, relative_to);
    g_signal_connect(menu->popover, "closed", G_CALLBACK(trg_menu_popover_closed), NULL);
    gtk_popover_popup(GTK_POPOVER(menu->popover));
}
