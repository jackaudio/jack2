#! /usr/bin/env python
# encoding: utf-8
# DC 2008
# Thomas Nagy 2010 (ita)

import re

from waflib import Utils, Task, TaskGen, Logs
from waflib.TaskGen import feature, before_method, after_method, extension
from waflib.Configure import conf

INC_REGEX = """(?:^|['">]\s*;)\s*(?:|#\s*)INCLUDE\s+(?:\w+_)?[<"'](.+?)(?=["'>])"""
USE_REGEX = """(?:^|;)\s*USE(?:\s+|(?:(?:\s*,\s*(?:NON_)?INTRINSIC)?\s*::))\s*(\w+)"""
MOD_REGEX = """(?:^|;)\s*MODULE(?!\s*PROCEDURE)(?:\s+|(?:(?:\s*,\s*(?:NON_)?INTRINSIC)?\s*::))\s*(\w+)"""

re_inc = re.compile(INC_REGEX, re.I)
re_use = re.compile(USE_REGEX, re.I)
re_mod = re.compile(MOD_REGEX, re.I)

class fortran_parser(object):
	"""
	This parser will return:

	* the nodes corresponding to the module names that will be produced
	* the nodes corresponding to the include files used
	* the module names used by the fortran file
	"""

	def __init__(self, incpaths):
		self.seen = []
		"""Files already parsed"""

		self.nodes = []
		"""List of :py:class:`waflib.Node.Node` representing the dependencies to return"""

		self.names = []
		"""List of module names to return"""

		self.incpaths = incpaths
		"""List of :py:class:`waflib.Node.Node` representing the include paths"""

	def find_deps(self, node):
		"""
		Parse a fortran file to read the dependencies used and provided

		:param node: fortran file to read
		:type node: :py:class:`waflib.Node.Node`
		:return: lists representing the includes, the modules used, and the modules created by a fortran file
		:rtype: tuple of list of strings
		"""
		txt = node.read()
		incs = []
		uses = []
		mods = []
		for line in txt.splitlines():
			# line by line regexp search? optimize?
			m = re_inc.search(line)
			if m:
				incs.append(m.group(1))
			m = re_use.search(line)
			if m:
				uses.append(m.group(1))
			m = re_mod.search(line)
			if m:
				mods.append(m.group(1))
		return (incs, uses, mods)

	def start(self, node):
		"""
		Start the parsing. Use the stack self.waiting to hold the nodes to iterate on

		:param node: fortran file
		:type node: :py:class:`waflib.Node.Node`
		"""
		self.waiting = [node]
		while self.waiting:
			nd = self.waiting.pop(0)
			self.iter(nd)

	def iter(self, node):
		"""
		Process a single file in the search for dependencies, extract the files used
		the modules used, and the modules provided.
		"""
		path = node.abspath()
		incs, uses, mods = self.find_deps(node)
		for x in incs:
			if x in self.seen:
				continue
			self.seen.append(x)
			self.tryfind_header(x)

		for x in uses:
			name = "USE@%s" % x
			if not name in self.names:
				self.names.append(name)

		for x in mods:
			name = "MOD@%s" % x
			if not name in self.names:
				self.names.append(name)

	def tryfind_header(self, filename):
		"""
		Try to find an include and add it the nodes to process

		:param filename: file name
		:type filename: string
		"""
		found = None
		for n in self.incpaths:
			found = n.find_resource(filename)
			if found:
				self.nodes.append(found)
				self.waiting.append(found)
				break
		if not found:
			if not filename in self.names:
				self.names.append(filename)


