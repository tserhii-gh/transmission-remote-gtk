/*
 * transmission-remote-gtk - Transmission RPC client for GTK
 * Copyright (C) 2011  Alan Fitton

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

#include <string.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "torrent.h"
#include "util.h"
#include "trg-general-panel.h"
#include "trg-torrent-model.h"

static void gtk_label_clear(GtkLabel * l);
static GtkLabel *gen_panel_label_get_key_label(GtkLabel * l);
static GtkLabel *trg_general_panel_add_label(TrgGeneralPanel * fixed,
                                             char *key, int col, int row);

G_DEFINE_TYPE(TrgGeneralPanel, trg_general_panel, GTK_TYPE_FIXED)
#define TRG_GENERAL_PANEL_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TRG_TYPE_GENERAL_PANEL, TrgGeneralPanelPrivate))
typedef struct _TrgGeneralPanelPrivate TrgGeneralPanelPrivate;

struct _TrgGeneralPanelPrivate {
    GtkLabel *gen_name_label;
    GtkLabel *gen_size_label;
    GtkLabel *gen_completed_label;
    GtkLabel *gen_seeders_label;
    GtkLabel *gen_leechers_label;
    GtkLabel *gen_status_label;
    GtkLabel *gen_eta_label;
    GtkLabel *gen_downloaded_label;
    GtkLabel *gen_uploaded_label;
    GtkLabel *gen_down_rate_label;
    GtkLabel *gen_up_rate_label;
    GtkLabel *gen_ratio_label;
    GtkLabel *gen_downloaddir_label;
    GtkLabel *gen_error_label;
    GtkTreeModel *model;
};

void trg_general_panel_clear(TrgGeneralPanel * panel)
{
    TrgGeneralPanelPrivate *priv = TRG_GENERAL_PANEL_GET_PRIVATE(panel);

    gtk_label_clear(priv->gen_name_label);
    gtk_label_clear(priv->gen_size_label);
    gtk_label_clear(priv->gen_completed_label);
    gtk_label_clear(priv->gen_seeders_label);
    gtk_label_clear(priv->gen_leechers_label);
    gtk_label_clear(priv->gen_status_label);
    gtk_label_clear(priv->gen_eta_label);
    gtk_label_clear(priv->gen_downloaded_label);
    gtk_label_clear(priv->gen_uploaded_label);
    gtk_label_clear(priv->gen_down_rate_label);
    gtk_label_clear(priv->gen_up_rate_label);
    gtk_label_clear(priv->gen_ratio_label);
    gtk_label_clear(priv->gen_downloaddir_label);
    gtk_label_clear(priv->gen_error_label);
    gtk_label_clear(gen_panel_label_get_key_label
                    (GTK_LABEL(priv->gen_error_label)));
}

static void gtk_label_clear(GtkLabel * l)
{
    gtk_label_set_text(l, "");
}

static GtkLabel *gen_panel_label_get_key_label(GtkLabel * l)
{
    return GTK_LABEL(g_object_get_data(G_OBJECT(l), "key-label"));
}

static void trg_general_panel_class_init(TrgGeneralPanelClass * klass)
{
    g_type_class_add_private(klass, sizeof(TrgGeneralPanelPrivate));
}

void trg_general_panel_update(TrgGeneralPanel * panel, JsonObject * t,
                              GtkTreeIter * iter)
{
    TrgGeneralPanelPrivate *priv;
    gchar buf[32];
    gint sizeOfBuf;
    gchar *statusString;
    const gchar *errorStr;
    gint64 eta;
    gint seeders, leechers;
    GtkLabel *keyLabel;

    priv = TRG_GENERAL_PANEL_GET_PRIVATE(panel);

    sizeOfBuf = sizeof(buf);

    trg_strlsize(buf, torrent_get_size(t));
    gtk_label_set_text(GTK_LABEL(priv->gen_size_label), buf);

    trg_strlspeed(buf, torrent_get_rate_down(t) / KILOBYTE_FACTOR);
    gtk_label_set_text(GTK_LABEL(priv->gen_down_rate_label), buf);

    trg_strlspeed(buf, torrent_get_rate_up(t) / KILOBYTE_FACTOR);
    gtk_label_set_text(GTK_LABEL(priv->gen_up_rate_label), buf);

    trg_strlsize(buf, torrent_get_uploaded(t));
    gtk_label_set_text(GTK_LABEL(priv->gen_uploaded_label), buf);

    trg_strlsize(buf, torrent_get_downloaded(t));
    gtk_label_set_text(GTK_LABEL(priv->gen_downloaded_label), buf);

    trg_strlratio(buf,
                  (double) torrent_get_uploaded(t) /
                  (double) torrent_get_downloaded(t));
    gtk_label_set_text(GTK_LABEL(priv->gen_ratio_label), buf);

    statusString = torrent_get_status_string(torrent_get_status(t));
    gtk_label_set_text(GTK_LABEL(priv->gen_status_label), statusString);
    g_free(statusString);

    trg_strlpercent(buf, torrent_get_percent_done(t));
    gtk_label_set_text(GTK_LABEL(priv->gen_completed_label), buf);

    gtk_label_set_text(GTK_LABEL(priv->gen_name_label),
                       torrent_get_name(t));

    gtk_label_set_text(GTK_LABEL(priv->gen_downloaddir_label),
                       torrent_get_download_dir(t));

    errorStr = torrent_get_errorstr(t);
    keyLabel =
        gen_panel_label_get_key_label(GTK_LABEL(priv->gen_error_label));
    if (strlen(errorStr) > 0) {
        gchar *markup =
            g_markup_printf_escaped("<span fgcolor=\"red\">%s</span>",
                                    errorStr);
        gtk_label_set_markup(GTK_LABEL(priv->gen_error_label),
                             markup);
        g_free(markup);

        markup = g_markup_printf_escaped("<span fgcolor=\"red\">%s</span>",
                                            _("Error"));
        gtk_label_set_markup(keyLabel,
                             markup);
        g_free(markup);
    } else {
        gtk_label_clear(GTK_LABEL(priv->gen_error_label));
        gtk_label_clear(keyLabel);
    }

    if ((eta = torrent_get_eta(t)) > 0) {
        tr_strltime_long(buf, eta, sizeOfBuf);
        gtk_label_set_text(GTK_LABEL(priv->gen_eta_label), buf);
    } else {
        gtk_label_set_text(GTK_LABEL(priv->gen_eta_label), _("N/A"));
    }

    seeders = 0;
    leechers = 0;
    gtk_tree_model_get(GTK_TREE_MODEL(priv->model), iter,
                       TORRENT_COLUMN_SEEDS, &seeders,
                       TORRENT_COLUMN_LEECHERS, &leechers, -1);

    snprintf(buf, sizeof(buf), "%d", seeders);
    gtk_label_set_text(GTK_LABEL(priv->gen_seeders_label), buf);
    snprintf(buf, sizeof(buf), "%d", leechers);
    gtk_label_set_text(GTK_LABEL(priv->gen_leechers_label), buf);
}

static GtkLabel *trg_general_panel_add_label(TrgGeneralPanel * fixed,
                                             char *key, int col, int row)
{
    GtkWidget *keyLabel;
    GtkWidget *value;
    gchar *keyMarkup;

    keyLabel = gtk_label_new(NULL);
    keyMarkup = g_markup_printf_escaped(strlen(key) > 0 ? "<b>%s:</b>" : "", key);
    gtk_label_set_markup(GTK_LABEL(keyLabel), keyMarkup);
    g_free(keyMarkup);
    gtk_fixed_put(GTK_FIXED(fixed), keyLabel, 10 + (col * 280),
                  10 + (row * 22));

    value = gtk_label_new(NULL);
    gtk_label_set_selectable(GTK_LABEL(value), TRUE);
    gtk_fixed_put(GTK_FIXED(fixed), value, 120 + (col * 300),
                  10 + (row * 22));
    g_object_set_data(G_OBJECT(value), "key-label", keyLabel);

    return GTK_LABEL(value);
}

static void trg_general_panel_init(TrgGeneralPanel * self)
{
    TrgGeneralPanelPrivate *priv = TRG_GENERAL_PANEL_GET_PRIVATE(self);

    priv->gen_name_label =
        trg_general_panel_add_label(self, _("Name"), 0, 0);

    priv->gen_size_label =
        trg_general_panel_add_label(self, _("Size"), 0, 1);
    priv->gen_eta_label = trg_general_panel_add_label(self, _("ETA"), 1, 1);
    priv->gen_completed_label =
        trg_general_panel_add_label(self, _("Completed"), 2, 1);

    priv->gen_seeders_label =
        trg_general_panel_add_label(self, _("Seeders"), 0, 2);
    priv->gen_down_rate_label =
        trg_general_panel_add_label(self, _("Rate Down"), 1, 2);
    priv->gen_downloaded_label =
        trg_general_panel_add_label(self, _("Downloaded"), 2, 2);

    priv->gen_leechers_label =
        trg_general_panel_add_label(self, _("Leechers"), 0, 3);
    priv->gen_up_rate_label =
        trg_general_panel_add_label(self, _("Rate Up"), 1, 3);
    priv->gen_uploaded_label =
        trg_general_panel_add_label(self, _("Uploaded"), 2, 3);

    priv->gen_status_label =
        trg_general_panel_add_label(self, _("Status"), 0, 4);
    priv->gen_ratio_label =
        trg_general_panel_add_label(self, _("Ratio"), 1, 4);

    priv->gen_downloaddir_label =
        trg_general_panel_add_label(self, _("Location"), 0, 5);

    priv->gen_error_label = trg_general_panel_add_label(self, "", 0, 6);

    gtk_widget_set_sensitive(GTK_WIDGET(self), FALSE);
}

TrgGeneralPanel *trg_general_panel_new(GtkTreeModel * model)
{
    GObject *obj;
    TrgGeneralPanelPrivate *priv;

    obj = g_object_new(TRG_TYPE_GENERAL_PANEL, NULL);

    priv = TRG_GENERAL_PANEL_GET_PRIVATE(obj);
    priv->model = model;

    return TRG_GENERAL_PANEL(obj);
}
