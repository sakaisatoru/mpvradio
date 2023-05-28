/*
 * mpvradio-ipc.h
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

#ifndef mpvradic_ipc_H
#define mpvradic_ipc_H

extern void mpvradio_ipc_remove_socket (void);

extern gpointer mpvradio_ipc_recv (gpointer n);
extern char *mpvradio_ipc_send_and_response (char *message);
extern int mpvradio_ipc_send (char *message);
extern void mpvradio_ipc_kill_mpv (void);
extern void mpvradio_ipc_fork_mpv (void);

#endif

