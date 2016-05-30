#!/usr/bin/python

import SimpleHTTPServer, BaseHTTPServer
import socket, urllib2, string

HTTP_PORT = 8000

forward_prefix = None

_httpd = None

class MyRequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):

    # Disable logging DNS lookups
    def address_string(self):
        return str(self.client_address[0])

    def do_GET(self):
        global forward_prefix

        if self.path.startswith("/wget/"):
            addr = self.path[6:]
            i = string.find(addr, "/")
            path = "/"
            if i > 0:
                path = addr[i:]
                addr = addr[0:i]

            if not forward_prefix or not addr.startswith(forward_prefix):
                self.send_error(403)
                return

            url = "http://[" + addr + "]" + path
            print "httpd: forwarding to", url
            data = urllib2.urlopen(url).read()
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            self.wfile.write(data);
            self.wfile.flush()
            return

        self.path = "/www" + self.path
        print "httpd: Path: ", self.path
        return SimpleHTTPServer.SimpleHTTPRequestHandler.do_GET(self)

def init():
    global _httpd

    _httpd = BaseHTTPServer.HTTPServer(("", HTTP_PORT), MyRequestHandler)
    print "httpd: serving at port", HTTP_PORT

def set_forward_prefix(prefix):
    global forward_prefix
    forward_prefix = prefix
    print "httpd: forward prefix set to \"" + prefix + "\""

def serve_forever():
    global _httpd
    if not _httpd:
        init()
    _httpd.serve_forever()

if __name__ == "__main__":
    # Started standalone
    init()
    serve_forever()
