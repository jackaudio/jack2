#!/usr/bin/env python
# encoding: utf-8
# Philipp Bender, 2012
# Matt Clarkson, 2012

import re
from waflib.Task import Task
from waflib.TaskGen import extension 

"""
A simple tool to integrate protocol buffers into your build system.

Example::

    def configure(conf):
        conf.load('compiler_cxx cxx protoc')

    def build(bld):
        bld(
                features = 'cxx cxxprogram'
                source   = 'main.cpp file1.proto proto/file2.proto', 
                include  = '. proto',
                target   = 'executable') 

Notes when using this tool:

- protoc command line parsing is tricky.

  The generated files can be put in subfolders which depend on
  the order of the include paths.

  Try to be simple when creating task generators
  containing protoc stuff.

"""

class protoc(Task):
	# protoc expects the input proto file to be an absolute path.
	run_str = '${PROTOC} ${PROTOC_FLAGS} ${PROTOC_ST:INCPATHS} ${SRC[0].abspath()}'
	color   = 'BLUE'
	ext_out = ['.h', 'pb.cc']
	def scan(self):
		"""
		Scan .proto dependencies
		"""
		node = self.inputs[0]

		nodes = []
		names = []
		seen = []

		if not node: return (nodes, names)

		search_paths = [self.generator.path.find_node(x) for x in self.generator.includes]

		def parse_node(node):
			if node in seen:
				return
			seen.append(node)
			code = node.read().splitlines()
			for line in code:
				m = re.search(r'^import\s+"(.*)";.*(//)?.*', line)
				if m:
					dep = m.groups()[0]
					for incpath in search_paths:
						found = incpath.find_resource(dep)
						if found:
							nodes.append(found)
							parse_node(found)
						else:
							names.append(dep)

		parse_node(node)
		return (nodes, names)

@extension('.proto')
def process_protoc(self, node):
	cpp_node = node.change_ext('.pb.cc')
	hpp_node = node.change_ext('.pb.h')
	self.create_task('protoc', node, [cpp_node, hpp_node])
	self.source.append(cpp_node)

	if 'cxx' in self.features and not self.env.PROTOC_FLAGS:
		#self.env.PROTOC_FLAGS = '--cpp_out=%s' % node.parent.get_bld().abspath() # <- this does not work
		self.env.PROTOC_FLAGS = '--cpp_out=.'

	use = getattr(self, 'use', '')
	if not 'PROTOBUF' in use:
		self.use = self.to_list(use) + ['PROTOBUF']

def configure(conf):
	conf.check_cfg(package="protobuf", uselib_store="PROTOBUF", args=['--cflags', '--libs'])
	conf.find_program('protoc', var='PROTOC')
	conf.env.PROTOC_ST = '-I%s'

