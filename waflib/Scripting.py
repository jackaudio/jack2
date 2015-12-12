#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2005-2010 (ita)

"Module called for configuring, compiling and installing targets"

import os, shlex, shutil, traceback, errno, sys, stat
from waflib import Utils, Configure, Logs, Options, ConfigSet, Context, Errors, Build, Node

build_dir_override = None

no_climb_commands = ['configure']

default_cmd = "build"

def waf_entry_point(current_directory, version, wafdir):
	"""
	This is the main entry point, all Waf execution starts here.

	:param current_directory: absolute path representing the current directory
	:type current_directory: string
	:param version: version number
	:type version: string
	:param wafdir: absolute path representing the directory of the waf library
	:type wafdir: string
	"""

	Logs.init_log()

	if Context.WAFVERSION != version:
		Logs.error('Waf script %r and library %r do not match (directory %r)' % (version, Context.WAFVERSION, wafdir))
		sys.exit(1)

	if '--version' in sys.argv:
		Context.run_dir = current_directory
		ctx = Context.create_context('options')
		ctx.curdir = current_directory
		ctx.parse_args()
		sys.exit(0)

	if len(sys.argv) > 1:
		# os.path.join handles absolute paths in sys.argv[1] accordingly (it discards the previous ones)
		# if sys.argv[1] is not an absolute path, then it is relative to the current working directory
		potential_wscript = os.path.join(current_directory, sys.argv[1])
		# maybe check if the file is executable
		# perhaps extract 'wscript' as a constant
		if os.path.basename(potential_wscript) == 'wscript' and os.path.isfile(potential_wscript):
			# need to explicitly normalize the path, as it may contain extra '/.'
			# TODO abspath?
			current_directory = os.path.normpath(os.path.dirname(potential_wscript))
			sys.argv.pop(1)

	Context.waf_dir = wafdir
	Context.launch_dir = current_directory

	# if 'configure' is in the commands, do not search any further
	no_climb = os.environ.get('NOCLIMB', None)
	if not no_climb:
		for k in no_climb_commands:
			for y in sys.argv:
				if y.startswith(k):
					no_climb = True
					break

	# if --top is provided assume the build started in the top directory
	for i, x in enumerate(sys.argv):
		# WARNING: this modifies sys.argv
		if x.startswith('--top='):
			Context.run_dir = Context.top_dir = Utils.sane_path(x[6:])
			sys.argv[i] = '--top=' + Context.run_dir
		if x.startswith('--out='):
			Context.out_dir = Utils.sane_path(x[6:])
			sys.argv[i] = '--out=' + Context.out_dir

	# try to find a lock file (if the project was configured)
	# at the same time, store the first wscript file seen
	cur = current_directory
	while cur and not Context.top_dir:
		lst = os.listdir(cur)
		if Options.lockfile in lst:
			env = ConfigSet.ConfigSet()
			try:
				env.load(os.path.join(cur, Options.lockfile))
				ino = os.stat(cur)[stat.ST_INO]
			except Exception:
				pass
			else:
				# check if the folder was not moved
				for x in (env.run_dir, env.top_dir, env.out_dir):
					if Utils.is_win32:
						if cur == x:
							load = True
							break
					else:
						# if the filesystem features symlinks, compare the inode numbers
						try:
							ino2 = os.stat(x)[stat.ST_INO]
						except OSError:
							pass
						else:
							if ino == ino2:
								load = True
								break
				else:
					Logs.warn('invalid lock file in %s' % cur)
					load = False

				if load:
					Context.run_dir = env.run_dir
					Context.top_dir = env.top_dir
					Context.out_dir = env.out_dir
					break

		if not Context.run_dir:
			if Context.WSCRIPT_FILE in lst:
				Context.run_dir = cur

		next = os.path.dirname(cur)
		if next == cur:
			break
		cur = next

		if no_climb:
			break

	if not Context.run_dir:
		if '-h' in sys.argv or '--help' in sys.argv:
			Logs.warn('No wscript file found: the help message may be incomplete')
			Context.run_dir = current_directory
			ctx = Context.create_context('options')
			ctx.curdir = current_directory
			ctx.parse_args()
			sys.exit(0)
		Logs.error('Waf: Run from a directory containing a file named %r' % Context.WSCRIPT_FILE)
		sys.exit(1)

	try:
		os.chdir(Context.run_dir)
	except OSError:
		Logs.error('Waf: The folder %r is unreadable' % Context.run_dir)
		sys.exit(1)

	try:
		set_main_module(os.path.normpath(os.path.join(Context.run_dir, Context.WSCRIPT_FILE)))
	except Errors.WafError as e:
		Logs.pprint('RED', e.verbose_msg)
		Logs.error(str(e))
		sys.exit(1)
	except Exception as e:
		Logs.error('Waf: The wscript in %r is unreadable' % Context.run_dir, e)
		traceback.print_exc(file=sys.stdout)
		sys.exit(2)

	"""
	import cProfile, pstats
	cProfile.runctx("from waflib import Scripting; Scripting.run_commands()", {}, {}, 'profi.txt')
	p = pstats.Stats('profi.txt')
	p.sort_stats('time').print_stats(75) # or 'cumulative'
	"""
	try:
		run_commands()
	except Errors.WafError as e:
		if Logs.verbose > 1:
			Logs.pprint('RED', e.verbose_msg)
		Logs.error(e.msg)
		sys.exit(1)
	except SystemExit:
		raise
	except Exception as e:
		traceback.print_exc(file=sys.stdout)
		sys.exit(2)
	except KeyboardInterrupt:
		Logs.pprint('RED', 'Interrupted')
		sys.exit(68)
	#"""

