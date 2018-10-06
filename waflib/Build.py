#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2005-2010 (ita)

"""
Classes related to the build phase (build, clean, install, step, etc)

The inheritance tree is the following:

"""

import os, sys, errno, re, shutil, stat
try:
	import cPickle
except ImportError:
	import pickle as cPickle
from waflib import Runner, TaskGen, Utils, ConfigSet, Task, Logs, Options, Context, Errors
import waflib.Node

CACHE_DIR = 'c4che'
"""Location of the cache files"""

CACHE_SUFFIX = '_cache.py'
"""Suffix for the cache files"""

INSTALL = 1337
"""Positive value '->' install, see :py:attr:`waflib.Build.BuildContext.is_install`"""

UNINSTALL = -1337
"""Negative value '<-' uninstall, see :py:attr:`waflib.Build.BuildContext.is_install`"""

SAVED_ATTRS = 'root node_deps raw_deps task_sigs'.split()
"""Build class members to save between the runs (root, node_deps, raw_deps, task_sigs)"""

CFG_FILES = 'cfg_files'
"""Files from the build directory to hash before starting the build (``config.h`` written during the configuration)"""

POST_AT_ONCE = 0
"""Post mode: all task generators are posted before the build really starts"""

POST_LAZY = 1
"""Post mode: post the task generators group after group"""

POST_BOTH = 2
"""Post mode: post the task generators at once, then re-check them for each group"""

PROTOCOL = -1
if sys.platform == 'cli':
	PROTOCOL = 0

