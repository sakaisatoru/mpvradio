/*
 * mpvradio_adduri.c
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
#include <glib.h>

#include "mpvradio-ipc.h"
#include "mpvradio.h"

gchar *mpvradio_adduri_quicktune (GtkWindow *oya);
void mpvradio_adduridialog_destroy (void);

static GtkWidget *dialog = NULL;


/*
 * URI追加用のダイアログを開く
 * 呼び出し元がstatusiconなので、親ウィンドウを渡さない。このため、mpvradioを閉じても
 * 起動中のダイアログは残される。
 */
static gboolean key_press_event_cb (GtkWidget *widget,
                            GdkEventKey *event, GtkDialog *dialog)
{
    /* entry を enterキーで抜ける */
    if (event->keyval == GDK_KEY_Return){
        gtk_dialog_response(dialog, GTK_RESPONSE_ACCEPT);
        return TRUE;
    }
    return FALSE;
}

static void filechooser_file_set_cb (GtkFileChooserButton *widget,
               gpointer              user_data)
{
    g_message (gtk_label_get_label (widget));
}


gchar *mpvradio_adduri_quicktune (GtkWindow *oya)
{
    GtkWidget *label, *content_area, *entry, *filechooser, *hbox;
    struct mpd_connection *cn;
    gchar *str;

    dialog = gtk_dialog_new_with_buttons (N_("Add URI"),
                        oya,   /* parent window */
                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_USE_HEADER_BAR,
                        GTK_STOCK_CANCEL,   GTK_RESPONSE_CANCEL,
                        GTK_STOCK_ADD,      GTK_RESPONSE_ACCEPT,
                        NULL);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG(dialog));
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
    label   = gtk_label_new (N_("URI:"));
    entry   = gtk_entry_new ();
    //~ filechooser = gtk_file_chooser_button_new (N_("FILE SELECT"),
                                        //~ GTK_FILE_CHOOSER_ACTION_OPEN);

    g_signal_connect (G_OBJECT(filechooser), "file-set",
        G_CALLBACK(filechooser_file_set_cb), entry);
    g_signal_connect (G_OBJECT(entry), "key-press-event",
        G_CALLBACK(key_press_event_cb), dialog);

    gtk_misc_set_alignment (label, 0, 0.5);   // プロンプトを左寄せ
    gtk_box_pack_start (hbox, entry, TRUE, TRUE, 0);
    //~ gtk_box_pack_start (hbox, filechooser, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX(content_area), label, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX(content_area), hbox, TRUE, TRUE, 0);

    gtk_widget_show_all (dialog);

    switch (gtk_dialog_run(GTK_DIALOG(dialog))){
        case GTK_RESPONSE_ACCEPT:
            str = g_strdup (gtk_entry_get_text (entry));
            if (strlen (str) != 0) {
                gtk_widget_destroy (dialog);
                dialog = NULL;
                return str;
            }
            else {
                g_warning (_("Null strings are not added."));
            }
            g_free (str);
            break;

        case GTK_RESPONSE_CANCEL:
            break;

        case GTK_RESPONSE_DELETE_EVENT:
            break;

        default:
            break;
    }

    gtk_widget_destroy (dialog);
    dialog = NULL;
    return NULL;
}

/*
 * ダイアログが開いているかどうか調べて、開いていれば閉じる。
 * statusicon内から作成されるため親ウィンドウを持てない。このため、親の死と
 * 同時に閉じることができないので、手動で管理する。
 */
void mpvradio_adduridialog_destroy (void)
{
    if (dialog != NULL) {
        gtk_dialog_response (dialog, GTK_RESPONSE_DELETE_EVENT);
    }
}
