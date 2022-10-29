bin_PROGRAMS = mpvradio

AM_CPPFLAGS = -DICONDIR=\"$(datadir)/pixmaps\" -DLOCALEDIR=\"$(localedir)\" -DGETTEXT_PACKAGE=\""$(GETTEXT_PACKAGE)"\"

mpvradio_SOURCES = \
	mpvradio-stationbutton.c	\
	mpvradio-stationbutton.h	\
	mpvradio-statusicon.c	\
	mpvradio-statusicon.h	\
	mpvradio-adduri.c	\
	mpvradio-config.c	\
	mpvradio-notify.c	\
	mpvradio.h			\
	mpvradio-main.c	\
	mpvradio-common.c	\
	mpvradio-ipc.c	\
	mpvradio-ipc.h


mpvradio_CFLAGS  =  $(GTK_CFLAGS) $(NOTIFY_CFLAGS) $(XAPP_CFLAGS)
mpvradio_LDADD   =  $(INTLLIBS) $(GTK_LIBS) $(NOTIFY_LIBS) $(XAPP_LIBS)
