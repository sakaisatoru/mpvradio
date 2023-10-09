/*
 * mpvradio_config.c
 *
 * Copyright 2022 sakai <endeavor2wako@gmail.com>
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
 *
 */

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "glib/gi18n.h"
#include "gtk/gtk.h"

void detach_config_file (GKeyFile *kf)
{
    g_key_file_unref (kf);
}

void save_config_file (GKeyFile *kf)
{
    GError *err = NULL;
    gchar *conf_file_name = g_build_filename (g_get_user_config_dir (),
                                    PACKAGE, PACKAGE".conf", NULL);

    if (g_key_file_save_to_file (kf, conf_file_name, &err) == FALSE) {
        /* 設定ファイルの保存に失敗しました。*/
        g_warning (err->message);
        g_clear_error (&err);
    }
    g_free (conf_file_name);
}

GKeyFile *load_config_file (void)
{
    gchar *conf_file_name, *dir;
    GError *err = NULL;
    GKeyFile *kf;

    dir = g_build_filename (g_get_user_config_dir (), PACKAGE, NULL);
    conf_file_name = g_build_filename (dir, PACKAGE".conf", NULL);

    kf = g_key_file_new ();
    if (g_key_file_load_from_file(kf, conf_file_name,
                            G_KEY_FILE_NONE, NULL) == FALSE) {
        // 設定ファイルが見つからなかったので取り敢えず既定値の保存を試みる
        const gchar *default_conf =
            "["PACKAGE"]\n" \
            "version="PACKAGE_VERSION"\n" \
            "\n"    \
            "[playlist]\n"   \
            "other="DATADIR"/mpvradio/playlists/radio.m3u\n" \
            "\n";

        g_key_file_load_from_data (kf, default_conf, -1,
            G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS,
            NULL);
        g_mkdir (dir, S_IRWXU);
        if (g_key_file_save_to_file (kf, conf_file_name, &err) == FALSE) {
            g_warning (err->message);
            g_clear_error (&err);
        }
    }

    g_free (conf_file_name);
    g_free (dir);

    return kf;
}

