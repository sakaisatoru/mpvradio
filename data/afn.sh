#! /bin/sh
ST=$1
echo "{\"command\": [\"loadfile\", \"https://`wget -q -O - "https://playerservices.streamtheworld.com/api/livestream?station=${ST}&transports=http,hls&version=1.8"|grep "<ip>"|head -n 1|sed -e 's/ //g' -e 's/<ip>//' -e 's/<\/ip>//'`/${ST}.mp3\"]}"| socat /run/user/1000/mpvsocket -
