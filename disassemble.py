#!/usr/bin/python3.9

# Simple script to disassemble and dump .pyc files.

import dis
import marshal

with open("input.cpython-39.pyc", "rb") as ifile:
	content = ifile.read()

data = marshal.loads(content[16:])
dis.dis(data)
