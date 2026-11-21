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
#include <stdint.h>

#include "trg-cell-renderer-file-icon.h"
#include "util.h"

enum {
    PROP_0,
    PROP_FILE_ID,
    PROP_FILE_NAME
};

G_DEFINE_TYPE(TrgCellRendererFileIcon, trg_cell_renderer_file_icon, GTK_TYPE_CELL_RENDERER)
#define TRG_CELL_RENDERER_FILE_ICON_GET_PRIVATE(o)                                                 \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), TRG_TYPE_CELL_RENDERER_FILE_ICON,                            \
                                 TrgCellRendererFileIconPrivate))
typedef struct _TrgCellRendererFileIconPrivate TrgCellRendererFileIconPrivate;

struct _TrgCellRendererFileIconPrivate {
    gint64 file_id;
    gchar *text;
    GIcon *icon;
};

static void trg_cell_renderer_file_icon_get_preferred_width(GtkCellRenderer *cell, GtkWidget *widget,
                                                            gint *minimum_size, gint *natural_size);
static void trg_cell_renderer_file_icon_get_preferred_height(GtkCellRenderer *cell,
                                                             GtkWidget *widget,
                                                             gint *minimum_size,
                                                             gint *natural_size);
static void trg_cell_renderer_file_icon_snapshot(GtkCellRenderer *cell, GtkSnapshot *snapshot,
                                                 GtkWidget *widget,
                                                 const GdkRectangle *background_area,
                                                 const GdkRectangle *cell_area,
                                                 GtkCellRendererState flags);

