#!/usr/bin/env python3
import cgi
import datetime
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

def get_table_names():
	query = sql.execute("SELECT name FROM sqlite_master WHERE type = 'table'")
	rows = query.fetchall()

	return rows

def get_column_names(tablename):
	query = sql.execute("PRAGMA table_info(" + tablename + ")")
	rows = query.fetchall()
	names = []
	for row in rows:
		names.append(row[1])

	return names

class HttpHandler(http.server.BaseHTTPRequestHandler):

	def write_json_response(self, data):
		json_string = json.dumps(data)
		self.send_response(HTTPStatus.OK)
		self.send_header("Content-type", "application/json")
		self.send_header("Content-Length", len(json_string))
		self.end_headers()
		self.wfile.write(str.encode(json_string))

	def do_POST(self):

		# get post data
		length = self.headers['content-length']
		data = self.rfile.read(int(length))
		
		# parse url
		parts = urllib.parse.urlsplit(self.path)
		query = urllib.parse.parse_qs(parts.query)
		if parts.path == "/save":
			parsed = urllib.parse.parse_qs(data, True)
			tablename = query['table'][0]
			columns = get_column_names(tablename)
			for row in parsed:
				i = 0
				pairs = []
				id = -1
				for col in parsed[row]:
					escaped = col.decode('utf-8').replace('"', '""')
					if i > 0:
						pairs.append(columns[i] + " = \"" + escaped + "\"")
					else:
						id = escaped
					i += 1
				update_sql = ', '.join(pairs)
				sql = "UPDATE {0} SET {1} WHERE id = {2}".format(tablename, update_sql, id)
				db.execute(sql)
				db.commit()
				self.write_json_response({'message':'saved ' + str(datetime.datetime.now())})
				return

		self.send_error(HTTPStatus.NOT_FOUND, "Not found")

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
		response = None
		if parts.path == "/data":
			tablename = query['table'][0]
			columns = get_column_names(tablename)

			query = sql.execute("select * from {0}".format(tablename));
			results = {}
			results['columns'] = columns
			results['data'] = query.fetchall()
			self.write_json_response(results)
			return True
		elif parts.path == "/columns":
			tablename = query['table']
			query = sql.execute("pragma table_info(" + tablename[0] + ")")
			results = query.fetchall()
			names = []
			for row in results:
				names.append(row[1])

			self.write_json_response(names)
			return True
		elif parts.path == "/tables":
			names = get_table_names()
			self.write_json_response(names)
			return True
		elif parts.path == "/":
			with open('index.html', 'r') as infile:
				data = infile.read()

			self.send_response(HTTPStatus.OK)
			self.send_header("Content-type", "text/html")
			self.send_header("Content-Length", len(data))
			self.end_headers()
			self.wfile.write(str.encode(data))
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

Port = 8080
if len(sys.argv) == 2:
	Port = int(sys.argv[1])

socketserver.TCPServer.allow_reuse_address = True
httpd = socketserver.TCPServer(("", Port), HttpHandler)

print("Starting http://localhost:" + str(Port))

try:
	httpd.serve_forever()
except KeyboardInterrupt:
	db.close()
	sys.exit(0)
