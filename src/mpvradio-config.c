/*
 * mpvradio_config.c
 *
 * Copyright 2019 sakai <endeavor2wako@gmail.com>
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

/* mpvradio_adduri.c */
extern void mpvradio_adduri_quicktune (GtkWidget *);


void detach_config_file (GKeyFile *kf)
{
    g_key_file_unref (kf);
}

void save_config_file (GKeyFile *kf, struct serverinfo *sv)
{
    gchar *conf_file_name;
    gchar *filename, *dir, *sel;

    dir = g_strdup_printf ("%s/%s", g_get_user_config_dir (), PACKAGE);
    conf_file_name = g_strdup_printf ("%s/%s.conf", dir, PACKAGE);
    g_message ("sv->id %s", sv->id);
    g_key_file_set_string (kf, PACKAGE, "selectserver", sv->id);

    if (g_key_file_save_to_file (kf, conf_file_name, NULL) == FALSE) {
        /* 設定ファイルの保存に失敗しました。*/
        g_warning (N_("Failed to save the configuration file."));
    }
    g_free (conf_file_name);
    g_free (dir);
}

GKeyFile *load_config_file (struct serverinfo *sv)
{
    gchar *conf_file_name;
    gchar *filename, *dir, *sel;
    GError *err = NULL;
    GKeyFile *kf;

    kf = g_key_file_new ();
    dir = g_strdup_printf ("%s/%s", g_get_user_config_dir (), PACKAGE);
    conf_file_name = g_strdup_printf ("%s/%s.conf", dir, PACKAGE);
    //~ printf ("conf file %s\n", conf_file_name);
    if (g_key_file_load_from_file(kf, conf_file_name,
            G_KEY_FILE_NONE, NULL) == FALSE) {
        g_key_file_load_from_data (
            kf,
            "[" PACKAGE "]\n" \
            "version=" PACKAGE_VERSION "\n" \
            "selectserver=1" \
            "\n"    \
            "[server1]\n"   \
            "name=localhost\n" \
            "ip4=127.0.0.1\n" \
            "port=6600\n" \
            "ua=false\n"    \
            "password=\n"
            "music_directory=\n"\
            "\n",   -1,
            G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS,
            NULL);
        // 設定ファイルが見つからなかったので取り敢えず既定値の保存を試みる
        g_mkdir (dir, S_IRWXU);
        if (g_key_file_save_to_file (kf, conf_file_name, &err) == FALSE) {
            g_warning (err->message);
            g_clear_error (&err);
        }
    }

    sv->id   = g_strdup (g_key_file_get_value (kf, PACKAGE, "selectserver", NULL));
    sel      = g_strdup_printf ("server%s", sv->id);
    sv->name = g_key_file_get_value (kf, sel, "name", NULL);
    sv->ip4  = g_key_file_get_value (kf, sel, "ip4", NULL);
    sv->port = atoi (g_key_file_get_value (kf, sel, "port", NULL));
    sv->ua = g_key_file_get_boolean (kf, sel, "ua", &err);
    if (err != NULL) {
        g_warning (err->message);
        g_clear_error (&err);
    }
    sv->password = g_key_file_get_value (kf, sel, "password", NULL);
    sv->music_directory = g_key_file_get_value (kf, sel, "music_directory", NULL);

    g_message ("configure loaded.sv->id %s", sv->id);
    g_free (sel);
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


static GtkWidget *mpvradio_config_page_ui_new (
                gchar *name,
                gchar *host,
                gchar *port,
                gboolean ua,
                gchar *password,
                gchar *music_directory)
{
    GtkWidget *grid, *entry, *sw, *password_entry;
    GtkBuilder *builder;

    builder = gtk_builder_new_from_string (
    "<interface>"
"<object class=\"GtkGrid\" id=\"grid\">"
"        <property name=\"visible\">True</property>"
"        <property name=\"can_focus\">False</property>"
"        <property name=\"row_spacing\">6</property>"
"        <property name=\"column_spacing\">5</property>"
"        <property name=\"column_homogeneous\">True</property>"
"        <property name=\"margin_left\">30</property>"
"        <property name=\"margin_right\">30</property>"
"        <property name=\"margin_top\">20</property>"
"        <property name=\"margin_bottom\">20</property>"
"        <child>"
"          <object class=\"GtkLabel\" id=\"namelabel\">"
"            <property name=\"visible\">True</property>"
"            <property name=\"can_focus\">False</property>"
"            <property name=\"halign\">start</property>"
"            <property name=\"label\" translatable=\"yes\">name</property>"
"          </object>"
"          <packing>"
"            <property name=\"left_attach\">0</property>"
"            <property name=\"top_attach\">0</property>"
"          </packing>"
"        </child>"
"        <child>"
"          <object class=\"GtkLabel\" id=\"hostlabel\">"
"            <property name=\"visible\">True</property>"
"            <property name=\"can_focus\">False</property>"
"            <property name=\"halign\">start</property>"
"            <property name=\"label\" translatable=\"yes\">host</property>"
"          </object>"
"          <packing>"
"            <property name=\"left_attach\">0</property>"
"            <property name=\"top_attach\">1</property>"
"          </packing>"
"        </child>"
"        <child>"
"          <object class=\"GtkLabel\" id=\"portlabel\">"
"            <property name=\"visible\">True</property>"
"            <property name=\"can_focus\">False</property>"
"            <property name=\"halign\">start</property>"
"            <property name=\"label\" translatable=\"yes\">port</property>"
"          </object>"
"          <packing>"
"            <property name=\"left_attach\">0</property>"
"            <property name=\"top_attach\">2</property>"
"          </packing>"
"        </child>"
"        <child>"
"          <object class=\"GtkLabel\" id=\"UAlabel\">"
"            <property name=\"visible\">True</property>"
"            <property name=\"can_focus\">False</property>"
"            <property name=\"halign\">start</property>"
"            <property name=\"label\" translatable=\"yes\">Use Authentication</property>"
"          </object>"
"          <packing>"
"            <property name=\"left_attach\">0</property>"
"            <property name=\"top_attach\">3</property>"
"          </packing>"
"        </child>"
"        <child>"
"          <object class=\"GtkLabel\" id=\"passwordlabel\">"
"            <property name=\"visible\">True</property>"
"            <property name=\"can_focus\">False</property>"
"            <property name=\"halign\">start</property>"
"            <property name=\"label\" translatable=\"yes\">password</property>"
"          </object>"
"          <packing>"
"            <property name=\"left_attach\">0</property>"
"            <property name=\"top_attach\">4</property>"
"          </packing>"
"        </child>"
"        <child>"
"          <object class=\"GtkLabel\" id=\"mdlabel\">"
"            <property name=\"visible\">True</property>"
"            <property name=\"can_focus\">False</property>"
"            <property name=\"halign\">start</property>"
"            <property name=\"label\" translatable=\"yes\">Music Directory</property>"
"          </object>"
"          <packing>"
"            <property name=\"left_attach\">0</property>"
"            <property name=\"top_attach\">5</property>"
"          </packing>"
"        </child>"
"        <child>"
"          <object class=\"GtkSwitch\" id=\"UA_sw\">"
"            <property name=\"visible\">True</property>"
"            <property name=\"can_focus\">True</property>"
"            <property name=\"halign\">end</property>"
//~ "            <property name=\"action_name\">ua_sw</property>"
//~ "            <signal name=\"activate\" handler=\"UA_sw_activate_cb\" swapped=\"no\"/>"
"          </object>"
"          <packing>"
"            <property name=\"left_attach\">1</property>"
"            <property name=\"top_attach\">3</property>"
"          </packing>"
"        </child>"
"        <child>"
"          <object class=\"GtkEntry\" id=\"name_ent\">"
"            <property name=\"visible\">True</property>"
"            <property name=\"can_focus\">True</property>"
"            <property name=\"halign\">end</property>"
"          </object>"
"          <packing>"
"            <property name=\"left_attach\">1</property>"
"            <property name=\"top_attach\">0</property>"
"          </packing>"
"        </child>"
"        <child>"
"          <object class=\"GtkEntry\" id=\"host_ent\">"
"            <property name=\"visible\">True</property>"
"            <property name=\"can_focus\">True</property>"
"            <property name=\"halign\">end</property>"
"            <property name=\"input_purpose\">url</property>"
"          </object>"
"          <packing>"
"            <property name=\"left_attach\">1</property>"
"            <property name=\"top_attach\">1</property>"
"          </packing>"
"        </child>"
"        <child>"
"          <object class=\"GtkEntry\" id=\"port_ent\">"
"            <property name=\"visible\">True</property>"
"            <property name=\"can_focus\">True</property>"
"            <property name=\"halign\">end</property>"
"            <property name=\"width_chars\">5</property>"
"            <property name=\"text\" translatable=\"yes\">6600</property>"
"            <property name=\"input_purpose\">digits</property>"
"          </object>"
"          <packing>"
"            <property name=\"left_attach\">1</property>"
"            <property name=\"top_attach\">2</property>"
"          </packing>"
"        </child>"
"        <child>"
"          <object class=\"GtkEntry\" id=\"password_ent\">"
"            <property name=\"visible\">True</property>"
"            <property name=\"can_focus\">True</property>"
"            <property name=\"halign\">end</property>"
"            <property name=\"visibility\">False</property>"
"            <property name=\"invisible_char\">*</property>"
"            <property name=\"input_purpose\">password</property>"
"          </object>"
"          <packing>"
"            <property name=\"left_attach\">1</property>"
"            <property name=\"top_attach\">4</property>"
"          </packing>"
"        </child>"
"        <child>"
"          <object class=\"GtkEntry\" id=\"md_ent\">"
"            <property name=\"visible\">True</property>"
"            <property name=\"can_focus\">True</property>"
"            <property name=\"halign\">end</property>"
"          </object>"
"          <packing>"
"            <property name=\"left_attach\">1</property>"
"            <property name=\"top_attach\">5</property>"
"          </packing>"
"        </child>"
"      </object>"
"</interface>",
    -1);
    entry = GTK_ENTRY(gtk_builder_get_object (builder, "name_ent"));
    gtk_entry_set_text (entry, name);
    g_signal_connect (G_OBJECT(entry), "destroy",
                                G_CALLBACK(grid_destroy_cb), "name");

    entry = GTK_ENTRY(gtk_builder_get_object (builder, "host_ent"));
    gtk_entry_set_text (entry, host);
    g_signal_connect (G_OBJECT(entry), "destroy",
                                G_CALLBACK(grid_destroy_cb), "host");

    entry = GTK_ENTRY(gtk_builder_get_object (builder, "port_ent"));
    gtk_entry_set_text (entry, port);
    g_signal_connect (G_OBJECT(entry), "destroy",
                                G_CALLBACK(grid_destroy_cb), "port");

    password_entry = GTK_ENTRY(gtk_builder_get_object (builder, "password_ent"));
    gtk_entry_set_text (password_entry, password);
    g_signal_connect (G_OBJECT(password_entry), "destroy",
                                G_CALLBACK(grid_destroy_cb), "password");

    sw = GTK_SWITCH(gtk_builder_get_object (builder, "UA_sw"));
    gtk_switch_set_active (sw, TRUE);
    gtk_switch_set_state (sw, FALSE);
    g_signal_connect (G_OBJECT(sw), "destroy",
                                G_CALLBACK(grid_destroy_cb), "ua_sw");
    g_signal_connect (G_OBJECT(sw), "notify::active",
                                G_CALLBACK(uasw_activate_cb), password_entry);

    //~ g_signal_connect (G_OBJECT(password_entry), "event",
                                //~ G_CALLBACK(password_focus_cb), sw);

    entry = GTK_ENTRY(gtk_builder_get_object (builder, "md_ent"));
    gtk_entry_set_text (entry, music_directory);
    g_signal_connect (G_OBJECT(entry), "destroy",
                                G_CALLBACK(grid_destroy_cb), "music directory");



    grid =  GTK_GRID(gtk_builder_get_object (builder, "grid"));
    g_signal_connect (G_OBJECT(grid), "destroy",
                                G_CALLBACK(grid_destroy_cb), "grid");

    return grid;

}
/*
 *      name :
 *      host :
 *      port : 6600
 *      Use Authentication: (sw)
 *      password:
 *      Music Directory:
 */
XAppPreferencesWindow *mpvradio_config_prefernces_ui (void)
{
    XAppPreferencesWindow *win;
    GtkWidget *tv, *newbtn, *connectbtn, *disconnectbtn, *notebook;
    GKeyFile *kf;
    struct serverinfo sv;
    int i;
    gchar *sel, *name, *host, *port, *password, *music_directory;
    gboolean ua;

    win = xapp_preferences_window_new ();
    newbtn = gtk_button_new_with_label ("new");
    connectbtn = gtk_button_new_with_label ("connect");
    disconnectbtn = gtk_button_new_with_label ("disconnect");
    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (notebook, GTK_POS_LEFT);
    gtk_notebook_set_scrollable (notebook, TRUE);
    //~ gtk_container_add (GTK_CONTAINER(win), notebook);
    xapp_preferences_window_add_button (win, newbtn, GTK_PACK_START);
    xapp_preferences_window_add_button (win, connectbtn, GTK_PACK_END);
    xapp_preferences_window_add_button (win, disconnectbtn, GTK_PACK_END);

    kf = load_config_file (&sv);
    i = 1;
    sel = g_strdup_printf ("server%d",i);
    while (g_key_file_has_group (kf, sel)) {
        name            = g_key_file_get_value (kf, sel, "name", NULL);
        host            = g_key_file_get_value (kf, sel, "ip4", NULL);
        port            = g_key_file_get_value (kf, sel, "port", NULL);
        ua              = g_key_file_get_boolean (kf, sel, "ua", NULL);
        password        = g_key_file_get_value (kf, sel, "password", NULL);
        music_directory = g_key_file_get_value (kf, sel, "music_directory", NULL);

        tv = mpvradio_config_page_ui_new (name, host, port, ua, password, music_directory);
        //~ xapp_preferences_window_add_page (win, tv, sel, name);
        gtk_notebook_append_page (notebook, tv, gtk_label_new (name));


        g_free (sel);
        i++;
        sel = g_strdup_printf ("server%d",i);
    }
    g_free (sel);
    xapp_preferences_window_add_page (win, notebook, "note", "note");


    g_key_file_unref (kf);
    return win;
}
