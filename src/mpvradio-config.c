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

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "libxapp/xapp-preferences-window.h"

#include "mpvradio.h"


void detach_config_file (GKeyFile *kf)
{
    g_key_file_unref (kf);
}

void save_config_file (GKeyFile *kf)
{
    gchar *conf_file_name;
    gchar *filename, *dir, *sel;

    dir = g_strdup_printf ("%s/%s", g_get_user_config_dir (), PACKAGE);
    conf_file_name = g_strdup_printf ("%s/%s.conf", dir, PACKAGE);

    if (g_key_file_save_to_file (kf, conf_file_name, NULL) == FALSE) {
        /* 設定ファイルの保存に失敗しました。*/
        g_warning (N_("Failed to save the configuration file."));
    }
    g_free (conf_file_name);
    g_free (dir);
}

GKeyFile *load_config_file (void)
{
    gchar *conf_file_name;
    gchar *filename, *dir, *sel;
    GError *err = NULL;
    GKeyFile *kf;

    kf = g_key_file_new ();
    dir = g_strdup_printf ("%s/%s", g_get_user_config_dir (), PACKAGE);
    conf_file_name = g_strdup_printf ("%s/%s.conf", dir, PACKAGE);
    //~ g_print ("conf file %s\n", conf_file_name);
    if (g_key_file_load_from_file(kf, conf_file_name,
            G_KEY_FILE_NONE, NULL) == FALSE) {

        gchar *default_conf = g_strdup_printf (
            "[%s]\n" \
            "version=%s\n" \
            "\n"    \
            "[playlist]\n"   \
            "radiko=%s/mpvradio/playlists/00_radiko.m3u\n" \
            "other=%s/mpvradio/playlists/radio.m3u\n" \
            "\n", PACKAGE, PACKAGE_VERSION, DATADIR, DATADIR);

        g_key_file_load_from_data (kf, default_conf, -1,
            G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS,
            NULL);
        g_free (default_conf);
        // 設定ファイルが見つからなかったので取り敢えず既定値の保存を試みる
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

static void
grid_destroy_cb (GtkWidget *obj, gpointer user_data)
{
    g_message ("%s destroy.", user_data);
}

static gboolean
password_focus_cb (GtkWidget       *widget,
               //~ GtkDirectionType direction,
                        GdkEvent  *event,
               gpointer         user_data)
{
    g_print ("password\n");
    GdkEventType t;
    t = gdk_event_get_event_type (event);
    if (t == GDK_ENTER_NOTIFY) {
        g_print ("GDK_ENTER_NOTIFY\n");
    }
    else if (t == GDK_LEAVE_NOTIFY) {
        g_print ("GDK_LEAVE_NOTIFY\n");
    }
    else if (t == GDK_FOCUS_CHANGE) {
        if (gtk_switch_get_state (GTK_SWITCH(user_data)) == TRUE) {
            g_print ("enable.\n");
            return FALSE;
        }
        else {
            g_print ("disable.\n");
            return TRUE;
        }
    }
    return FALSE;
}

static void
uasw_activate_cb (GtkWidget *obj, GParamSpec *pspec, gpointer user_data)
{
    gboolean flag;

    flag = gtk_switch_get_state (GTK_SWITCH(obj));
    g_print ("uasw state : %s\n", ((flag == TRUE) ? "TRUE":"FALSE"));
    gtk_widget_set_can_focus (GTK_WIDGET(user_data), flag);
}

