lgi = require 'lgi'
socket = require("posix.sys.socket")
unistd = require("posix.unistd")

iconv = require("iconv")
utf8toascii = iconv.new("ASCII//TRANSLIT","utf-8")

Notify = lgi.require('Notify')
Notify.init("mpv")
TitleInfo=Notify.Notification.new("mpv","","mpv")

function send_message_mpvradio(message)
    local fd = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM, 0)
    socket.connect(fd, {family = socket.AF_UNIX, path = "/run/user/1000/mpvradio"})
    socket.send(fd, message)
    unistd.close(fd)
end

function on_file_loaded(event)
        TitleInfo:update("演奏開始","","mpv")
        TitleInfo:show()
end
--~ mp.register_event("file-loaded", on_file_loaded)

function on_end_file(event)
        s = event.reason
        if s == "error" then
            s = event.error
        end
        TitleInfo:update("演奏終了",s,"mpv")
        TitleInfo:show()
end
--~ mp.register_event("end-file", on_end_file)

function on_pause_change(name, value)
    if value == true then
        mp.set_property("fullscreen", "no")
    end
end
mp.observe_property("pause", "bool", on_pause_change)

function on_media_title_change(name, value)
    if value ~= nil then
        --~ print(name)
        --~ print(value)

        --~ metadataの情報を得る
        a = mp.get_property_native("metadata")
        fi = {ARTIST="",ALBUM="",TRACK="",TITLE=""}
        if a ~= nil then
            i,v = next(a,nil)
            repeat
                if fi[string.upper(i)] ~= nil then
                    fi[string.upper(i)] = v
                end
              --~ print("index=",i,"  value=",v)
              i,v = next(a,i)
            until i ==nil
            --~ print(fi.TRACK," ",fi.TITLE," ",fi.ALBUM," ",fi.ARTIST)
        end
        title = fi.TITLE
        if title == "" or title == nil then
            title = value
        end
        if fi.TRACK ~= nil then
            title = string.format("%s  %s", fi.TRACK, title)
        else
            fi.TRACK = ""
        end
        if fi.ALBUM == nil then
            fi.ALBUM = ""
        end
        if fi.ARTIST == nil then
            fi.ARTIST = ""
        end

        --~ title2,err = utf8toascii:iconv(title)
        --~ artist2,err = utf8toascii:iconv(fi.ARTIST)
        --~ TitleInfo:update(title2,artist2,"mpv")
        TitleInfo:update(title,fi.ARTIST,"mpv")
        TitleInfo:show()

        if fi.ALBUM ~= "" then
            fi.ALBUM = string.format("(album) %s", fi.ALBUM)
        end
        if fi.ARTIST ~= "" then
            fi.ARTIST = string.format("(artist) %s", fi.ARTIST)
        end
        --~ s = string.format("echo -n '%s   %s   %s' | socat - /run/user/1000/mpvradio",
            --~ title,fi.ALBUM,fi.ARTIST)
        s = string.format("%s   %s   %s",title, fi.ALBUM, fi.ARTIST)
        send_message_mpvradio(s)
        --~ os.execute(s)
        --~ print(s)
    end
end
mp.observe_property("media-title", "string", on_media_title_change)


