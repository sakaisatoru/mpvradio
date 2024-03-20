bin_PROGRAMS = mpvradio

AM_CPPFLAGS = -DDATADIR=\"$(datadir)\" -DICONDIR=\"$(datadir)/pixmaps\" -DLOCALEDIR=\"$(localedir)\" -DGETTEXT_PACKAGE=\""$(GETTEXT_PACKAGE)"\"

mpvradio_SOURCES = \
	mpvradio-statusicon.c	\
	mpvradio-statusicon.h	\
	mpvradio-adduri.c	\
	mpvradio-config.c	\
	mpvradio.h			\
	mpvradio-main.c	\
	mpvradio-common.c	\
	mpvradio-ipc.c	\
	mpvradio-ipc.h	\
	mpvradio-radiopanel.c \
	mpvradio-radiopanel.h	\
	mpvradio-banner.h	\
	mpvradio-banner.c

#	mpvradio-notify.c

mpvradio_CFLAGS  =  $(GTK_CFLAGS) $(JSONGLIB_CFLAGS) $(NOTIFY_CFLAGS) $(XAPP_CFLAGS)
mpvradio_LDADD   =  $(INTLLIBS) $(GTK_LIBS) $(JSONGLIB_LIBS) $(NOTIFY_LIBS) $(XAPP_LIBS)
