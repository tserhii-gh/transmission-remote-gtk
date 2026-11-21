/*
 * icons.[ch] written by Paolo Bacchilega, who writes:
 * "There is no problem for me, you can license my code
 * under whatever licence you wish :)"
 *
 * $Id: icons.c 12639 2011-08-07 18:41:13Z jordan $
 */

#include "config.h"

#include "icons.h"
#include <gio/gio.h>
#include <glib.h>
#include <string.h> /* strcmp */

#define VOID_PIXBUF_KEY "void-pixbuf"

static const char *get_static_string(const char *s)
{
    static GStringChunk *static_strings = NULL;

    if (s == NULL)
        return NULL;

    if (static_strings == NULL)
        static_strings = g_string_chunk_new(1024);

    return g_string_chunk_insert_const(static_strings, s);
}

typedef struct {
    GtkIconTheme *icon_theme;
    int icon_size;
    GHashTable *cache;
} IconCache;

static IconCache *icon_cache[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };

static GdkPixbuf *create_void_pixbuf(int width, int height)
{
    GdkPixbuf *p;

    p = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
    gdk_pixbuf_fill(p, 0xFFFFFF00);

    return p;
}

static int get_size_in_pixels(int icon_size)
{
    return MAX(icon_size, 1);
}

/* GTK4 has no gtk_icon_info_load_icon(); render the paintable to a pixbuf. */
static GdkPixbuf *paintable_to_pixbuf(GdkPaintable *paintable, int size)
{
    GtkSnapshot *snapshot = gtk_snapshot_new();
    GskRenderNode *node;
    cairo_surface_t *surface;
    cairo_t *cr;
    GdkPixbuf *pixbuf;

    gdk_paintable_snapshot(paintable, snapshot, size, size);
    node = gtk_snapshot_free_to_node(snapshot);

    if (node == NULL)
        return NULL;

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cr = cairo_create(surface);
    gsk_render_node_draw(node, cr);
    cairo_destroy(cr);

    pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, size, size);

    cairo_surface_destroy(surface);
    gsk_render_node_unref(node);

    return pixbuf;
}

static IconCache *icon_cache_new(GtkWidget *for_widget, int icon_size)
{
    IconCache *icon_cache;

    g_return_val_if_fail(for_widget != NULL, NULL);

    icon_cache = g_new0(IconCache, 1);
    icon_cache->icon_theme = gtk_icon_theme_get_for_display(gtk_widget_get_display(for_widget));
    icon_cache->icon_size = get_size_in_pixels(icon_size);
    icon_cache->cache = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_object_unref);

    g_hash_table_insert(icon_cache->cache, (void *)VOID_PIXBUF_KEY,
                        create_void_pixbuf(icon_cache->icon_size, icon_cache->icon_size));

    return icon_cache;
}

static const char *_icon_cache_get_icon_key(GIcon *icon)
{
    const char *key = NULL;

    if (G_IS_THEMED_ICON(icon)) {
        char **icon_names;
        char *name;

        g_object_get(icon, "names", &icon_names, NULL);
        name = g_strjoinv(",", icon_names);

        key = get_static_string(name);

        g_free(name);
        g_strfreev(icon_names);
    } else if (G_IS_FILE_ICON(icon)) {
        GFile *file;
        char *filename;

        file = g_file_icon_get_file(G_FILE_ICON(icon));
        filename = g_file_get_path(file);

        key = get_static_string(filename);

        g_free(filename);
    }

    return key;
}

static GdkPixbuf *get_themed_icon_pixbuf(GThemedIcon *icon, int size, GtkIconTheme *icon_theme)
{
    char **icon_names = NULL;
    GtkIconPaintable *paintable = NULL;
    GdkPixbuf *pixbuf;
    guint i;

    g_object_get(icon, "names", &icon_names, NULL);

    for (i = 0; icon_names != NULL && icon_names[i] != NULL; i++) {
        paintable = gtk_icon_theme_lookup_icon(icon_theme, icon_names[i], NULL, size, 1,
                                               GTK_TEXT_DIR_NONE, 0);
        if (paintable != NULL)
            break;
    }

    if (paintable == NULL)
        paintable = gtk_icon_theme_lookup_icon(icon_theme, "text-x-generic", NULL, size, 1,
                                               GTK_TEXT_DIR_NONE, 0);

    pixbuf = paintable ? paintable_to_pixbuf(GDK_PAINTABLE(paintable), size) : NULL;

    g_clear_object(&paintable);
    g_strfreev(icon_names);

    return pixbuf;
}

static GdkPixbuf *get_file_icon_pixbuf(GFileIcon *icon, int size)
{
    GFile *file;
    char *filename;
    GdkPixbuf *pixbuf;

    file = g_file_icon_get_file(icon);
    filename = g_file_get_path(file);
    pixbuf = gdk_pixbuf_new_from_file_at_size(filename, size, -1, NULL);
    g_free(filename);

    return pixbuf;
}

static GdkPixbuf *_get_icon_pixbuf(GIcon *icon, int size, GtkIconTheme *theme)
{
    if (icon == NULL)
        return NULL;
    if (G_IS_THEMED_ICON(icon))
        return get_themed_icon_pixbuf(G_THEMED_ICON(icon), size, theme);
    if (G_IS_FILE_ICON(icon))
        return get_file_icon_pixbuf(G_FILE_ICON(icon), size);
    return NULL;
}

static GdkPixbuf *icon_cache_get_mime_type_icon(IconCache *icon_cache, const char *mime_type)
{
    GIcon *icon;
    const char *key = NULL;
    GdkPixbuf *pixbuf;

    icon = g_content_type_get_icon(mime_type);
    key = _icon_cache_get_icon_key(icon);

    if (key == NULL)
        key = VOID_PIXBUF_KEY;

    pixbuf = g_hash_table_lookup(icon_cache->cache, key);
    if (pixbuf != NULL) {
        g_object_ref(pixbuf);
        g_object_unref(G_OBJECT(icon));
        return pixbuf;
    }

    pixbuf = _get_icon_pixbuf(icon, icon_cache->icon_size, icon_cache->icon_theme);
    if (pixbuf != NULL)
        g_hash_table_insert(icon_cache->cache, (gpointer)key, g_object_ref(pixbuf));

    g_object_unref(G_OBJECT(icon));

    return pixbuf;
}

GdkPixbuf *gtr_get_mime_type_icon(const char *mime_type, int icon_size,
                                  GtkWidget *for_widget)
{
    int n;

    switch (icon_size) {
    case 16:
        n = 1;
        break;
    case 24:
        n = 2;
        break;
    case 32:
        n = 3;
        break;
    case 48:
        n = 4;
        break;
    case 64:
        n = 5;
        break;
    case 96:
        n = 6;
        break;
    default:
        n = 0;
        break;
    }

    if (icon_cache[n] == NULL)
        icon_cache[n] = icon_cache_new(for_widget, icon_size);

    return icon_cache_get_mime_type_icon(icon_cache[n], mime_type);
}

const char *gtr_get_mime_type_from_filename(const char *file G_GNUC_UNUSED)
{
    char *tmp = g_content_type_guess(file, NULL, 0, NULL);
    const char *ret = get_static_string(tmp);
    g_free(tmp);
    return ret;
}