def set_main_module(file_path):
	"""
	Read the main wscript file into :py:const:`waflib.Context.Context.g_module` and
	bind default functions such as ``init``, ``dist``, ``distclean`` if not defined.
	Called by :py:func:`waflib.Scripting.waf_entry_point` during the initialization.

	:param file_path: absolute path representing the top-level wscript file
	:type file_path: string
	"""
	Context.g_module = Context.load_module(file_path)
	Context.g_module.root_path = file_path

	# note: to register the module globally, use the following:
	# sys.modules['wscript_main'] = g_module

	def set_def(obj):
		name = obj.__name__
		if not name in Context.g_module.__dict__:
			setattr(Context.g_module, name, obj)
	for k in (update, dist, distclean, distcheck, update):
		set_def(k)
	# add dummy init and shutdown functions if they're not defined
	if not 'init' in Context.g_module.__dict__:
		Context.g_module.init = Utils.nada
	if not 'shutdown' in Context.g_module.__dict__:
		Context.g_module.shutdown = Utils.nada
	if not 'options' in Context.g_module.__dict__:
		Context.g_module.options = Utils.nada

def parse_options():
	"""
	Parse the command-line options and initialize the logging system.
	Called by :py:func:`waflib.Scripting.waf_entry_point` during the initialization.
	"""
	Context.create_context('options').execute()

	for var in Options.envvars:
		(name, value) = var.split('=', 1)
		os.environ[name.strip()] = value

	if not Options.commands:
		Options.commands = [default_cmd]
	Options.commands = [x for x in Options.commands if x != 'options'] # issue 1076

	# process some internal Waf options
	Logs.verbose = Options.options.verbose
	#Logs.init_log()

	if Options.options.zones:
		Logs.zones = Options.options.zones.split(',')
		if not Logs.verbose:
			Logs.verbose = 1
	elif Logs.verbose > 0:
		Logs.zones = ['runner']

	if Logs.verbose > 2:
		Logs.zones = ['*']

def run_command(cmd_name):
	"""
	Execute a single command. Called by :py:func:`waflib.Scripting.run_commands`.

	:param cmd_name: command to execute, like ``build``
	:type cmd_name: string
	"""
	ctx = Context.create_context(cmd_name)
	ctx.log_timer = Utils.Timer()
	ctx.options = Options.options # provided for convenience
	ctx.cmd = cmd_name
	try:
		ctx.execute()
	finally:
		# Issue 1374
		ctx.finalize()
	return ctx

def run_commands():
	"""
	Execute the commands that were given on the command-line, and the other options
	Called by :py:func:`waflib.Scripting.waf_entry_point` during the initialization, and executed
	after :py:func:`waflib.Scripting.parse_options`.
	"""
	parse_options()
	run_command('init')
	while Options.commands:
		cmd_name = Options.commands.pop(0)
		ctx = run_command(cmd_name)
		Logs.info('%r finished successfully (%s)' % (cmd_name, str(ctx.log_timer)))
	run_command('shutdown')

###########################################################################################

def _can_distclean(name):
	# WARNING: this method may disappear anytime
	for k in '.o .moc .exe'.split():
		if name.endswith(k):
			return True
	return False

def distclean_dir(dirname):
	"""
	Distclean function called in the particular case when::

		top == out

	:param dirname: absolute path of the folder to clean
	:type dirname: string
	"""
	for (root, dirs, files) in os.walk(dirname):
		for f in files:
			if _can_distclean(f):
				fname = os.path.join(root, f)
				try:
					os.remove(fname)
				except OSError:
					Logs.warn('Could not remove %r' % fname)

	for x in (Context.DBFILE, 'config.log'):
		try:
			os.remove(x)
		except OSError:
			pass

	try:
		shutil.rmtree('c4che')
	except OSError:
		pass

