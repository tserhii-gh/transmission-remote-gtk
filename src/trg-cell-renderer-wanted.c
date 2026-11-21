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

#include <math.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdint.h>

#include "protocol-constants.h"
#include "trg-cell-renderer-wanted.h"
#include "trg-files-model.h"
#include "util.h"

enum {
    PROP_0,
    PROP_WANTED_VALUE
};

G_DEFINE_TYPE(TrgCellRendererWanted, trg_cell_renderer_wanted, GTK_TYPE_CELL_RENDERER)
#define TRG_CELL_RENDERER_WANTED_GET_PRIVATE(o)                                                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), TRG_TYPE_CELL_RENDERER_WANTED, TrgCellRendererWantedPrivate))
typedef struct _TrgCellRendererWantedPrivate TrgCellRendererWantedPrivate;

struct _TrgCellRendererWantedPrivate {
    gint wanted_value;
    gboolean active;
    gboolean inconsistent;
};

static void trg_cell_renderer_wanted_get_preferred_size(GtkCellRenderer *cell,
                                                        GtkWidget *widget G_GNUC_UNUSED,
                                                        gint *minimum_size, gint *natural_size);
static void trg_cell_renderer_wanted_snapshot(GtkCellRenderer *cell, GtkSnapshot *snapshot,
                                              GtkWidget *widget,
                                              const GdkRectangle *background_area,
                                              const GdkRectangle *cell_area,
                                              GtkCellRendererState flags);

static void trg_cell_renderer_wanted_get_property(GObject *object, guint property_id, GValue *value,
                                                  GParamSpec *pspec)
{
    TrgCellRendererWantedPrivate *priv = TRG_CELL_RENDERER_WANTED_GET_PRIVATE(object);
    switch (property_id) {
    case PROP_WANTED_VALUE:
        g_value_set_int(value, priv->wanted_value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void trg_cell_renderer_wanted_set_property(GObject *object, guint property_id,
                                                  const GValue *value, GParamSpec *pspec)
{
    TrgCellRendererWantedPrivate *priv = TRG_CELL_RENDERER_WANTED_GET_PRIVATE(object);

    if (property_id == PROP_WANTED_VALUE) {
        priv->wanted_value = g_value_get_int(value);
        priv->inconsistent = (priv->wanted_value == TR_PRI_MIXED);
        priv->active = (priv->wanted_value == TRUE);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void trg_cell_renderer_wanted_class_init(TrgCellRendererWantedClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->get_property = trg_cell_renderer_wanted_get_property;
    object_class->set_property = trg_cell_renderer_wanted_set_property;

    g_object_class_install_property(
        object_class, PROP_WANTED_VALUE,
        g_param_spec_int("wanted-value", "Wanted Value", "Wanted Value", TR_PRI_UNSET, TRUE, TRUE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK
                             | G_PARAM_STATIC_BLURB));

    GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS(klass);

    cell_class->get_preferred_width = trg_cell_renderer_wanted_get_preferred_size;
    cell_class->get_preferred_height = trg_cell_renderer_wanted_get_preferred_size;
    cell_class->snapshot = trg_cell_renderer_wanted_snapshot;

    g_type_class_add_private(klass, sizeof(TrgCellRendererWantedPrivate));
}

#define WANTED_ICON_SIZE 16

static void trg_cell_renderer_wanted_get_preferred_size(GtkCellRenderer *cell,
                                                        GtkWidget *widget G_GNUC_UNUSED,
                                                        gint *minimum_size, gint *natural_size)
{
    gint xpad, ypad;

    gtk_cell_renderer_get_padding(cell, &xpad, &ypad);

    if (minimum_size)
        *minimum_size = WANTED_ICON_SIZE + xpad * 2;
    if (natural_size)
        *natural_size = WANTED_ICON_SIZE + ypad * 2;
}

static void trg_cell_renderer_wanted_snapshot(GtkCellRenderer *cell, GtkSnapshot *snapshot,
                                              GtkWidget *widget,
                                              const GdkRectangle *background_area,
                                              const GdkRectangle *cell_area,
                                              GtkCellRendererState flags)
{
    TrgCellRendererWantedPrivate *priv = TRG_CELL_RENDERER_WANTED_GET_PRIVATE(cell);
    const char *icon_name = NULL;
    GtkIconTheme *theme;
    GtkIconPaintable *icon;
    graphene_point_t point;
    gint xpad, ypad, size;

    (void)background_area;
    (void)flags;

    if (priv->inconsistent)
        icon_name = "dialog-question-symbolic";
    else if (priv->active)
        icon_name = "object-select-symbolic";

    if (icon_name == NULL || widget == NULL)
        return;

    theme = gtk_icon_theme_get_for_display(gtk_widget_get_display(widget));
    icon = gtk_icon_theme_lookup_icon(theme, icon_name, NULL, WANTED_ICON_SIZE, 1,
                                      gtk_widget_get_direction(widget), GTK_ICON_LOOKUP_NONE);

    if (icon == NULL)
        return;

    gtk_cell_renderer_get_padding(cell, &xpad, &ypad);
    size = WANTED_ICON_SIZE;

    point.x = cell_area->x + xpad + (cell_area->width - xpad * 2 - size) / 2.0;
    point.y = cell_area->y + ypad + (cell_area->height - ypad * 2 - size) / 2.0;

    gtk_snapshot_save(snapshot);
    gtk_snapshot_translate(snapshot, &point);
    gdk_paintable_snapshot(GDK_PAINTABLE(icon), snapshot, size, size);
    gtk_snapshot_restore(snapshot);

    g_object_unref(icon);
}

static void trg_cell_renderer_wanted_init(TrgCellRendererWanted *self)
{
    (void)self;
}

GtkCellRenderer *trg_cell_renderer_wanted_new(void)
{
    return GTK_CELL_RENDERER(g_object_new(TRG_TYPE_CELL_RENDERER_WANTED, NULL));
}
