#! /usr/bin/env python
# encoding: utf-8
# DC 2008
# Thomas Nagy 2010 (ita)

"""
fortran support
"""

from waflib import Utils, Task, Logs
from waflib.Tools import ccroot, fc_config, fc_scan
from waflib.TaskGen import feature, extension
from waflib.Configure import conf

ccroot.USELIB_VARS['fc'] = set(['FCFLAGS', 'DEFINES', 'INCLUDES'])
ccroot.USELIB_VARS['fcprogram_test'] = ccroot.USELIB_VARS['fcprogram'] = set(['LIB', 'STLIB', 'LIBPATH', 'STLIBPATH', 'LINKFLAGS', 'RPATH', 'LINKDEPS'])
ccroot.USELIB_VARS['fcshlib'] = set(['LIB', 'STLIB', 'LIBPATH', 'STLIBPATH', 'LINKFLAGS', 'RPATH', 'LINKDEPS'])
ccroot.USELIB_VARS['fcstlib'] = set(['ARFLAGS', 'LINKDEPS'])

@feature('fcprogram', 'fcshlib', 'fcstlib', 'fcprogram_test')
def dummy(self):
	"""
	Unused function that does nothing (TODO: remove in waf 1.9)
	"""
	pass

@extension('.f', '.f90', '.F', '.F90', '.for', '.FOR')
def fc_hook(self, node):
	"Bind the typical Fortran file extensions to the creation of a :py:class:`waflib.Tools.fc.fc` instance"
	return self.create_compiled_task('fc', node)

@conf
def modfile(conf, name):
	"""
	Turn a module name into the right module file name.
	Defaults to all lower case.
	"""
	return {'lower'     :name.lower() + '.mod',
		'lower.MOD' :name.upper() + '.MOD',
		'UPPER.mod' :name.upper() + '.mod',
		'UPPER'     :name.upper() + '.MOD'}[conf.env.FC_MOD_CAPITALIZATION or 'lower']

def get_fortran_tasks(tsk):
	"""
	Obtain all other fortran tasks from the same build group. Those tasks must not have
	the attribute 'nomod' or 'mod_fortran_done'
	"""
	bld = tsk.generator.bld
	tasks = bld.get_tasks_group(bld.get_group_idx(tsk.generator))
	return [x for x in tasks if isinstance(x, fc) and not getattr(x, 'nomod', None) and not getattr(x, 'mod_fortran_done', None)]