def distclean(ctx):
	'''removes the build directory'''
	lst = os.listdir('.')
	for f in lst:
		if f == Options.lockfile:
			try:
				proj = ConfigSet.ConfigSet(f)
			except IOError:
				Logs.warn('Could not read %r' % f)
				continue

			if proj['out_dir'] != proj['top_dir']:
				try:
					shutil.rmtree(proj['out_dir'])
				except IOError:
					pass
				except OSError as e:
					if e.errno != errno.ENOENT:
						Logs.warn('Could not remove %r' % proj['out_dir'])
			else:
				distclean_dir(proj['out_dir'])

			for k in (proj['out_dir'], proj['top_dir'], proj['run_dir']):
				p = os.path.join(k, Options.lockfile)
				try:
					os.remove(p)
				except OSError as e:
					if e.errno != errno.ENOENT:
						Logs.warn('Could not remove %r' % p)

		# remove local waf cache folders
		if not Options.commands:
			for x in '.waf-1. waf-1. .waf3-1. waf3-1.'.split():
				if f.startswith(x):
					shutil.rmtree(f, ignore_errors=True)

class Dist(Context.Context):
	'''creates an archive containing the project source code'''
	cmd = 'dist'
	fun = 'dist'
	algo = 'tar.bz2'
	ext_algo = {}

	def execute(self):
		"""
		See :py:func:`waflib.Context.Context.execute`
		"""
		self.recurse([os.path.dirname(Context.g_module.root_path)])
		self.archive()

	def archive(self):
		"""
		Create the archive.
		"""
		import tarfile

		arch_name = self.get_arch_name()

		try:
			self.base_path
		except AttributeError:
			self.base_path = self.path

		node = self.base_path.make_node(arch_name)
		try:
			node.delete()
		except OSError:
			pass

		files = self.get_files()

		if self.algo.startswith('tar.'):
			tar = tarfile.open(arch_name, 'w:' + self.algo.replace('tar.', ''))

			for x in files:
				self.add_tar_file(x, tar)
			tar.close()
		elif self.algo == 'zip':
			import zipfile
			zip = zipfile.ZipFile(arch_name, 'w', compression=zipfile.ZIP_DEFLATED)

			for x in files:
				archive_name = self.get_base_name() + '/' + x.path_from(self.base_path)
				zip.write(x.abspath(), archive_name, zipfile.ZIP_DEFLATED)
			zip.close()
		else:
			self.fatal('Valid algo types are tar.bz2, tar.gz, tar.xz or zip')

		try:
			from hashlib import sha1 as sha
		except ImportError:
			from sha import sha
		try:
			digest = " (sha=%r)" % sha(node.read()).hexdigest()
		except Exception:
			digest = ''

		Logs.info('New archive created: %s%s' % (self.arch_name, digest))

	def get_tar_path(self, node):
		"""
		return the path to use for a node in the tar archive, the purpose of this
		is to let subclases resolve symbolic links or to change file names
		"""
		return node.abspath()

	def add_tar_file(self, x, tar):
		"""
		Add a file to the tar archive. Transform symlinks into files if the files lie out of the project tree.
		"""
		p = self.get_tar_path(x)
		tinfo = tar.gettarinfo(name=p, arcname=self.get_tar_prefix() + '/' + x.path_from(self.base_path))
		tinfo.uid   = 0
		tinfo.gid   = 0
		tinfo.uname = 'root'
		tinfo.gname = 'root'

		fu = None
		try:
			fu = open(p, 'rb')
			tar.addfile(tinfo, fileobj=fu)
		finally:
			if fu:
				fu.close()

	def get_tar_prefix(self):
		try:
			return self.tar_prefix
		except AttributeError:
			return self.get_base_name()

	def get_arch_name(self):
		"""
		Return the name of the archive to create. Change the default value by setting *arch_name*::

			def dist(ctx):
				ctx.arch_name = 'ctx.tar.bz2'

		:rtype: string
		"""
		try:
			self.arch_name
		except AttributeError:
			self.arch_name = self.get_base_name() + '.' + self.ext_algo.get(self.algo, self.algo)
		return self.arch_name

	def get_base_name(self):
		"""
		Return the default name of the main directory in the archive, which is set to *appname-version*.
		Set the attribute *base_name* to change the default value::

			def dist(ctx):
				ctx.base_name = 'files'

		:rtype: string
		"""
		try:
			self.base_name
		except AttributeError:
			appname = getattr(Context.g_module, Context.APPNAME, 'noname')
			version = getattr(Context.g_module, Context.VERSION, '1.0')
			self.base_name = appname + '-' + version
		return self.base_name

	def get_excl(self):
		"""
		Return the patterns to exclude for finding the files in the top-level directory. Set the attribute *excl*
		to change the default value::

			def dist(ctx):
				ctx.excl = 'build **/*.o **/*.class'

		:rtype: string
		"""
		try:
			return self.excl
		except AttributeError:
			self.excl = Node.exclude_regs + ' **/waf-1.8.* **/.waf-1.8* **/waf3-1.8.* **/.waf3-1.8* **/*~ **/*.rej **/*.orig **/*.pyc **/*.pyo **/*.bak **/*.swp **/.lock-w*'
			if Context.out_dir:
				nd = self.root.find_node(Context.out_dir)
				if nd:
					self.excl += ' ' + nd.path_from(self.base_path)
			return self.excl

	def get_files(self):
		"""
		The files to package are searched automatically by :py:func:`waflib.Node.Node.ant_glob`. Set
		*files* to prevent this behaviour::

			def dist(ctx):
				ctx.files = ctx.path.find_node('wscript')

		The files are searched from the directory 'base_path', to change it, set::

			def dist(ctx):
				ctx.base_path = path

		:rtype: list of :py:class:`waflib.Node.Node`
		"""
		try:
			files = self.files
		except AttributeError:
			files = self.base_path.ant_glob('**/*', excl=self.get_excl())
		return files


