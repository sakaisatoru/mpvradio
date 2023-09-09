desktopdir       = $(datadir)/applications
desktop_in_files = mpvradio.desktop.in
desktop_DATA     = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir       = $(datadir)/pixmaps
pixmaps_DATA     = mpvradio.png

pluginsdir		 = $(datadir)/mpvradio/plugins
plugins_DATA	 = radiko.py

EXTRA_DIST = \
	$(pixmaps_DATA) $(plugins_DATA)

