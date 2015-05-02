#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2006-2015 (ita)

"""
Build as batches.

Instead of compiling object files one by one, c/c++ compilers are often able to compile at once:
cc -c ../file1.c ../file2.c ../file3.c

Files are output on the directory where the compiler is called, and dependencies are more difficult
to track (do not run the command on all source files if only one file changes)

As such, we do as if the files were compiled one by one, but no command is actually run:
replace each cc/cpp Task by a TaskSlave. A new task called TaskMaster collects the
signatures from each slave and finds out the command-line to run.

Just import this module in the configuration (no other change required).
This is provided as an example, for performance unity builds are recommended (fewer tasks and
fewer jobs to execute). See waflib/extras/unity.py.
"""

from waflib import Task, Utils
from waflib.TaskGen import extension, feature, after_method
from waflib.Tools import c, cxx

MAX_BATCH = 50

c_str = '${CC} ${CFLAGS} ${CPPFLAGS} ${FRAMEWORKPATH_ST:FRAMEWORKPATH} ${CPPPATH_ST:INCPATHS} ${DEFINES_ST:DEFINES} -c ${SRCLST} ${CXX_TGT_F_BATCHED}'
c_fun, _ = Task.compile_fun_noshell(c_str)

cxx_str = '${CXX} ${CXXFLAGS} ${CPPFLAGS} ${FRAMEWORKPATH_ST:FRAMEWORKPATH} ${CPPPATH_ST:INCPATHS} ${DEFINES_ST:DEFINES} -c ${SRCLST} ${CXX_TGT_F_BATCHED}'
cxx_fun, _ = Task.compile_fun_noshell(cxx_str)

count = 70000
class batch_task(Task.Task):
	color = 'PINK'

	after = ['c', 'cxx']
	before = ['cprogram', 'cshlib', 'cstlib', 'cxxprogram', 'cxxshlib', 'cxxstlib']

	def uid(self):
		m = Utils.md5()
		m.update(Task.Task.uid(self))
		m.update(str(self.generator.idx).encode())
		return m.digest()

	def __str__(self):
		return 'Batch compilation for %d slaves' % len(self.slaves)

	def __init__(self, *k, **kw):
		Task.Task.__init__(self, *k, **kw)
		self.slaves = []
		self.inputs = []
		self.hasrun = 0

		global count
		count += 1
		self.idx = count

	def add_slave(self, slave):
		self.slaves.append(slave)
		self.set_run_after(slave)

	def runnable_status(self):
		for t in self.run_after:
			if not t.hasrun:
				return Task.ASK_LATER

		for t in self.slaves:
			#if t.executed:
			if t.hasrun != Task.SKIPPED:
				return Task.RUN_ME

		return Task.SKIP_ME

	def run(self):
		self.outputs = []

		srclst = []
		slaves = []
		for t in self.slaves:
			if t.hasrun != Task.SKIPPED:
				slaves.append(t)
				srclst.append(t.inputs[0].abspath())

		self.env.SRCLST = srclst
		self.cwd = slaves[0].outputs[0].parent.abspath()

		if self.slaves[0].__class__.__name__ == 'c':
			ret = c_fun(self)
		else:
			ret = cxx_fun(self)

		if ret:
			return ret

		for t in slaves:
			t.old_post_run()

def hook(cls_type):
	def n_hook(self, node):

		ext = '.obj' if self.env.CC_NAME == 'msvc' else '.o'
		name = node.name
		k = name.rfind('.')
		if k >= 0:
			basename = name[:k] + ext
		else:
			basename = name + ext

		outdir = node.parent.get_bld().make_node('%d' % self.idx)
		outdir.mkdir()
		out = outdir.find_or_declare(basename)

		task = self.create_task(cls_type, node, out)

		try:
			self.compiled_tasks.append(task)
		except AttributeError:
			self.compiled_tasks = [task]

		if not getattr(self, 'masters', None):
			self.masters = {}
			self.allmasters = []

		def fix_path(tsk):
			if self.env.CC_NAME == 'msvc':
				tsk.env.append_unique('CXX_TGT_F_BATCHED', '/Fo%s\\' % outdir.abspath())

		if not node.parent in self.masters:
			m = self.masters[node.parent] = self.master = self.create_task('batch')
			fix_path(m)
			self.allmasters.append(m)
		else:
			m = self.masters[node.parent]
			if len(m.slaves) > MAX_BATCH:
				m = self.masters[node.parent] = self.master = self.create_task('batch')
				fix_path(m)
				self.allmasters.append(m)
		m.add_slave(task)
		return task
	return n_hook

extension('.c')(hook('c'))
extension('.cpp','.cc','.cxx','.C','.c++')(hook('cxx'))

@feature('cprogram', 'cshlib', 'cstaticlib', 'cxxprogram', 'cxxshlib', 'cxxstlib')
@after_method('apply_link')
def link_after_masters(self):
	if getattr(self, 'allmasters', None):
		for m in self.allmasters:
			self.link_task.set_run_after(m)

# Modify the c and cxx task classes - in theory it would be best to
# create subclasses and to re-map the c/c++ extensions
for x in ('c', 'cxx'):
	t = Task.classes[x]
	def run(self):
		pass

	def post_run(self):
		pass

	setattr(t, 'oldrun', getattr(t, 'run', None))
	setattr(t, 'run', run)
	setattr(t, 'old_post_run', t.post_run)
	setattr(t, 'post_run', post_run)

