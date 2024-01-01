#! /bin/sh
target=$HOME/.cache/mpvradio/logo
mkdir -p $target
cd $target

wget --output-document=radioparadaise.png https://radioparadise.com/rpassets/images/logo/logo_2022_nav_244x64.svg
wget --output-document=smile.png https://775fm.co.jp/wp/wp-content/themes/775fm/images/logo.png
wget --output-document=AFN.png https://pacific.afn.mil/portals/101/Images/AFNgo_tile.png
for i in `cat /usr/local/share/mpvradio/playlists/radio.m3u|grep "plugin"|cut -d"/" -f3`;do wget --output-document=$i.png "https://radiko.jp/station/logo/$i/logo_large.png";done
