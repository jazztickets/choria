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
import re
import shutil
import socketserver
import sys
import urllib
import sqlite3
import subprocess

db_file = '../../working/stats/stats.db'
db = sqlite3.connect(db_file)
db.execute("PRAGMA foreign_keys=ON")
cursor = db.cursor()

def get_table_names():
	query = cursor.execute("SELECT name FROM sqlite_master WHERE type = 'table' ORDER BY name")
	rows = query.fetchall()

	return rows

def get_column_names(tablename):
	query = cursor.execute("PRAGMA table_info(" + tablename + ")")
	rows = query.fetchall()
	names = []
	for row in rows:
		names.append(row[1])

	return names

def get_children(tablename):
	query = cursor.execute("SELECT sql FROM sqlite_master WHERE sql LIKE '%REFERENCES {0}(%'".format(tablename))
	results = query.fetchall()
	children = {}
	for row in results:

		# get related table name
		matches = re.findall('CREATE TABLE "(.*?)"', row[0])
		related_table = matches[0]
		children[related_table] = []

		# get foreign key fields
		matches = re.findall("(.*?) INTEGER.*?REFERENCES {0}\(".format(tablename), row[0])
		for match in matches:
			field = match.strip().replace('"', '')
			children[related_table].append(field)

	return children

def get_references(tablename):
	query = cursor.execute("SELECT sql FROM sqlite_master WHERE name = '{0}'".format(tablename))
	row = query.fetchone()
	references = {}
	if row:
		matches = re.findall('(.*?) INTEGER.*?REFERENCES (.*?) ON', row[0])

		for match in matches:
			field = match[0].strip().replace('"', '')
			id = re.search('(.*?)\((.*?)\)', match[1])
			if id:
				references[field] = [ id.group(1), id.group(2) ]

	return references

class HttpHandler(http.server.BaseHTTPRequestHandler):

	def write_json_response(self, data):
		json_string = json.dumps(data)
		self.send_response(200)
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
		parsed = urllib.parse.parse_qs(data, True)
		params = {}
		for var in query:
			params[var] = query[var][0]
		tablename = '"' + params['table'] + '"'

		if parts.path == "/save":
			columns = get_column_names(tablename)
			id_name = columns[0]
			for row in parsed:
				i = 0
				pairs = []
				id = -1
				for col in parsed[row]:
					escaped = col.decode('utf-8').replace("'", "''")
					if i > 0:
						pairs.append(columns[i] + " = '" + escaped + "'")
					else:
						id = escaped
					i += 1
				update_sql = ', '.join(pairs)
				sql = "UPDATE {0} SET {1} WHERE {2} = ?".format(tablename, update_sql, id_name)
				db.execute(sql, (id, ))

			# commit transaction
			try:
				db.commit()
				dump_file = open("../data/stats.txt", "w")
				subprocess.call(["sqlite3", db_file, ".dump"], stdout=dump_file)
			except sqlite3.Error as e:
				self.write_json_response({'message':sql + ": " + str(e)})
				return

			self.write_json_response({'message':'saved ' + str(datetime.datetime.now())})
			return
		elif parts.path == "/add":

			# get additional query fields
			fields = []
			values = []
			for param in params:
				escaped = params[param].replace('"', '""')
				if param != "table" and param.find('!') == -1:
					fields.append(param)
					values.append("\"" + escaped + "\"")

			# add row
			if len(fields):
				sql = "INSERT INTO {0} ({1}) VALUES({2})".format(tablename, ', '.join(fields), ', '.join(values))
			else:
				sql = "INSERT INTO {0} DEFAULT VALUES".format(tablename)

			db.execute(sql)
			db.commit()

			self.write_json_response({'message':'1 row added'})
			return
		elif parts.path == "/remove":
			columns = get_column_names(tablename)
			id_name = columns[0]

			id = parsed[b'id'][0].decode('utf-8')
			sql = "DELETE FROM {0} WHERE {1} = ?".format(tablename, id_name)
			db.execute(sql, (id,))
			db.commit()

			self.write_json_response({'message':'Row deleted'})
			return

		self.send_error(404, "Not found")

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
				self.send_response(301)
				new_parts = (parts[0], parts[1], parts[2] + '/', parts[3], parts[4])
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
			self.send_error(404, "File not found")
			return
		try:
			self.send_response(200)
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
		params = {}
		for var in query:
			params[var] = query[var][0]

		response = None
		if parts.path == "/data":
			tablename = '"' + params['table'] + '"'

			results = {}
			results['column_names'] = get_column_names(tablename)
			references = get_references(params['table'])
			children = get_children(params['table'])

			# build where query
			pairs = []
			for param in params:
				escaped = params[param].replace('"', '""')
				if param != "table":
					op = " = "
					if param.find('!') != -1:
						op = " <> "

					param = param.replace('!', '')
					pairs.append(param + op + "\"" + escaped + "\"")

			where = ""
			if len(pairs):
				where = 'WHERE ' + ' AND '.join(pairs)

			# get table data
			try:
				sql = "SELECT * FROM {0} {1}".format(tablename, where)
				query = cursor.execute(sql);
			except sqlite3.Error as e:
				self.write_json_response({'message':sql + ": " + str(e)})
				return True

			results['data'] = query.fetchall()

			# add sum row
			empty_row = ['sum']
			empty_row.extend([0] * (len(results['column_names'])-1))
			results['data'].append(empty_row)

			# get references
			for key, value in references.items():
				ref_table = value[0]
				ref_id = value[1]
				try:
					sql = "SELECT {0}, name FROM {1}".format(ref_id, ref_table)
					query = cursor.execute(sql);
				except sqlite3.Error as e:
					self.write_json_response({'message':sql + ": " + str(e)})
					return True

				rows = query.fetchall()
				references[key] = {}
				for row in rows:
					references[key][row[0]] = row[1]

			# get results
			results['references'] = references
			results['children'] = children
			self.write_json_response(results)
			return True
		elif parts.path == "/columns":
			tablename = '"' + params['table'] + '"'
			query = cursor.execute("pragma table_info(" + tablename + ")")
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
			content = self.get_page("edit.html")
			self.write_page(content)
			return True

		return False

	def get_page(self, page):
		with open('layout.html', 'r') as infile:
			layout = infile.read()

		with open(page, 'r') as infile:
			content = infile.read()

		return layout % {'content':content}

	def write_page(self, content):
		self.send_response(200)
		self.send_header("Content-type", "text/html")
		self.send_header("Content-Length", len(content))
		self.end_headers()
		self.wfile.write(str.encode(content))

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

Port = 8000
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
