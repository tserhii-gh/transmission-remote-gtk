/*
 * This file Copyright (C) Mnemosyne LLC
 *
 * This file is licensed by the GPL version 2. Works owned by the
 * Transmission project are granted a special exemption to clause 2(b)
 * so that the bulk of its code can remain under the MIT license.
 * This exemption does not extend to derived works not owned by
 * the Transmission project.
 *
 * $Id: torrent-cell-renderer.h 12658 2011-08-09 05:47:24Z jordan $
 */

#ifndef GTR_TORRENT_CELL_RENDERER_H
#define GTR_TORRENT_CELL_RENDERER_H

#include <gtk/gtk.h>

#define GTR_UNICODE_UP      "\xE2\x86\x91"
#define GTR_UNICODE_DOWN    "\xE2\x86\x93"
#define DIRECTORY_MIME_TYPE "folder"
#define FILE_MIME_TYPE      "file"
#define UNKNOWN_MIME_TYPE   "unknown"

#define TORRENT_CELL_RENDERER_TYPE (torrent_cell_renderer_get_type())

#define TORRENT_CELL_RENDERER(o)                                                                   \
    (G_TYPE_CHECK_INSTANCE_CAST((o), TORRENT_CELL_RENDERER_TYPE, TorrentCellRenderer))

typedef struct TorrentCellRenderer TorrentCellRenderer;

typedef struct TorrentCellRendererClass TorrentCellRendererClass;

struct TorrentCellRenderer {
    GtkCellRenderer parent;

    /*< private > */
    struct TorrentCellRendererPrivate *priv;
};

struct TorrentCellRendererClass {
    GtkCellRendererClass parent;
};

GType torrent_cell_renderer_get_type(void) G_GNUC_CONST;

GtkCellRenderer *torrent_cell_renderer_new(void);
GtkTreeView *torrent_cell_renderer_get_owner(TorrentCellRenderer *r);

#endif /* GTR_TORRENT_CELL_RENDERER_H */
