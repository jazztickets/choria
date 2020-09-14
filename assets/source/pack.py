#!/usr/bin/env python3

import os
import sys
import struct
import shutil
import argparse

def check_dir(string):
	if os.path.isdir(string):
		return string
	else:
		raise argparse.ArgumentTypeError(f"{string} is not a valid path")

parser = argparse.ArgumentParser()
parser.add_argument("base_path", help="base input directory", type=check_dir)
parser.add_argument("sub_path", help="sub path", type=check_dir)
parser.add_argument("-d", "--debug", action='store_true', help="show header")
arguments = parser.parse_args()

base_path = arguments.base_path
sub_path = arguments.sub_path
full_path = os.path.join(base_path, sub_path)
basedirname = os.path.basename(sub_path.strip(os.sep))

# get list of files
files = os.listdir(full_path)
files.sort()
file_list = []
for filename in files:
	file_path = full_path + os.path.sep + filename
	if os.path.isfile(file_path):
		final_path = os.path.join(sub_path, filename)
		file_list.append(final_path)

# generate header file
header_file = open("header", "wb")
header_file.write(struct.pack('i', len(file_list)))
if arguments.debug:
	print(len(file_list))

# generate data
data_file = open("body", "wb")
for final_path in file_list:

	# load file into memory
	file = open(os.path.join(base_path, final_path), "rb")
	data = file.read()
	file.close()

	# write data
	data_file.write(data)

	# get sizes
	size = len(data)
	filename_size = len(final_path)

	# print header
	if arguments.debug:
		print(str(filename_size) + "," + final_path + "," + str(size))

	# write header
	header_file.write(struct.pack('b', filename_size))
	header_file.write(final_path.encode("ascii"))
	header_file.write(struct.pack('i', size))

header_file.close()
data_file.close()

# combine header and body
with open('final', 'wb') as write_file:
	for file in ['header', 'body']:
		with open(file, 'rb') as in_file:
			shutil.copyfileobj(in_file, write_file)

		os.remove(file)

os.rename("final", basedirname + ".bin")
