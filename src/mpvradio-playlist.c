/*
 * mpvradio-playlist.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include <unistd.h>
#include "glib/gi18n.h"
#include "gtk/gtk.h"
#include "glib.h"
#include "gdk/gdkkeysyms.h"
#include "tag_c.h"

/*
 * 曲ファイルからtag情報を得て store にセットする
 */
static gboolean store_songinfo (char *songfile, GtkListStore *store)
{
    TagLib_File *tagfile;
    TagLib_Tag *taglib;
    TagLib_AudioProperties *ap;
    GtkTreeIter iter;
    char *title, *playtime;
    unsigned int track;
    int length;

    if (fopen (songfile, "r") == NULL) {
        return FALSE;
    }

    tagfile = taglib_file_new (songfile);
    if (tagfile == NULL) {
        return FALSE;
    }

    taglib = taglib_file_tag (tagfile);
    title = taglib_tag_title (taglib);
    track = taglib_tag_track (taglib);


    // 曲名が空白ならファイル名で代用する。処理後のg_free に備えて、
    // 曲名がある場合もコピーを作成する
    //~ title = (strlen (title) == 0) ?
                //~ g_basename (songfile) : g_strdup (title);

    ap = taglib_file_audioproperties (tagfile);
    length = taglib_audioproperties_length (ap);
    //~ playtime = g_strdup_printf ("%d分%d秒", length / 60, length % 60);
    playtime = g_strdup_printf ("%d:%02d", length / 60, length % 60);

    //~ gtk_list_store_insert (store, &iter, 0);
    gtk_list_store_append (store, &iter);
    if (track == 0) {
        gtk_list_store_set (store, &iter,
                            1, title,
                            2, playtime,
                            3, songfile,
                            -1 );
    }
    else {
        gtk_list_store_set (store, &iter,
                            0, track,
                            1, title,
                            2, playtime,
                            3, songfile,
                            -1 );
    }
    //~ g_free (title);
    g_free (playtime);

    taglib_tag_free_strings ();
    taglib_file_free (tagfile);

    return TRUE;
}


/*
 * プレイリストファイル (.pls, .m3u) あるいは曲ファイルからタグを読んで
 * store に格納する
 */
gboolean read_songlist (char *filename, GtkListStore *playstore)
{
    char *ext;

    ext = strrchr (filename, '.');
    if (!strcmp (ext, ".pls")) {
        GError *err = NULL;
        GKeyFile *kf;

        kf = g_key_file_new ();
        if (g_key_file_load_from_file(kf, filename,
                        G_KEY_FILE_NONE, NULL) == TRUE) {
            gchar **keys, **pos;
            keys = pos = g_key_file_get_keys (kf, "playlist",NULL,NULL);
            if (keys != NULL) {
                while (*pos != NULL) {
                    //~ g_print ("key:%s ", *pos);
                    if (!memcmp (*pos, "File", 4)) {
                        gchar *tmp = g_key_file_get_string(kf, "playlist", *pos, NULL);
                        if (strlen (tmp) != 0) {
                            store_songinfo (tmp, playstore);
                        }
                    }
                    pos++;
                }
                g_strfreev (keys);
            }
        }
        g_key_file_unref (kf);
    }
    else if (!strcmp (ext, ".m3u") || !strcmp (ext, ".m3u8")) {
        char buf[1024], *s, *t;
        FILE *fp = fopen (filename, "rt");
        while (!feof (fp)) {
            if ((s = fgets (buf, sizeof (buf)-1, fp)) == NULL) break;
            s = g_strchug (s);
            if (*s == '#') continue;    // コメント行は飛ばす
            if ((t = strrchr (s, '\n')) != NULL) *t = '\0';
            printf ("%s\n", s);
            if (strlen (s) != 0) {
                store_songinfo (s, playstore);
            }
        }
        fclose (fp);
    }
    else {
        // 単独の曲ファイルとみなしてタグを読み込む
        store_songinfo (filename, playstore);
    }
    return TRUE;
}

GtkWidget *playlist_viewer_new (GtkListStore *playstore)
{
    GtkWidget *view, *scroll;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;

    view = gtk_tree_view_new_with_model (GTK_TREE_MODEL(playstore));
    gtk_tree_view_set_headers_visible (view, TRUE);

    // トラック番号
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (
                _("track"), renderer, "text", 0, NULL );
    gtk_tree_view_column_set_max_width (column, 40);
    g_object_set (column, "alignment", 0.5, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, 0);
    gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

    // 曲名
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (
                _("song"), renderer, "text", 1, NULL );
    gtk_tree_view_column_set_max_width (column, 600);
    g_object_set (column, "alignment", 0.5, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, 1);
    gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

    // 演奏時間
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (
                _("time"), renderer, "text", 2, NULL );
    gtk_tree_view_column_set_max_width (column, 140);
    g_object_set (column, "alignment", 0.5, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, 2);
    gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_kinetic_scrolling (scroll, TRUE);
    gtk_scrolled_window_set_capture_button_press (scroll, TRUE);
    gtk_scrolled_window_set_overlay_scrolling (scroll, TRUE);
    gtk_scrolled_window_set_propagate_natural_height (scroll, TRUE);
    gtk_scrolled_window_set_propagate_natural_width (scroll, TRUE);


    gtk_container_add (scroll, view);
    gtk_widget_show_all (view);
    return scroll;
}

