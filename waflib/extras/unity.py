#! /usr/bin/env python
# encoding: utf-8

"""
Compile whole groups of C/C++ files at once.

def build(bld):
	bld.load('compiler_cxx unity')
"""

import sys
from waflib import Task, Options
from waflib.Tools import c_preproc
from waflib import TaskGen

MAX_BATCH = 50

def options(opt):
	global MAX_BATCH
	opt.add_option('--batchsize', action='store', dest='batchsize', type='int', default=MAX_BATCH, help='batch size (0 for no batch)')

class unity(Task.Task):
	color = 'BLUE'
	scan = c_preproc.scan
	def run(self):
		lst = ['#include "%s"\n' % node.abspath() for node in self.inputs]
		txt = ''.join(lst)
		self.outputs[0].write(txt)

@TaskGen.taskgen_method
def batch_size(self):
	return getattr(Options.options, 'batchsize', MAX_BATCH)

def make_batch_fun(ext):
	# this generic code makes this quite unreadable, defining the function two times might have been better
	def make_batch(self, node):
		cnt = self.batch_size()
		if cnt <= 1:
			return self.create_compiled_task(ext, node)
		x = getattr(self, 'master_%s' % ext, None)
		if not x or len(x.inputs) >= cnt:
			x = self.create_task('unity')
			setattr(self, 'master_%s' % ext, x)

			cnt_cur = getattr(self, 'cnt_%s' % ext, 0)
			cxxnode = node.parent.find_or_declare('unity_%s_%d_%d.%s' % (self.idx, cnt_cur, cnt, ext))
			x.outputs = [cxxnode]
			setattr(self, 'cnt_%s' % ext, cnt_cur + 1)
			self.create_compiled_task(ext, cxxnode)
		x.inputs.append(node)
	return make_batch

def enable_support(cc, cxx):
	if cxx or not cc:
		make_cxx_batch = TaskGen.extension('.cpp', '.cc', '.cxx', '.C', '.c++')(make_batch_fun('cxx'))
	if cc:
		make_c_batch = TaskGen.extension('.c')(make_batch_fun('c'))
	else:
		TaskGen.task_gen.mappings['.c'] = TaskGen.task_gen.mappings['.cpp']

has_c = '.c' in TaskGen.task_gen.mappings or 'waflib.Tools.compiler_c' in sys.modules
has_cpp = '.cpp' in TaskGen.task_gen.mappings or 'waflib.Tools.compiler_cxx' in sys.modules
enable_support(has_c, has_cpp) # by default

def build(bld):
	# it is best to do this
	enable_support(bld.env.CC_NAME, bld.env.CXX_NAME)