class fc(Task.Task):
	"""
	The fortran tasks can only run when all fortran tasks in the current group are ready to be executed
	This may cause a deadlock if another fortran task is waiting for something that cannot happen (circular dependency)
	in this case, set the 'nomod=True' on those tasks instances to break the loop
	"""

	color = 'GREEN'
	run_str = '${FC} ${FCFLAGS} ${FCINCPATH_ST:INCPATHS} ${FCDEFINES_ST:DEFINES} ${_FCMODOUTFLAGS} ${FC_TGT_F}${TGT[0].abspath()} ${FC_SRC_F}${SRC[0].abspath()}'
	vars = ["FORTRANMODPATHFLAG"]

	def scan(self):
		"""scanner for fortran dependencies"""
		tmp = fc_scan.fortran_parser(self.generator.includes_nodes)
		tmp.task = self
		tmp.start(self.inputs[0])
		if Logs.verbose:
			Logs.debug('deps: deps for %r: %r; unresolved %r' % (self.inputs, tmp.nodes, tmp.names))
		return (tmp.nodes, tmp.names)

	def runnable_status(self):
		"""
		Set the mod file outputs and the dependencies on the mod files over all the fortran tasks
		executed by the main thread so there are no concurrency issues
		"""
		if getattr(self, 'mod_fortran_done', None):
			return super(fc, self).runnable_status()

		# now, if we reach this part it is because this fortran task is the first in the list
		bld = self.generator.bld

		# obtain the fortran tasks
		lst = get_fortran_tasks(self)

		# disable this method for other tasks
		for tsk in lst:
			tsk.mod_fortran_done = True

		# wait for all the .f tasks to be ready for execution
		# and ensure that the scanners are called at least once
		for tsk in lst:
			ret = tsk.runnable_status()
			if ret == Task.ASK_LATER:
				# we have to wait for one of the other fortran tasks to be ready
				# this may deadlock if there are dependencies between the fortran tasks
				# but this should not happen (we are setting them here!)
				for x in lst:
					x.mod_fortran_done = None

				# TODO sort the list of tasks in bld.producer.outstanding to put all fortran tasks at the end
				return Task.ASK_LATER

		ins = Utils.defaultdict(set)
		outs = Utils.defaultdict(set)

		# the .mod files to create
		for tsk in lst:
			key = tsk.uid()
			for x in bld.raw_deps[key]:
				if x.startswith('MOD@'):
					name = bld.modfile(x.replace('MOD@', ''))
					node = bld.srcnode.find_or_declare(name)
					if not hasattr(node, 'sig'):
						node.sig = Utils.SIG_NIL
					tsk.set_outputs(node)
					outs[id(node)].add(tsk)

		# the .mod files to use
		for tsk in lst:
			key = tsk.uid()
			for x in bld.raw_deps[key]:
				if x.startswith('USE@'):
					name = bld.modfile(x.replace('USE@', ''))
					node = bld.srcnode.find_resource(name)
					if node and node not in tsk.outputs:
						if not node in bld.node_deps[key]:
							bld.node_deps[key].append(node)
						ins[id(node)].add(tsk)

		# if the intersection matches, set the order
		for k in ins.keys():
			for a in ins[k]:
				a.run_after.update(outs[k])

				# the scanner cannot output nodes, so we have to set them
				# ourselves as task.dep_nodes (additional input nodes)
				tmp = []
				for t in outs[k]:
					tmp.extend(t.outputs)
				a.dep_nodes.extend(tmp)
				a.dep_nodes.sort(key=lambda x: x.abspath())

		# the task objects have changed: clear the signature cache
		for tsk in lst:
			try:
				delattr(tsk, 'cache_sig')
			except AttributeError:
				pass

		return super(fc, self).runnable_status()

class fcprogram(ccroot.link_task):
	"""Link fortran programs"""
	color = 'YELLOW'
	run_str = '${FC} ${LINKFLAGS} ${FCLNK_SRC_F}${SRC} ${FCLNK_TGT_F}${TGT[0].abspath()} ${RPATH_ST:RPATH} ${FCSTLIB_MARKER} ${FCSTLIBPATH_ST:STLIBPATH} ${FCSTLIB_ST:STLIB} ${FCSHLIB_MARKER} ${FCLIBPATH_ST:LIBPATH} ${FCLIB_ST:LIB} ${LDFLAGS}'
	inst_to = '${BINDIR}'

class fcshlib(fcprogram):
	"""Link fortran libraries"""
	inst_to = '${LIBDIR}'

class fcprogram_test(fcprogram):
	"""Custom link task to obtain the compiler outputs for fortran configuration tests"""

	def runnable_status(self):
		"""This task is always executed"""
		ret = super(fcprogram_test, self).runnable_status()
		if ret == Task.SKIP_ME:
			ret = Task.RUN_ME
		return ret

	def exec_command(self, cmd, **kw):
		"""Store the compiler std our/err onto the build context, to bld.out + bld.err"""
		bld = self.generator.bld

		kw['shell'] = isinstance(cmd, str)
		kw['stdout'] = kw['stderr'] = Utils.subprocess.PIPE
		kw['cwd'] = bld.variant_dir
		bld.out = bld.err = ''

		bld.to_log('command: %s\n' % cmd)

		kw['output'] = 0
		try:
			(bld.out, bld.err) = bld.cmd_and_log(cmd, **kw)
		except Exception:
			return -1

		if bld.out:
			bld.to_log("out: %s\n" % bld.out)
		if bld.err:
			bld.to_log("err: %s\n" % bld.err)

class fcstlib(ccroot.stlink_task):
	"""Link fortran static libraries (uses ar by default)"""
	pass # do not remove the pass statement

