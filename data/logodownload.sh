#! /bin/sh
target=$HOME/.cache/mpvradio/logo
mkdir -p $target
cd $target

wget --output-document="radioparadaise.com(main mix)".png https://radioparadise.com/rpassets/images/logo/logo_2022_nav_244x64.svg
cp "radioparadaise.com(main mix)".png "radioparadaise.com(mellow mix)".png
cp "radioparadaise.com(main mix)".png "radioparadaise.com(rock mix)".png
cp "radioparadaise.com(main mix)".png "radioparadaise.com(global mix)".png
wget --output-document="スマイルラジオ.png" https://775fm.co.jp/wp/wp-content/themes/775fm/images/logo.png

wget --output-document="AFN TOKYO".png https://pacific.afn.mil/portals/101/Images/AFNgo_tile.png
cp "AFN TOKYO.png" "AFN Gravity.png"
cp "AFN TOKYO.png" "AFN Joe Radio(Pacific).png"
cp "AFN TOKYO.png" "AFN Joe Radio.png"
cp "AFN TOKYO.png" "AFN Hot AC(Pacific).png"
cp "AFN TOKYO.png" "AFN Hot AC.png"
cp "AFN TOKYO.png" "AFN Legacy.png"
cp "AFN TOKYO.png" "AFN Freedom.png"
cp "AFN TOKYO.png" "AFN Freedom(Pacific).png"

wget --output-document=NHKラジオ第２.svg https://www.nhk.or.jp/radio/assets/img/badge_r2.svg

for i in `cat /usr/local/share/mpvradio/playlists/radio.m3u|grep "plugin"|cut -d"/" -f3`;do wget --output-document=$i.png "https://radiko.jp/station/logo/$i/logo_large.png";done
