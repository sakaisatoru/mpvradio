/*
 * mpvradio-ipc.c
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
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <glib.h>
#include <locale.h>

#include "glib/gi18n.h"
#include "gtk/gtk.h"

#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

#define MPV_SOCKET_PATH "/run/user/1000/mpvsocket"
#define MPVOPTION1      "--idle"
#define MPVOPTION2      "--input-ipc-server="MPV_SOCKET_PATH
#define MPVOPTION3      "--no-video"

#define READ_BUFFER_SIZE    1024


static pid_t current_mpv = 0;

void mpvradio_ipc_kill_mpv (void)
{
    if (current_mpv) {
        g_message ("kill %d", current_mpv);
        kill (current_mpv, SIGTERM);
    }
}

void mpvradio_ipc_fork_mpv (void)
{
    pid_t pid, pid2;

    int status;
    int pfd[2];

    if (pipe(pfd) == -1) {
        g_message ("パイプの作成に失敗");
        return;
    }

    pid = fork ();
    if (pid == -1) {
        // 失敗
        g_message ("子プロセスの起動に失敗");
    } else if (pid == 0) {
        // 子プロセス
        pid2 = fork ();
        if (pid2 == -1) {
            // 失敗
            g_message ("孫プロセスの起動に失敗");
        }
        else if (pid2 == 0) {
            // 孫プロセス
            // 呼び出し元のこのスレッド以外は引き継がない事に注意。
            //~ xapp_status_icon_set_label (appindicator, gtk_button_get_label (btn));
            //~ mpvradio_stop_mpv ();
            execlp ("mpv", "mpv", MPVOPTION1, MPVOPTION2, MPVOPTION3, (char*)NULL);
        }
        else {
            // 子プロセスは親プロセスにpipeで pidを返してすぐに終了、
            // 孫プロセスは孤児となる
            close (pfd[0]);
            write (pfd[1], &pid2, sizeof(pid2));
            close (pfd[1]);
            g_message ( "pid send : %d", pid2);
            exit (0);
        }
    }
    else {
        // 親プロセス
        close (pfd[1]);
        read (pfd[0], &current_mpv, sizeof(current_mpv));
        close (pfd[0]);
        waitpid (pid, &status, 0);
        g_message ("pid recv : %d", current_mpv);
    }
    sleep (1);  // 直後のsocket通信でセグるので安定のために時間稼ぎする。
}

/*
 * mpv に message を送ってレスポンスを返す。
 * レスポンスは使用後に g_free で必ず開放する事。
 * 何らかのエラーが発生した際は NULL を返す。
 */
char *mpvradio_ipc_send_and_recv (char *message)
{
    int ret_code = 0;
    uint32_t message_len = 0;
    char *retstr = NULL;

    int fd = -1;
    ssize_t size = 0;
    char readbuffer[READ_BUFFER_SIZE];

    // ソケットアドレス構造体
    struct sockaddr_un sun;
    memset (&sun, 0, sizeof(sun));

    // UNIXドメインのソケットを作成
    fd = socket (AF_LOCAL, SOCK_STREAM, 0);
    if (fd == -1) {
        g_message ("failed to create_socket(errno=%d:%s)\n", errno, strerror (errno));
        return NULL;
    }

    // ソケットアドレス構造体に接続先(サーバー)を設定
    sun.sun_family = AF_LOCAL;
    strcpy(sun.sun_path, MPV_SOCKET_PATH);

    // サーバーに接続
    ret_code = connect(fd, (const struct sockaddr *)&sun, sizeof(sun));
    if (ret_code == -1) {
        g_message ("failed to connect_socket(errno:%d, error_str:%s)\n", errno, strerror(errno));
        close (fd);
        return NULL;
    }

    // 送信
    message_len = strlen (message);
    size = write (fd, message, message_len);
    if (size < message_len) {
        g_message ("failed to send data(errno:%d, error_str:%s)\n", errno, strerror(errno));
        close (fd);
        return NULL;
    }
    //~ g_message (message);

    // レスポンスの受信
    for (;;) {
        size = read (fd, readbuffer, READ_BUFFER_SIZE-1);
        //~ g_message (readbuffer);
        if (size < READ_BUFFER_SIZE-1) {
            readbuffer[size] = '\0';
            break;
        }
    }
    retstr = g_strdup (readbuffer);

    // ソケットを閉じる
    close (fd);
    return retstr;
}

int mpvradio_ipc_send (char *message)
{
    char *s;
    int retval = 0;

    s = mpvradio_ipc_send_and_recv (message);

    JsonParser *parser = json_parser_new ();
    json_parser_load_from_data (parser, s, -1, NULL);

    JsonReader *reader = json_reader_new (json_parser_get_root (parser));

    //~ json_reader_read_member (reader, "request_id");
    //~ int mpv_request_id = json_reader_get_int_value (reader);
    //~ g_print ("request_id : %d ", mpv_request_id);
    //~ json_reader_end_member (reader);

    json_reader_read_member (reader, "error");
    const char *mpv_error = json_reader_get_string_value (reader);
    //~ g_print ("error : %s ",mpv_error);
    json_reader_end_member (reader);

    retval = !strcmp(mpv_error, "success")? 0 : -1;

    //~ json_reader_read_member (reader, "data");
    //~ gboolean mpv_data = json_reader_get_boolean_value (reader);
    //~ g_print ("data : %s(%d)", mpv_data?"true":"false",mpv_data);
    //~ json_reader_end_member (reader);


    //~ json_reader_read_member (reader, "size");
    //~ json_reader_read_element (reader, 0);
    //~ int width = json_reader_get_int_value (reader);
    //~ json_reader_end_element (reader);
    //~ json_reader_read_element (reader, 1);
    //~ int height = json_reader_get_int_value (reader);
    //~ json_reader_end_element (reader);
    //~ json_reader_end_member (reader);

    g_object_unref (reader);
    g_object_unref (parser);


    g_free (s);

    return retval;
}