class BuildContext(Context.Context):
	'''executes the build'''

	cmd = 'build'
	variant = ''

	def __init__(self, **kw):
		super(BuildContext, self).__init__(**kw)

		self.is_install = 0
		"""Non-zero value when installing or uninstalling file"""

		self.top_dir = kw.get('top_dir', Context.top_dir)

		self.run_dir = kw.get('run_dir', Context.run_dir)

		self.post_mode = POST_AT_ONCE
		"""post the task generators at once, group-by-group, or both"""

		# output directory - may be set until the nodes are considered
		self.out_dir = kw.get('out_dir', Context.out_dir)

		self.cache_dir = kw.get('cache_dir', None)
		if not self.cache_dir:
			self.cache_dir = os.path.join(self.out_dir, CACHE_DIR)

		# map names to environments, the '' must be defined
		self.all_envs = {}

		# ======================================= #
		# cache variables

		self.task_sigs = {}
		"""Signatures of the tasks (persists between build executions)"""

		self.node_deps = {}
		"""Dict of node dependencies found by :py:meth:`waflib.Task.Task.scan` (persists between build executions)"""

		self.raw_deps = {}
		"""Dict of custom data returned by :py:meth:`waflib.Task.Task.scan` (persists between build executions)"""

		# list of folders that are already scanned
		# so that we do not need to stat them one more time
		self.cache_dir_contents = {}

		self.task_gen_cache_names = {}

		self.launch_dir = Context.launch_dir

		self.jobs = Options.options.jobs
		self.targets = Options.options.targets
		self.keep = Options.options.keep
		self.progress_bar = Options.options.progress_bar

		############ stuff below has not been reviewed

		# Manual dependencies.
		self.deps_man = Utils.defaultdict(list)
		"""Manual dependencies set by :py:meth:`waflib.Build.BuildContext.add_manual_dependency`"""

		# just the structure here
		self.current_group = 0
		"""
		Current build group
		"""

		self.groups = []
		"""
		List containing lists of task generators
		"""
		self.group_names = {}
		"""
		Map group names to the group lists. See :py:meth:`waflib.Build.BuildContext.add_group`
		"""

	def get_variant_dir(self):
		"""Getter for the variant_dir attribute"""
		if not self.variant:
			return self.out_dir
		return os.path.join(self.out_dir, self.variant)
	variant_dir = property(get_variant_dir, None)

	def __call__(self, *k, **kw):
		"""
		Create a task generator and add it to the current build group. The following forms are equivalent::

			def build(bld):
				tg = bld(a=1, b=2)

			def build(bld):
				tg = bld()
				tg.a = 1
				tg.b = 2

			def build(bld):
				tg = TaskGen.task_gen(a=1, b=2)
				bld.add_to_group(tg, None)

		:param group: group name to add the task generator to
		:type group: string
		"""
		kw['bld'] = self
		ret = TaskGen.task_gen(*k, **kw)
		self.task_gen_cache_names = {} # reset the cache, each time
		self.add_to_group(ret, group=kw.get('group', None))
		return ret

	def rule(self, *k, **kw):
		"""
		Wrapper for creating a task generator using the decorator notation. The following code::

			@bld.rule(
				target = "foo"
			)
			def _(tsk):
				print("bar")

		is equivalent to::

			def bar(tsk):
				print("bar")

			bld(
				target = "foo",
				rule = bar,
			)
		"""
		def f(rule):
			ret = self(*k, **kw)
			ret.rule = rule
			return ret
		return f

	def __copy__(self):
		"""Implemented to prevents copies of build contexts (raises an exception)"""
		raise Errors.WafError('build contexts are not supposed to be copied')

	def install_files(self, *k, **kw):
		"""Actual implementation provided by :py:meth:`waflib.Build.InstallContext.install_files`"""
		pass

	def install_as(self, *k, **kw):
		"""Actual implementation provided by :py:meth:`waflib.Build.InstallContext.install_as`"""
		pass

	def symlink_as(self, *k, **kw):
		"""Actual implementation provided by :py:meth:`waflib.Build.InstallContext.symlink_as`"""
		pass

	def load_envs(self):
		"""
		The configuration command creates files of the form ``build/c4che/NAMEcache.py``. This method
		creates a :py:class:`waflib.ConfigSet.ConfigSet` instance for each ``NAME`` by reading those
		files. The config sets are then stored in the dict :py:attr:`waflib.Build.BuildContext.allenvs`.
		"""
		node = self.root.find_node(self.cache_dir)
		if not node:
			raise Errors.WafError('The project was not configured: run "waf configure" first!')
		lst = node.ant_glob('**/*%s' % CACHE_SUFFIX, quiet=True)

		if not lst:
			raise Errors.WafError('The cache directory is empty: reconfigure the project')

		for x in lst:
			name = x.path_from(node).replace(CACHE_SUFFIX, '').replace('\\', '/')
			env = ConfigSet.ConfigSet(x.abspath())
			self.all_envs[name] = env
			for f in env[CFG_FILES]:
				newnode = self.root.find_resource(f)
				try:
					h = Utils.h_file(newnode.abspath())
				except (IOError, AttributeError):
					Logs.error('cannot find %r' % f)
					h = Utils.SIG_NIL
				newnode.sig = h

	def init_dirs(self):
		"""
		Initialize the project directory and the build directory by creating the nodes
		:py:attr:`waflib.Build.BuildContext.srcnode` and :py:attr:`waflib.Build.BuildContext.bldnode`
		corresponding to ``top_dir`` and ``variant_dir`` respectively. The ``bldnode`` directory will be
		created if it does not exist.
		"""

		if not (os.path.isabs(self.top_dir) and os.path.isabs(self.out_dir)):
			raise Errors.WafError('The project was not configured: run "waf configure" first!')

		self.path = self.srcnode = self.root.find_dir(self.top_dir)
		self.bldnode = self.root.make_node(self.variant_dir)
		self.bldnode.mkdir()

	def execute(self):
		"""
		Restore the data from previous builds and call :py:meth:`waflib.Build.BuildContext.execute_build`. Overrides from :py:func:`waflib.Context.Context.execute`
		"""
		self.restore()
		if not self.all_envs:
			self.load_envs()

		self.execute_build()

	def execute_build(self):
		"""
		Execute the build by:

		* reading the scripts (see :py:meth:`waflib.Context.Context.recurse`)
		* calling :py:meth:`waflib.Build.BuildContext.pre_build` to call user build functions
		* calling :py:meth:`waflib.Build.BuildContext.compile` to process the tasks
		* calling :py:meth:`waflib.Build.BuildContext.post_build` to call user build functions
		"""

		Logs.info("Waf: Entering directory `%s'" % self.variant_dir)
		self.recurse([self.run_dir])
		self.pre_build()

		# display the time elapsed in the progress bar
		self.timer = Utils.Timer()

		try:
			self.compile()
		finally:
			if self.progress_bar == 1 and sys.stderr.isatty():
				c = len(self.returned_tasks) or 1
				m = self.progress_line(c, c, Logs.colors.BLUE, Logs.colors.NORMAL)
				Logs.info(m, extra={'stream': sys.stderr, 'c1': Logs.colors.cursor_off, 'c2' : Logs.colors.cursor_on})
			Logs.info("Waf: Leaving directory `%s'" % self.variant_dir)
		self.post_build()

	def restore(self):
		"""
		Load the data from a previous run, sets the attributes listed in :py:const:`waflib.Build.SAVED_ATTRS`
		"""
		try:
			env = ConfigSet.ConfigSet(os.path.join(self.cache_dir, 'build.config.py'))
		except EnvironmentError:
			pass
		else:
			if env['version'] < Context.HEXVERSION:
				raise Errors.WafError('Version mismatch! reconfigure the project')
			for t in env['tools']:
				self.setup(**t)

		dbfn = os.path.join(self.variant_dir, Context.DBFILE)
		try:
			data = Utils.readf(dbfn, 'rb')
		except (IOError, EOFError):
			# handle missing file/empty file
			Logs.debug('build: Could not load the build cache %s (missing)' % dbfn)
		else:
			try:
				waflib.Node.pickle_lock.acquire()
				waflib.Node.Nod3 = self.node_class
				try:
					data = cPickle.loads(data)
				except Exception as e:
					Logs.debug('build: Could not pickle the build cache %s: %r' % (dbfn, e))
				else:
					for x in SAVED_ATTRS:
						setattr(self, x, data[x])
			finally:
				waflib.Node.pickle_lock.release()

		self.init_dirs()

	def store(self):
		"""
		Store the data for next runs, sets the attributes listed in :py:const:`waflib.Build.SAVED_ATTRS`. Uses a temporary
		file to avoid problems on ctrl+c.
		"""

		data = {}
		for x in SAVED_ATTRS:
			data[x] = getattr(self, x)
		db = os.path.join(self.variant_dir, Context.DBFILE)

		try:
			waflib.Node.pickle_lock.acquire()
			waflib.Node.Nod3 = self.node_class
			x = cPickle.dumps(data, PROTOCOL)
		finally:
			waflib.Node.pickle_lock.release()

		Utils.writef(db + '.tmp', x, m='wb')

		try:
			st = os.stat(db)
			os.remove(db)
			if not Utils.is_win32: # win32 has no chown but we're paranoid
				os.chown(db + '.tmp', st.st_uid, st.st_gid)
		except (AttributeError, OSError):
			pass

		# do not use shutil.move (copy is not thread-safe)
		os.rename(db + '.tmp', db)

	def compile(self):
		"""
		Run the build by creating an instance of :py:class:`waflib.Runner.Parallel`
		The cache file is not written if the build is up to date (no task executed).
		"""
		Logs.debug('build: compile()')

		# use another object to perform the producer-consumer logic (reduce the complexity)
		self.producer = Runner.Parallel(self, self.jobs)
		self.producer.biter = self.get_build_iterator()
		self.returned_tasks = [] # not part of the API yet
		try:
			self.producer.start()
		except KeyboardInterrupt:
			self.store()
			raise
		else:
			if self.producer.dirty:
				self.store()

		if self.producer.error:
			raise Errors.BuildError(self.producer.error)

	def setup(self, tool, tooldir=None, funs=None):
		"""
		Import waf tools, used to import those accessed during the configuration::

			def configure(conf):
				conf.load('glib2')

			def build(bld):
				pass # glib2 is imported implicitly

		:param tool: tool list
		:type tool: list
		:param tooldir: optional tool directory (sys.path)
		:type tooldir: list of string
		:param funs: unused variable
		"""
		if isinstance(tool, list):
			for i in tool: self.setup(i, tooldir)
			return

		module = Context.load_tool(tool, tooldir)
		if hasattr(module, "setup"): module.setup(self)

	def get_env(self):
		"""Getter for the env property"""
		try:
			return self.all_envs[self.variant]
		except KeyError:
			return self.all_envs['']
	def set_env(self, val):
		"""Setter for the env property"""
		self.all_envs[self.variant] = val

	env = property(get_env, set_env)

	def add_manual_dependency(self, path, value):
		"""
		Adds a dependency from a node object to a value::

			def build(bld):
				bld.add_manual_dependency(
					bld.path.find_resource('wscript'),
					bld.root.find_resource('/etc/fstab'))

		:param path: file path
		:type path: string or :py:class:`waflib.Node.Node`
		:param value: value to depend on
		:type value: :py:class:`waflib.Node.Node`, string, or function returning a string
		"""
		if path is None:
			raise ValueError('Invalid input')

		if isinstance(path, waflib.Node.Node):
			node = path
		elif os.path.isabs(path):
			node = self.root.find_resource(path)
		else:
			node = self.path.find_resource(path)

		if isinstance(value, list):
			self.deps_man[id(node)].extend(value)
		else:
			self.deps_man[id(node)].append(value)

	def launch_node(self):
		"""Returns the launch directory as a :py:class:`waflib.Node.Node` object"""
		try:
			# private cache
			return self.p_ln
		except AttributeError:
			self.p_ln = self.root.find_dir(self.launch_dir)
			return self.p_ln

	def hash_env_vars(self, env, vars_lst):
		"""
		Hash configuration set variables::

			def build(bld):
				bld.hash_env_vars(bld.env, ['CXX', 'CC'])

		:param env: Configuration Set
		:type env: :py:class:`waflib.ConfigSet.ConfigSet`
		:param vars_lst: list of variables
		:type vars_list: list of string
		"""

		if not env.table:
			env = env.parent
			if not env:
				return Utils.SIG_NIL

		idx = str(id(env)) + str(vars_lst)
		try:
			cache = self.cache_env
		except AttributeError:
			cache = self.cache_env = {}
		else:
			try:
				return self.cache_env[idx]
			except KeyError:
				pass

		lst = [env[a] for a in vars_lst]
		ret = Utils.h_list(lst)
		Logs.debug('envhash: %s %r', Utils.to_hex(ret), lst)

		cache[idx] = ret

		return ret

	def get_tgen_by_name(self, name):
		"""
		Retrieves a task generator from its name or its target name
		the name must be unique::

			def build(bld):
				tg = bld(name='foo')
				tg == bld.get_tgen_by_name('foo')
		"""
		cache = self.task_gen_cache_names
		if not cache:
			# create the index lazily
			for g in self.groups:
				for tg in g:
					try:
						cache[tg.name] = tg
					except AttributeError:
						# raised if not a task generator, which should be uncommon
						pass
		try:
			return cache[name]
		except KeyError:
			raise Errors.WafError('Could not find a task generator for the name %r' % name)

	def progress_line(self, state, total, col1, col2):
		"""
		Compute the progress bar used by ``waf -p``
		"""
		if not sys.stderr.isatty():
			return ''

		n = len(str(total))

		Utils.rot_idx += 1
		ind = Utils.rot_chr[Utils.rot_idx % 4]

		pc = (100.*state)/total
		eta = str(self.timer)
		fs = "[%%%dd/%%%dd][%%s%%2d%%%%%%s][%s][" % (n, n, ind)
		left = fs % (state, total, col1, pc, col2)
		right = '][%s%s%s]' % (col1, eta, col2)

		cols = Logs.get_term_cols() - len(left) - len(right) + 2*len(col1) + 2*len(col2)
		if cols < 7: cols = 7

		ratio = ((cols*state)//total) - 1

		bar = ('='*ratio+'>').ljust(cols)
		msg = Logs.indicator % (left, bar, right)

		return msg

	def declare_chain(self, *k, **kw):
		"""
		Wrapper for :py:func:`waflib.TaskGen.declare_chain` provided for convenience
		"""
		return TaskGen.declare_chain(*k, **kw)

	def pre_build(self):
		"""Execute user-defined methods before the build starts, see :py:meth:`waflib.Build.BuildContext.add_pre_fun`"""
		for m in getattr(self, 'pre_funs', []):
			m(self)

	def post_build(self):
		"""Executes the user-defined methods after the build is successful, see :py:meth:`waflib.Build.BuildContext.add_post_fun`"""
		for m in getattr(self, 'post_funs', []):
			m(self)

	def add_pre_fun(self, meth):
		"""
		Bind a method to execute after the scripts are read and before the build starts::

			def mycallback(bld):
				print("Hello, world!")

			def build(bld):
				bld.add_pre_fun(mycallback)
		"""
		try:
			self.pre_funs.append(meth)
		except AttributeError:
			self.pre_funs = [meth]

	def add_post_fun(self, meth):
		"""
		Bind a method to execute immediately after the build is successful::

			def call_ldconfig(bld):
				bld.exec_command('/sbin/ldconfig')

			def build(bld):
				if bld.cmd == 'install':
					bld.add_pre_fun(call_ldconfig)
		"""
		try:
			self.post_funs.append(meth)
		except AttributeError:
			self.post_funs = [meth]

	def get_group(self, x):
		"""
		Get the group x, or return the current group if x is None

		:param x: name or number or None
		:type x: string, int or None
		"""
		if not self.groups:
			self.add_group()
		if x is None:
			return self.groups[self.current_group]
		if x in self.group_names:
			return self.group_names[x]
		return self.groups[x]

	def add_to_group(self, tgen, group=None):
		"""add a task or a task generator for the build"""
		# paranoid
		assert(isinstance(tgen, TaskGen.task_gen) or isinstance(tgen, Task.TaskBase))
		tgen.bld = self
		self.get_group(group).append(tgen)

	def get_group_name(self, g):
		"""name for the group g (utility)"""
		if not isinstance(g, list):
			g = self.groups[g]
		for x in self.group_names:
			if id(self.group_names[x]) == id(g):
				return x
		return ''

	def get_group_idx(self, tg):
		"""
		Index of the group containing the task generator given as argument::

			def build(bld):
				tg = bld(name='nada')
				0 == bld.get_group_idx(tg)

		:param tg: Task generator object
		:type tg: :py:class:`waflib.TaskGen.task_gen`
		"""
		se = id(tg)
		for i in range(len(self.groups)):
			for t in self.groups[i]:
				if id(t) == se:
					return i
		return None

	def add_group(self, name=None, move=True):
		"""
		Add a new group of tasks/task generators. By default the new group becomes the default group for new task generators.

		:param name: name for this group
		:type name: string
		:param move: set the group created as default group (True by default)
		:type move: bool
		"""
		#if self.groups and not self.groups[0].tasks:
		#	error('add_group: an empty group is already present')
		if name and name in self.group_names:
			Logs.error('add_group: name %s already present' % name)
		g = []
		self.group_names[name] = g
		self.groups.append(g)
		if move:
			self.current_group = len(self.groups) - 1

	def set_group(self, idx):
		"""
		Set the current group to be idx: now new task generators will be added to this group by default::

			def build(bld):
				bld(rule='touch ${TGT}', target='foo.txt')
				bld.add_group() # now the current group is 1
				bld(rule='touch ${TGT}', target='bar.txt')
				bld.set_group(0) # now the current group is 0
				bld(rule='touch ${TGT}', target='truc.txt') # build truc.txt before bar.txt

		:param idx: group name or group index
		:type idx: string or int
		"""
		if isinstance(idx, str):
			g = self.group_names[idx]
			for i in range(len(self.groups)):
				if id(g) == id(self.groups[i]):
					self.current_group = i
					break
		else:
			self.current_group = idx

	def total(self):
		"""
		Approximate task count: this value may be inaccurate if task generators are posted lazily (see :py:attr:`waflib.Build.BuildContext.post_mode`).
		The value :py:attr:`waflib.Runner.Parallel.total` is updated during the task execution.
		"""
		total = 0
		for group in self.groups:
			for tg in group:
				try:
					total += len(tg.tasks)
				except AttributeError:
					total += 1
		return total

	def get_targets(self):
		"""
		Return the task generator corresponding to the 'targets' list, used by :py:meth:`waflib.Build.BuildContext.get_build_iterator`::

			$ waf --targets=myprogram,myshlib
		"""
		to_post = []
		min_grp = 0
		for name in self.targets.split(','):
			tg = self.get_tgen_by_name(name)
			m = self.get_group_idx(tg)
			if m > min_grp:
				min_grp = m
				to_post = [tg]
			elif m == min_grp:
				to_post.append(tg)
		return (min_grp, to_post)

	def get_all_task_gen(self):
		"""
		Utility method, returns a list of all task generators - if you need something more complicated, implement your own
		"""
		lst = []
		for g in self.groups:
			lst.extend(g)
		return lst

	def post_group(self):
		"""
		Post the task generators from the group indexed by self.cur, used by :py:meth:`waflib.Build.BuildContext.get_build_iterator`
		"""
		if self.targets == '*':
			for tg in self.groups[self.cur]:
				try:
					f = tg.post
				except AttributeError:
					pass
				else:
					f()
		elif self.targets:
			if self.cur < self._min_grp:
				for tg in self.groups[self.cur]:
					try:
						f = tg.post
					except AttributeError:
						pass
					else:
						f()
			else:
				for tg in self._exact_tg:
					tg.post()
		else:
			ln = self.launch_node()
			if ln.is_child_of(self.bldnode):
				Logs.warn('Building from the build directory, forcing --targets=*')
				ln = self.srcnode
			elif not ln.is_child_of(self.srcnode):
				Logs.warn('CWD %s is not under %s, forcing --targets=* (run distclean?)' % (ln.abspath(), self.srcnode.abspath()))
				ln = self.srcnode
			for tg in self.groups[self.cur]:
				try:
					f = tg.post
				except AttributeError:
					pass
				else:
					if tg.path.is_child_of(ln):
						f()

	def get_tasks_group(self, idx):
		"""
		Return all the tasks for the group of num idx, used by :py:meth:`waflib.Build.BuildContext.get_build_iterator`
		"""
		tasks = []
		for tg in self.groups[idx]:
			try:
				tasks.extend(tg.tasks)
			except AttributeError: # not a task generator, can be the case for installation tasks
				tasks.append(tg)
		return tasks

	def get_build_iterator(self):
		"""
		Creates a generator object that returns lists of tasks executable in parallel (yield)

		:return: tasks which can be executed immediatly
		:rtype: list of :py:class:`waflib.Task.TaskBase`
		"""
		self.cur = 0

		if self.targets and self.targets != '*':
			(self._min_grp, self._exact_tg) = self.get_targets()

		global lazy_post
		if self.post_mode != POST_LAZY:
			while self.cur < len(self.groups):
				self.post_group()
				self.cur += 1
			self.cur = 0

		while self.cur < len(self.groups):
			# first post the task generators for the group
			if self.post_mode != POST_AT_ONCE:
				self.post_group()

			# then extract the tasks
			tasks = self.get_tasks_group(self.cur)
			# if the constraints are set properly (ext_in/ext_out, before/after)
			# the call to set_file_constraints may be removed (can be a 15% penalty on no-op rebuilds)
			# (but leave set_file_constraints for the installation step)
			#
			# if the tasks have only files, set_file_constraints is required but set_precedence_constraints is not necessary
			#
			Task.set_file_constraints(tasks)
			Task.set_precedence_constraints(tasks)

			self.cur_tasks = tasks
			self.cur += 1
			if not tasks: # return something else the build will stop
				continue
			yield tasks
		while 1:
			yield []

class inst(Task.Task):
	"""
	Special task used for installing files and symlinks, it behaves both like a task
	and like a task generator
	"""
	color = 'CYAN'

	def uid(self):
		lst = [self.dest, self.path] + self.source
		return Utils.h_list(repr(lst))

	def post(self):
		"""
		Same interface as in :py:meth:`waflib.TaskGen.task_gen.post`
		"""
		buf = []
		for x in self.source:
			if isinstance(x, waflib.Node.Node):
				y = x
			else:
				y = self.path.find_resource(x)
				if not y:
					if os.path.isabs(x):
						y = self.bld.root.make_node(x)
					else:
						y = self.path.make_node(x)
			buf.append(y)
		self.inputs = buf

	def runnable_status(self):
		"""
		Installation tasks are always executed, so this method returns either :py:const:`waflib.Task.ASK_LATER` or :py:const:`waflib.Task.RUN_ME`.
		"""
		ret = super(inst, self).runnable_status()
		if ret == Task.SKIP_ME:
			return Task.RUN_ME
		return ret

	def __str__(self):
		"""Return an empty string to disable the display"""
		return ''

	def run(self):
		"""The attribute 'exec_task' holds the method to execute"""
		return self.generator.exec_task()

	def get_install_path(self, destdir=True):
		"""
		Installation path obtained from ``self.dest`` and prefixed by the destdir.
		The variables such as '${PREFIX}/bin' are substituted.
		"""
		dest = Utils.subst_vars(self.dest, self.env)
		dest = dest.replace('/', os.sep)
		if destdir and Options.options.destdir:
			dest = os.path.join(Options.options.destdir, os.path.splitdrive(dest)[1].lstrip(os.sep))
		return dest

	def exec_install_files(self):
		"""
		Predefined method for installing files
		"""
		destpath = self.get_install_path()
		if not destpath:
			raise Errors.WafError('unknown installation path %r' % self.generator)
		for x, y in zip(self.source, self.inputs):
			if self.relative_trick:
				destfile = os.path.join(destpath, y.path_from(self.path))
			else:
				destfile = os.path.join(destpath, y.name)
			self.generator.bld.do_install(y.abspath(), destfile, chmod=self.chmod, tsk=self)

	def exec_install_as(self):
		"""
		Predefined method for installing one file with a given name
		"""
		destfile = self.get_install_path()
		self.generator.bld.do_install(self.inputs[0].abspath(), destfile, chmod=self.chmod, tsk=self)

	def exec_symlink_as(self):
		"""
		Predefined method for installing a symlink
		"""
		destfile = self.get_install_path()
		src = self.link
		if self.relative_trick:
			src = os.path.relpath(src, os.path.dirname(destfile))
		self.generator.bld.do_link(src, destfile, tsk=self)

class InstallContext(BuildContext):
	'''installs the targets on the system'''
	cmd = 'install'

	def __init__(self, **kw):
		super(InstallContext, self).__init__(**kw)

		# list of targets to uninstall for removing the empty folders after uninstalling
		self.uninstall = []
		self.is_install = INSTALL

	def copy_fun(self, src, tgt, **kw):
		# override this if you want to strip executables
		# kw['tsk'].source is the task that created the files in the build
		if Utils.is_win32 and len(tgt) > 259 and not tgt.startswith('\\\\?\\'):
			tgt = '\\\\?\\' + tgt
		shutil.copy2(src, tgt)
		os.chmod(tgt, kw.get('chmod', Utils.O644))

	def do_install(self, src, tgt, **kw):
		"""
		Copy a file from src to tgt with given file permissions. The actual copy is not performed
		if the source and target file have the same size and the same timestamps. When the copy occurs,
		the file is first removed and then copied (prevent stale inodes).

		This method is overridden in :py:meth:`waflib.Build.UninstallContext.do_install` to remove the file.

		:param src: file name as absolute path
		:type src: string
		:param tgt: file destination, as absolute path
		:type tgt: string
		:param chmod: installation mode
		:type chmod: int
		"""
		d, _ = os.path.split(tgt)
		if not d:
			raise Errors.WafError('Invalid installation given %r->%r' % (src, tgt))
		Utils.check_dir(d)

		srclbl = src.replace(self.srcnode.abspath() + os.sep, '')
		if not Options.options.force:
			# check if the file is already there to avoid a copy
			try:
				st1 = os.stat(tgt)
				st2 = os.stat(src)
			except OSError:
				pass
			else:
				# same size and identical timestamps -> make no copy
				if st1.st_mtime + 2 >= st2.st_mtime and st1.st_size == st2.st_size:
					if not self.progress_bar:
						Logs.info('- install %s (from %s)' % (tgt, srclbl))
					return False

		if not self.progress_bar:
			Logs.info('+ install %s (from %s)' % (tgt, srclbl))

		# Give best attempt at making destination overwritable,
		# like the 'install' utility used by 'make install' does.
		try:
			os.chmod(tgt, Utils.O644 | stat.S_IMODE(os.stat(tgt).st_mode))
		except EnvironmentError:
			pass

		# following is for shared libs and stale inodes (-_-)
		try:
			os.remove(tgt)
		except OSError:
			pass

		try:
			self.copy_fun(src, tgt, **kw)
		except IOError:
			try:
				os.stat(src)
			except EnvironmentError:
				Logs.error('File %r does not exist' % src)
			raise Errors.WafError('Could not install the file %r' % tgt)

	def do_link(self, src, tgt, **kw):
		"""
		Create a symlink from tgt to src.

		This method is overridden in :py:meth:`waflib.Build.UninstallContext.do_link` to remove the symlink.

		:param src: file name as absolute path
		:type src: string
		:param tgt: file destination, as absolute path
		:type tgt: string
		"""
		d, _ = os.path.split(tgt)
		Utils.check_dir(d)

		link = False
		if not os.path.islink(tgt):
			link = True
		elif os.readlink(tgt) != src:
			link = True

		if link:
			try: os.remove(tgt)
			except OSError: pass
			if not self.progress_bar:
				Logs.info('+ symlink %s (to %s)' % (tgt, src))
			os.symlink(src, tgt)
		else:
			if not self.progress_bar:
				Logs.info('- symlink %s (to %s)' % (tgt, src))

	def run_task_now(self, tsk, postpone):
		"""
		This method is called by :py:meth:`waflib.Build.InstallContext.install_files`,
		:py:meth:`waflib.Build.InstallContext.install_as` and :py:meth:`waflib.Build.InstallContext.symlink_as` immediately
		after the installation task is created. Its role is to force the immediate execution if necessary, that is when
		``postpone=False`` was given.
		"""
		tsk.post()
		if not postpone:
			if tsk.runnable_status() == Task.ASK_LATER:
				raise self.WafError('cannot post the task %r' % tsk)
			tsk.run()
			tsk.hasrun = True

	def install_files(self, dest, files, env=None, chmod=Utils.O644, relative_trick=False, cwd=None, add=True, postpone=True, task=None):
		"""
		Create a task to install files on the system::

			def build(bld):
				bld.install_files('${DATADIR}', self.path.find_resource('wscript'))

		:param dest: absolute path of the destination directory
		:type dest: string
		:param files: input files
		:type files: list of strings or list of nodes
		:param env: configuration set for performing substitutions in dest
		:type env: Configuration set
		:param relative_trick: preserve the folder hierarchy when installing whole folders
		:type relative_trick: bool
		:param cwd: parent node for searching srcfile, when srcfile is not a :py:class:`waflib.Node.Node`
		:type cwd: :py:class:`waflib.Node.Node`
		:param add: add the task created to a build group - set ``False`` only if the installation task is created after the build has started
		:type add: bool
		:param postpone: execute the task immediately to perform the installation
		:type postpone: bool
		"""
		assert(dest)
		tsk = inst(env=env or self.env)
		tsk.bld = self
		tsk.path = cwd or self.path
		tsk.chmod = chmod
		tsk.task = task
		if isinstance(files, waflib.Node.Node):
			tsk.source =  [files]
		else:
			tsk.source = Utils.to_list(files)
		tsk.dest = dest
		tsk.exec_task = tsk.exec_install_files
		tsk.relative_trick = relative_trick
		if add: self.add_to_group(tsk)
		self.run_task_now(tsk, postpone)
		return tsk

	def install_as(self, dest, srcfile, env=None, chmod=Utils.O644, cwd=None, add=True, postpone=True, task=None):
		"""
		Create a task to install a file on the system with a different name::

			def build(bld):
				bld.install_as('${PREFIX}/bin', 'myapp', chmod=Utils.O755)

		:param dest: absolute path of the destination file
		:type dest: string
		:param srcfile: input file
		:type srcfile: string or node
		:param cwd: parent node for searching srcfile, when srcfile is not a :py:class:`waflib.Node.Node`
		:type cwd: :py:class:`waflib.Node.Node`
		:param env: configuration set for performing substitutions in dest
		:type env: Configuration set
		:param add: add the task created to a build group - set ``False`` only if the installation task is created after the build has started
		:type add: bool
		:param postpone: execute the task immediately to perform the installation
		:type postpone: bool
		"""
		assert(dest)
		tsk = inst(env=env or self.env)
		tsk.bld = self
		tsk.path = cwd or self.path
		tsk.chmod = chmod
		tsk.source = [srcfile]
		tsk.task = task
		tsk.dest = dest
		tsk.exec_task = tsk.exec_install_as
		if add: self.add_to_group(tsk)
		self.run_task_now(tsk, postpone)
		return tsk

	def symlink_as(self, dest, src, env=None, cwd=None, add=True, postpone=True, relative_trick=False, task=None):
		"""
		Create a task to install a symlink::

			def build(bld):
				bld.symlink_as('${PREFIX}/lib/libfoo.so', 'libfoo.so.1.2.3')

		:param dest: absolute path of the symlink
		:type dest: string
		:param src: absolute or relative path of the link
		:type src: string
		:param env: configuration set for performing substitutions in dest
		:type env: Configuration set
		:param add: add the task created to a build group - set ``False`` only if the installation task is created after the build has started
		:type add: bool
		:param postpone: execute the task immediately to perform the installation
		:type postpone: bool
		:param relative_trick: make the symlink relative (default: ``False``)
		:type relative_trick: bool
		"""
		if Utils.is_win32:
			# symlinks *cannot* work on that platform
			# TODO waf 1.9 - replace by install_as
			return
		assert(dest)
		tsk = inst(env=env or self.env)
		tsk.bld = self
		tsk.dest = dest
		tsk.path = cwd or self.path
		tsk.source = []
		tsk.task = task
		tsk.link = src
		tsk.relative_trick = relative_trick
		tsk.exec_task = tsk.exec_symlink_as
		if add: self.add_to_group(tsk)
		self.run_task_now(tsk, postpone)
		return tsk

class UninstallContext(InstallContext):
	'''removes the targets installed'''
	cmd = 'uninstall'

	def __init__(self, **kw):
		super(UninstallContext, self).__init__(**kw)
		self.is_install = UNINSTALL

	def rm_empty_dirs(self, tgt):
		while tgt:
			tgt = os.path.dirname(tgt)
			try:
				os.rmdir(tgt)
			except OSError:
				break

	def do_install(self, src, tgt, **kw):
		"""See :py:meth:`waflib.Build.InstallContext.do_install`"""
		if not self.progress_bar:
			Logs.info('- remove %s' % tgt)

		self.uninstall.append(tgt)
		try:
			os.remove(tgt)
		except OSError as e:
			if e.errno != errno.ENOENT:
				if not getattr(self, 'uninstall_error', None):
					self.uninstall_error = True
					Logs.warn('build: some files could not be uninstalled (retry with -vv to list them)')
				if Logs.verbose > 1:
					Logs.warn('Could not remove %s (error code %r)' % (e.filename, e.errno))

		self.rm_empty_dirs(tgt)

	def do_link(self, src, tgt, **kw):
		"""See :py:meth:`waflib.Build.InstallContext.do_link`"""
		try:
			if not self.progress_bar:
				Logs.info('- remove %s' % tgt)
			os.remove(tgt)
		except OSError:
			pass

		self.rm_empty_dirs(tgt)

	def execute(self):
		"""
		See :py:func:`waflib.Context.Context.execute`
		"""
		try:
			# do not execute any tasks
			def runnable_status(self):
				return Task.SKIP_ME
			setattr(Task.Task, 'runnable_status_back', Task.Task.runnable_status)
			setattr(Task.Task, 'runnable_status', runnable_status)

			super(UninstallContext, self).execute()
		finally:
			setattr(Task.Task, 'runnable_status', Task.Task.runnable_status_back)

class CleanContext(BuildContext):
	'''cleans the project'''
	cmd = 'clean'
	def execute(self):
		"""
		See :py:func:`waflib.Context.Context.execute`
		"""
		self.restore()
		if not self.all_envs:
			self.load_envs()

		self.recurse([self.run_dir])
		try:
			self.clean()
		finally:
			self.store()

	def clean(self):
		"""Remove files from the build directory if possible, and reset the caches"""
		Logs.debug('build: clean called')

		if self.bldnode != self.srcnode:
			# would lead to a disaster if top == out
			lst=[]
			for e in self.all_envs.values():
				lst.extend(self.root.find_or_declare(f) for f in e[CFG_FILES])
			for n in self.bldnode.ant_glob('**/*', excl='.lock* *conf_check_*/** config.log c4che/*', quiet=True):
				if n in lst:
					continue
				n.delete()
		self.root.children = {}

		for v in 'node_deps task_sigs raw_deps'.split():
			setattr(self, v, {})

class ListContext(BuildContext):
	'''lists the targets to execute'''

	cmd = 'list'
	def execute(self):
		"""
		See :py:func:`waflib.Context.Context.execute`.
		"""
		self.restore()
		if not self.all_envs:
			self.load_envs()

		self.recurse([self.run_dir])
		self.pre_build()

		# display the time elapsed in the progress bar
		self.timer = Utils.Timer()

		for g in self.groups:
			for tg in g:
				try:
					f = tg.post
				except AttributeError:
					pass
				else:
					f()

		try:
			# force the cache initialization
			self.get_tgen_by_name('')
		except Exception:
			pass
		lst = list(self.task_gen_cache_names.keys())
		lst.sort()
		for k in lst:
			Logs.pprint('GREEN', k)

class StepContext(BuildContext):
	'''executes tasks in a step-by-step fashion, for debugging'''
	cmd = 'step'

	def __init__(self, **kw):
		super(StepContext, self).__init__(**kw)
		self.files = Options.options.files

	def compile(self):
		"""
		Compile the tasks matching the input/output files given (regular expression matching). Derived from :py:meth:`waflib.Build.BuildContext.compile`::

			$ waf step --files=foo.c,bar.c,in:truc.c,out:bar.o
			$ waf step --files=in:foo.cpp.1.o # link task only

		"""
		if not self.files:
			Logs.warn('Add a pattern for the debug build, for example "waf step --files=main.c,app"')
			BuildContext.compile(self)
			return

		targets = None
		if self.targets and self.targets != '*':
			targets = self.targets.split(',')

		for g in self.groups:
			for tg in g:
				if targets and tg.name not in targets:
					continue

				try:
					f = tg.post
				except AttributeError:
					pass
				else:
					f()

			for pat in self.files.split(','):
				matcher = self.get_matcher(pat)
				for tg in g:
					if isinstance(tg, Task.TaskBase):
						lst = [tg]
					else:
						lst = tg.tasks
					for tsk in lst:
						do_exec = False
						for node in getattr(tsk, 'inputs', []):
							if matcher(node, output=False):
								do_exec = True
								break
						for node in getattr(tsk, 'outputs', []):
							if matcher(node, output=True):
								do_exec = True
								break
						if do_exec:
							ret = tsk.run()
							Logs.info('%s -> exit %r' % (str(tsk), ret))

	def get_matcher(self, pat):
		# this returns a function
		inn = True
		out = True
		if pat.startswith('in:'):
			out = False
			pat = pat.replace('in:', '')
		elif pat.startswith('out:'):
			inn = False
			pat = pat.replace('out:', '')

		anode = self.root.find_node(pat)
		pattern = None
		if not anode:
			if not pat.startswith('^'):
				pat = '^.+?%s' % pat
			if not pat.endswith('$'):
				pat = '%s$' % pat
			pattern = re.compile(pat)

		def match(node, output):
			if output == True and not out:
				return False
			if output == False and not inn:
				return False

			if anode:
				return anode == node
			else:
				return pattern.match(node.abspath())
		return match