static void trg_cell_renderer_file_icon_get_property(GObject *object, guint property_id,
                                                     GValue *value, GParamSpec *pspec)
{
    TrgCellRendererFileIconPrivate *priv = TRG_CELL_RENDERER_FILE_ICON_GET_PRIVATE(object);
    switch (property_id) {
    case PROP_FILE_ID:
        g_value_set_int64(value, priv->file_id);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void trg_cell_renderer_file_icon_refresh(TrgCellRendererFileIcon *fi)
{
    TrgCellRendererFileIconPrivate *priv = TRG_CELL_RENDERER_FILE_ICON_GET_PRIVATE(fi);

    g_clear_object(&priv->icon);

    if (priv->file_id == -2) {
        return;
    } else if (priv->file_id == -1) {
        priv->icon = g_themed_icon_new("folder");
    } else if (priv->text) {
#ifndef G_OS_WIN32
        gboolean uncertain;
        gchar *mimetype = g_content_type_guess(priv->text, NULL, 0, &uncertain);

        if (!uncertain && mimetype)
            priv->icon = g_content_type_get_icon(mimetype);

        g_free(mimetype);
#endif
        if (priv->icon == NULL)
            priv->icon = g_themed_icon_new("text-x-generic");
    }
}

static void trg_cell_renderer_file_icon_set_property(GObject *object, guint property_id,
                                                     const GValue *value, GParamSpec *pspec)
{
    TrgCellRendererFileIconPrivate *priv = TRG_CELL_RENDERER_FILE_ICON_GET_PRIVATE(object);
    if (property_id == PROP_FILE_ID) {
        priv->file_id = g_value_get_int64(value);
        trg_cell_renderer_file_icon_refresh(TRG_CELL_RENDERER_FILE_ICON(object));
    } else if (property_id == PROP_FILE_NAME) {
        if (priv->file_id != -1) {
            g_free(priv->text);
            priv->text = g_strdup(g_value_get_string(value));
            trg_cell_renderer_file_icon_refresh(TRG_CELL_RENDERER_FILE_ICON(object));
        }
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void trg_cell_renderer_file_icon_dispose(GObject *object)
{
    TrgCellRendererFileIconPrivate *priv = TRG_CELL_RENDERER_FILE_ICON_GET_PRIVATE(object);
    g_free(priv->text);
    g_clear_object(&priv->icon);
    G_OBJECT_CLASS(trg_cell_renderer_file_icon_parent_class)->dispose(object);
}

static void trg_cell_renderer_file_icon_class_init(TrgCellRendererFileIconClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->get_property = trg_cell_renderer_file_icon_get_property;
    object_class->set_property = trg_cell_renderer_file_icon_set_property;
    object_class->dispose = trg_cell_renderer_file_icon_dispose;

    g_object_class_install_property(
        object_class, PROP_FILE_ID,
        g_param_spec_int64("file-id", "File ID", "File ID", -2, G_MAXINT64, -2,
                           G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK
                               | G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class, PROP_FILE_NAME,
                                    g_param_spec_string("file-name", "Filename", "Filename", NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_NAME
                                                            | G_PARAM_STATIC_NICK
                                                            | G_PARAM_STATIC_BLURB));

    GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS(klass);

    cell_class->get_preferred_width = trg_cell_renderer_file_icon_get_preferred_width;
    cell_class->get_preferred_height = trg_cell_renderer_file_icon_get_preferred_height;
    cell_class->snapshot = trg_cell_renderer_file_icon_snapshot;

    g_type_class_add_private(klass, sizeof(TrgCellRendererFileIconPrivate));
}

#define FILE_ICON_SIZE 16

static void trg_cell_renderer_file_icon_get_preferred_width(GtkCellRenderer *cell,
                                                            GtkWidget *widget G_GNUC_UNUSED,
                                                            gint *minimum_size, gint *natural_size)
{
    gint xpad, ypad;

    gtk_cell_renderer_get_padding(cell, &xpad, &ypad);

    if (minimum_size)
        *minimum_size = FILE_ICON_SIZE + xpad * 2;
    if (natural_size)
        *natural_size = FILE_ICON_SIZE + xpad * 2;
}

static void trg_cell_renderer_file_icon_get_preferred_height(GtkCellRenderer *cell,
                                                             GtkWidget *widget G_GNUC_UNUSED,
                                                             gint *minimum_size, gint *natural_size)
{
    gint xpad, ypad;

    gtk_cell_renderer_get_padding(cell, &xpad, &ypad);

    if (minimum_size)
        *minimum_size = FILE_ICON_SIZE + ypad * 2;
    if (natural_size)
        *natural_size = FILE_ICON_SIZE + ypad * 2;
}

static void trg_cell_renderer_file_icon_snapshot(GtkCellRenderer *cell, GtkSnapshot *snapshot,
                                                 GtkWidget *widget,
                                                 const GdkRectangle *background_area,
                                                 const GdkRectangle *cell_area,
                                                 GtkCellRendererState flags)
{
    TrgCellRendererFileIconPrivate *priv = TRG_CELL_RENDERER_FILE_ICON_GET_PRIVATE(cell);
    GtkIconTheme *theme;
    GtkIconPaintable *icon;
    graphene_point_t point;
    gint xpad, ypad;

    (void)background_area;
    (void)flags;

    if (priv->icon == NULL || widget == NULL)
        return;

    theme = gtk_icon_theme_get_for_display(gtk_widget_get_display(widget));
    icon = gtk_icon_theme_lookup_by_gicon(theme, priv->icon, FILE_ICON_SIZE, 1,
                                          gtk_widget_get_direction(widget), GTK_ICON_LOOKUP_NONE);

    if (icon == NULL)
        return;

    gtk_cell_renderer_get_padding(cell, &xpad, &ypad);

    point.x = cell_area->x + xpad;
    point.y = cell_area->y + ypad + (cell_area->height - ypad * 2 - FILE_ICON_SIZE) / 2.0;

    gtk_snapshot_save(snapshot);
    gtk_snapshot_translate(snapshot, &point);
    gdk_paintable_snapshot(GDK_PAINTABLE(icon), snapshot, FILE_ICON_SIZE, FILE_ICON_SIZE);
    gtk_snapshot_restore(snapshot);

    g_object_unref(icon);
}

static void trg_cell_renderer_file_icon_init(TrgCellRendererFileIcon *self)
{
    (void)self;
}

GtkCellRenderer *trg_cell_renderer_file_icon_new(void)
{
    return GTK_CELL_RENDERER(g_object_new(TRG_TYPE_CELL_RENDERER_FILE_ICON, NULL));
}
