import requests
from html.parser import HTMLParser

class TestParser(HTMLParser):
    def __init__(self):
        self.id = ''
        self.rawdata =''

    def handle_starttag(self, tag, attrs):
        self.id = tag

    def handle_endtag(self, tag):
        self.id = ''

    def handle_data(self, data):
        if self.id == 'ip':
            print(data)

url = 'https://playerservices.streamtheworld.com/api/livestream?station=AFN_JOEP&transports=http,hls&version=1.8'

r = requests.get(url)
parser = TestParser()
r.encoding = 'utf-8'
parser.feed(r.text)