def dist(ctx):
	'''makes a tarball for redistributing the sources'''
	pass

class DistCheck(Dist):
	"""
	Create an archive of the project, and try to build the project in a temporary directory::

		$ waf distcheck
	"""

	fun = 'distcheck'
	cmd = 'distcheck'

	def execute(self):
		"""
		See :py:func:`waflib.Context.Context.execute`
		"""
		self.recurse([os.path.dirname(Context.g_module.root_path)])
		self.archive()
		self.check()

	def check(self):
		"""
		Create the archive, uncompress it and try to build the project
		"""
		import tempfile, tarfile

		t = None
		try:
			t = tarfile.open(self.get_arch_name())
			for x in t:
				t.extract(x)
		finally:
			if t:
				t.close()

		cfg = []

		if Options.options.distcheck_args:
			cfg = shlex.split(Options.options.distcheck_args)
		else:
			cfg = [x for x in sys.argv if x.startswith('-')]

		instdir = tempfile.mkdtemp('.inst', self.get_base_name())
		ret = Utils.subprocess.Popen([sys.executable, sys.argv[0], 'configure', 'install', 'uninstall', '--destdir=' + instdir] + cfg, cwd=self.get_base_name()).wait()
		if ret:
			raise Errors.WafError('distcheck failed with code %i' % ret)

		if os.path.exists(instdir):
			raise Errors.WafError('distcheck succeeded, but files were left in %s' % instdir)

		shutil.rmtree(self.get_base_name())


def distcheck(ctx):
	'''checks if the project compiles (tarball from 'dist')'''
	pass

def update(ctx):
	lst = Options.options.files
	if lst:
		lst = lst.split(',')
	else:
		path = os.path.join(Context.waf_dir, 'waflib', 'extras')
		lst = [x for x in Utils.listdir(path) if x.endswith('.py')]
	for x in lst:
		tool = x.replace('.py', '')
		if not tool:
			continue
		try:
			dl = Configure.download_tool
		except AttributeError:
			ctx.fatal('The command "update" is dangerous; include the tool "use_config" in your project!')
		try:
			dl(tool, force=True, ctx=ctx)
		except Errors.WafError:
			Logs.error('Could not find the tool %r in the remote repository' % x)
		else:
			Logs.warn('Updated %r' % tool)

def autoconfigure(execute_method):
	"""
	Decorator used to set the commands that can be configured automatically
	"""
	def execute(self):
		if not Configure.autoconfig:
			return execute_method(self)

		env = ConfigSet.ConfigSet()
		do_config = False
		try:
			env.load(os.path.join(Context.top_dir, Options.lockfile))
		except Exception:
			Logs.warn('Configuring the project')
			do_config = True
		else:
			if env.run_dir != Context.run_dir:
				do_config = True
			else:
				h = 0
				for f in env['files']:
					h = Utils.h_list((h, Utils.readf(f, 'rb')))
				do_config = h != env.hash

		if do_config:
			Options.commands.insert(0, self.cmd)
			Options.commands.insert(0, 'configure')
			if Configure.autoconfig == 'clobber':
				Options.options.__dict__ = env.options
			return

		return execute_method(self)
	return execute
Build.BuildContext.execute = autoconfigure(Build.BuildContext.execute)

