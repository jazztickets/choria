#!/usr/bin/env python3
import cgi
import html
import http.server
import io
import json
import mimetypes
import os
import posixpath
import shutil
import socketserver
import sys
import urllib
import sqlite3
from http import HTTPStatus

db = sqlite3.connect('../../working/stats/stats.db')
sql = db.cursor()

class HttpHandler(http.server.BaseHTTPRequestHandler):

	def do_POST(self):

		# get post data
		length = self.headers['content-length']
		data = self.rfile.read(int(length))
		
		# parse url
		parts = urllib.parse.urlsplit(self.path)
		print(parts)
		if parts.path == "/save":
			parsed = urllib.parse.parse_qs(data)
			for row in parsed:
				for col in parsed[row]:
					print(col.decode('utf-8'))

	def do_GET(self):
		self.handle_request(False)

	def do_HEAD(self):
		self.handle_request(True)

	def handle_request(self, head_only):
		path = self.translate_path(self.path)
		f = None
		if os.path.isdir(path):
			parts = urllib.parse.urlsplit(self.path)
			if not parts.path.endswith('/'):
				# redirect browser - doing basically what apache does
				self.send_response(HTTPStatus.MOVED_PERMANENTLY)
				new_parts = (parts[0], parts[1], parts[2] + '/',
							 parts[3], parts[4])
				new_url = urllib.parse.urlunsplit(new_parts)
				self.send_header("Location", new_url)
				self.end_headers()
				return
			for index in "index.html", "index.htm":
				index = os.path.join(path, index)
				if os.path.exists(index):
					path = index
					break

		if self.request_handler(path):
			return

		ctype = self.guess_type(path)
		try:
			f = open(path, 'rb')
		except OSError:
			self.send_error(HTTPStatus.NOT_FOUND, "File not found")
			return
		try:
			self.send_response(HTTPStatus.OK)
			self.send_header("Content-type", ctype)
			fs = os.fstat(f.fileno())
			self.send_header("Content-Length", str(fs[6]))
			self.send_header("Last-Modified", self.date_time_string(fs.st_mtime))
			self.end_headers()
			if not head_only:
				shutil.copyfileobj(f, self.wfile)
			f.close()
		except:
			f.close()
			raise

	def request_handler(self, path):
		parts = urllib.parse.urlsplit(self.path)
		query = urllib.parse.parse_qs(parts.query)
		if parts.path == "/data":
			tablename = query['table']
			query = sql.execute("select * from item");
			results = query.fetchall()
			json_string = json.dumps(results)
			self.send_response(HTTPStatus.OK)
			self.send_header("Content-type", "application/json")
			self.send_header("Content-Length", len(json_string))
			self.end_headers()
			self.wfile.write(str.encode(json_string))
			return True

		return False

	def translate_path(self, path):
		# abandon query parameters
		path = path.split('?',1)[0]
		path = path.split('#',1)[0]
		# Don't forget explicit trailing slash when normalizing. Issue17324
		trailing_slash = path.rstrip().endswith('/')
		try:
			path = urllib.parse.unquote(path, errors='surrogatepass')
		except UnicodeDecodeError:
			path = urllib.parse.unquote(path)
		path = posixpath.normpath(path)
		words = path.split('/')
		words = filter(None, words)
		path = os.getcwd()
		for word in words:
			drive, word = os.path.splitdrive(word)
			head, word = os.path.split(word)
			if word in (os.curdir, os.pardir): continue
			path = os.path.join(path, word)
		if trailing_slash:
			path += '/'
		return path

	def guess_type(self, path):
		base, ext = posixpath.splitext(path)
		if ext in self.extensions_map:
			return self.extensions_map[ext]
		ext = ext.lower()
		if ext in self.extensions_map:
			return self.extensions_map[ext]
		else:
			return self.extensions_map['']

	if not mimetypes.inited:
		mimetypes.init()

	extensions_map = mimetypes.types_map.copy()
	extensions_map.update({
		'': 'application/octet-stream',
		})

socketserver.TCPServer.allow_reuse_address = True
httpd = socketserver.TCPServer(("", 8000), HttpHandler)

print("Starting http://localhost:8000")

try:
	httpd.serve_forever()
except KeyboardInterrupt:
	sys.exit(0)
