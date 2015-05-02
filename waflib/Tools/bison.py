#!/usr/bin/env python
# encoding: utf-8
# John O'Meara, 2006
# Thomas Nagy 2009-2010 (ita)

"""
The **bison** program is a code generator which creates C or C++ files.
The generated files are compiled into object files.
"""

from waflib import Task
from waflib.TaskGen import extension

class bison(Task.Task):
	"""Compile bison files"""
	color   = 'BLUE'
	run_str = '${BISON} ${BISONFLAGS} ${SRC[0].abspath()} -o ${TGT[0].name}'
	ext_out = ['.h'] # just to make sure

@extension('.y', '.yc', '.yy')
def big_bison(self, node):
	"""
	Create a bison task, which must be executed from the directory of the output file.
	"""
	has_h = '-d' in self.env['BISONFLAGS']

	outs = []
	if node.name.endswith('.yc'):
		outs.append(node.change_ext('.tab.cc'))
		if has_h:
			outs.append(node.change_ext('.tab.hh'))
	else:
		outs.append(node.change_ext('.tab.c'))
		if has_h:
			outs.append(node.change_ext('.tab.h'))

	tsk = self.create_task('bison', node, outs)
	tsk.cwd = node.parent.get_bld().abspath()

	# and the c/cxx file must be compiled too
	self.source.append(outs[0])

def configure(conf):
	"""
	Detect the *bison* program
	"""
	conf.find_program('bison', var='BISON')
	conf.env.BISONFLAGS = ['-d']

