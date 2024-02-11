desktopdir       = $(datadir)/applications
desktop_in_files = mpvradio.desktop.in
desktop_DATA     = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir       = $(datadir)/pixmaps
pixmaps_DATA     = mpvradio.png

pluginsdir		 = $(datadir)/mpvradio/plugins
plugins_DATA	 = radiko.py afn.sh

playlistsdir	 = $(datadir)/mpvradio/playlists
playlists_DATA	 = radio.m3u

scriptsdir	 	 = $(datadir)/mpvradio/scripts
scripts_DATA	 = mpvradio.lua logodownload.sh

soundsdir		 = $(datadir)/mpvradio/sounds
sounds_DATA	     = button57.mp3 button71.mp3

EXTRA_DIST = \
	$(pixmaps_DATA) $(plugins_DATA) $(playlists_DATA) $(scripts_DATA) $(sounds_DATA)

install-data-hook:
	chmod a+x $(pluginsdir)/*
