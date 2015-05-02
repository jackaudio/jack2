#! /usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2014 (ita)

"""
This module enables automatic handling of network paths of the form \\server\share for both input
and output files. While a typical script may require the following::

	import os
	def build(bld):

		node = bld.root.make_node('\\\\COMPUTER\\share\\test.txt')

		# mark the server/share levels as folders
		k = node.parent
		while k:
			k.cache_isdir = True
			k = k.parent

		# clear the file if removed
		if not os.path.isfile(node.abspath()):
			node.sig = None

		# create the folder structure
		if node.parent.height() > 2:
			node.parent.mkdir()

		# then the task generator
		def myfun(tsk):
			tsk.outputs[0].write("data")
		bld(rule=myfun, source='wscript', target=[nd])

this tool will make the process much easier, for example::

	def configure(conf):
		conf.load('unc') # do not import the module directly

	def build(bld):
		def myfun(tsk):
			tsk.outputs[0].write("data")
		bld(rule=myfun, update_outputs=True,
			source='wscript',
			target='\\\\COMPUTER\\share\\test.txt')
		bld(rule=myfun, update_outputs=True,
			source='\\\\COMPUTER\\share\\test.txt',
			target='\\\\COMPUTER\\share\\test2.txt')
"""

import os
from waflib import Node, Utils, Context

def find_resource(self, lst):
	if isinstance(lst, str):
		lst = [x for x in Node.split_path(lst) if x and x != '.']

	if lst[0].startswith('\\\\'):
		if len(lst) < 3:
			return None
		node = self.ctx.root.make_node(lst[0]).make_node(lst[1])
		node.cache_isdir = True
		node.parent.cache_isdir = True

		ret = node.search_node(lst[2:])
		if not ret:
			ret = node.find_node(lst[2:])
		if ret and os.path.isdir(ret.abspath()):
			return None
		return ret

	return self.find_resource_orig(lst)

def find_or_declare(self, lst):
	if isinstance(lst, str):
		lst = [x for x in Node.split_path(lst) if x and x != '.']

	if lst[0].startswith('\\\\'):
		if len(lst) < 3:
			return None
		node = self.ctx.root.make_node(lst[0]).make_node(lst[1])
		node.cache_isdir = True
		node.parent.cache_isdir = True
		ret = node.find_node(lst[2:])
		if not ret:
			ret = node.make_node(lst[2:])
		if not os.path.isfile(ret.abspath()):
			ret.sig = None
			ret.parent.mkdir()
		return ret

	return self.find_or_declare_orig(lst)

def abspath(self):
	"""For MAX_PATH limitations"""
	ret = self.abspath_orig()
	if not ret.startswith("\\"):
		return "\\\\?\\" + ret
	return ret

if Utils.is_win32:
	Node.Node.find_resource_orig = Node.Node.find_resource
	Node.Node.find_resource = find_resource

	Node.Node.find_or_declare_orig = Node.Node.find_or_declare
	Node.Node.find_or_declare = find_or_declare

	Node.Node.abspath_orig = Node.Node.abspath
	Node.Node.abspath = abspath

	for k in list(Context.cache_modules.keys()):
		Context.cache_modules["\\\\?\\" + k] = Context.cache_modules[k]

