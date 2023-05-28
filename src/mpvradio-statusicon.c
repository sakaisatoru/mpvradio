/*
 * mpvradio-statusicon.c
 *
 * Copyright 2022 sakai satoru <endeavor2wako@gmail.com>
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
#include <libgen.h>
#include <glib.h>

#include "glib/gi18n.h"
#include "gtk/gtk.h"
#include "gdk/gdkkeysyms.h"
#include "libnotify/notification.h"
#include "libnotify/notify.h"

#include "libxapp/xapp-status-icon.h"
#include "libxapp/xapp-preferences-window.h"

#include "mpvradio.h"
#include "mpvradio-stationbutton.h"
#include "mpvradio-common.h"

/*
 * デスクトップ用ステータスアイコン (Linux Mint 専用)
 * シグナルコールバック
 */
static void
mpvradio_statusicon_activate_cb (XAppStatusIcon *icon,
               guint           button,
               guint           time,
               gpointer        app)
{
    GList *windows;
    GtkWindow *radiopanel;
    windows = gtk_application_get_windows (app);
    if (windows != NULL) {
        radiopanel = windows->data;
        if (button == 1) {
            if (gtk_widget_get_visible (radiopanel)) {
                gtk_widget_hide (radiopanel);
            }
            else {
                gtk_widget_show_all (radiopanel);
            }
        }
    }
}

extern GtkWidget *volume_up_button, *volume_down_button;
static void
mpvradio_statusicon_scroll_event_cb (XAppStatusIcon *icon,
               gint                amount,
               XAppScrollDirection direction,
               guint               time,
               gpointer            app)
{
    if (direction == XAPP_SCROLL_UP) {
        g_signal_emit_by_name (volume_up_button, "clicked", NULL);
    }
    else if (direction == XAPP_SCROLL_DOWN) {
        g_signal_emit_by_name (volume_down_button, "clicked", NULL);
    }
}

static void
menu_quit_cb (GtkWidget *menuitem, gpointer app)
{
    g_application_quit (G_APPLICATION (app));
}

extern GHashTable *playlist_table;  // main.c
extern GList *playlist_sorted;

/*
 * 選局メニューのコールバック
 */
static void
_mpvradio_statusicon_menuitem_cb (GtkWidget *menuitem,
                                                gpointer data)
{
    char *label = gtk_menu_item_get_label (menuitem);
    char *message;
    char *url = g_hash_table_lookup (playlist_table, label);
    mpvradio_ipc_send ("{\"command\": [\"set_property\", \"pause\", false]}\x0a");
    message = g_strdup_printf ("{\"command\": [\"loadfile\",\"%s\"]}\x0a", url);

    mpvradio_ipc_send (message);
    g_free (message);
}

static void
mpvradio_statusicon_button_release_event_cb (XAppStatusIcon *icon,
               gint            x,
               gint            y,
               guint           button,
               guint           time,
               gint            panel_position,
               gpointer        app)
{
    GtkWidget *menu, *menuitem;
    /* 右ボタンでメニューを開く */
    if (button == 3) {
        /* メニュー作成 */
        menu = gtk_menu_new ();
        menuitem = gtk_menu_item_new_with_label ("⏸ pause");
        g_signal_connect (menuitem, "activate",
            //~ G_CALLBACK (mpvradio_common_cb), mpvradio_common_play);
            G_CALLBACK (mpvradio_common_cb), mpvradio_common_toggle_pause);
        gtk_menu_shell_append (menu, menuitem);
        menuitem = gtk_menu_item_new_with_label ("⏹ stop");
        g_signal_connect (menuitem, "activate",
            G_CALLBACK (mpvradio_common_cb), mpvradio_common_stop);
        gtk_menu_shell_append (menu, menuitem);

        gtk_menu_shell_append (menu, gtk_separator_menu_item_new ());

        gpointer url;
        GList *curr = g_list_first (playlist_sorted);
        while (curr != NULL) {
            if (curr->data != NULL) {
                g_print ("station : %s\n", curr->data);
                url = g_hash_table_lookup (playlist_table, curr->data);
                menuitem = gtk_menu_item_new_with_label (curr->data);
                g_signal_connect (menuitem, "activate",
                    G_CALLBACK (_mpvradio_statusicon_menuitem_cb), NULL);
                gtk_menu_shell_append (menu, menuitem);
            }
            curr = g_list_next (curr);
        }

        gtk_menu_shell_append (menu, gtk_separator_menu_item_new ());

        menuitem = gtk_menu_item_new_with_label ("🚪 quit");
        g_signal_connect (menuitem, "activate",
            G_CALLBACK (menu_quit_cb), app);
        gtk_menu_shell_append (menu, menuitem);
        //~ gtk_menu_shell_prepend (menu, menuitem);

        gtk_widget_show_all (menu);
        xapp_status_icon_popup_menu (icon, menu, x, y, button, time, panel_position);
    }
}



XAppStatusIcon *mpvradio_statusicon_new (GtkApplication *app)
{
    XAppStatusIcon *appindicator = xapp_status_icon_new ();
    xapp_status_icon_set_icon_name (appindicator, mpvradioICON);
    g_signal_connect (appindicator, "activate",
                    G_CALLBACK (mpvradio_statusicon_activate_cb), app);
    g_signal_connect (appindicator, "scroll-event",
                    G_CALLBACK (mpvradio_statusicon_scroll_event_cb), app);
    g_signal_connect (appindicator, "button-release-event",
                    G_CALLBACK (mpvradio_statusicon_button_release_event_cb), app);
    return appindicator;
}
