#!/usr/bin/env python3
"""Minimal backend server that identifies itself by port."""

from http.server import BaseHTTPRequestHandler, HTTPServer
import sys

port = int(sys.argv[1]) if len(sys.argv) > 1 else 9001


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/plain")
        self.end_headers()
        self.wfile.write(f"backend {port}\n".encode())

    def log_message(self, format, *args):
        pass  # silence request logs


if __name__ == "__main__":
    HTTPServer(("127.0.0.1", port), Handler).serve_forever()
