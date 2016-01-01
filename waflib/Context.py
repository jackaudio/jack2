#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2010 (ita)

"""
Classes and functions required for waf commands
"""

import os, re, imp, sys
from waflib import Utils, Errors, Logs
import waflib.Node

# the following 3 constants are updated on each new release (do not touch)
HEXVERSION=0x1081100
"""Constant updated on new releases"""

WAFVERSION="1.8.17"
"""Constant updated on new releases"""

WAFREVISION="cd7579a727d1b390bf9cbf111c1b20e811370bc0"
"""Git revision when the waf version is updated"""

ABI = 98
"""Version of the build data cache file format (used in :py:const:`waflib.Context.DBFILE`)"""

DBFILE = '.wafpickle-%s-%d-%d' % (sys.platform, sys.hexversion, ABI)
"""Name of the pickle file for storing the build data"""

APPNAME = 'APPNAME'
"""Default application name (used by ``waf dist``)"""

VERSION = 'VERSION'
"""Default application version (used by ``waf dist``)"""

TOP  = 'top'
"""The variable name for the top-level directory in wscript files"""

OUT  = 'out'
"""The variable name for the output directory in wscript files"""

WSCRIPT_FILE = 'wscript'
"""Name of the waf script files"""


launch_dir = ''
"""Directory from which waf has been called"""
run_dir = ''
"""Location of the wscript file to use as the entry point"""
top_dir = ''
"""Location of the project directory (top), if the project was configured"""
out_dir = ''
"""Location of the build directory (out), if the project was configured"""
waf_dir = ''
"""Directory containing the waf modules"""

local_repo = ''
"""Local repository containing additional Waf tools (plugins)"""
remote_repo = 'https://raw.githubusercontent.com/waf-project/waf/master/'
"""
Remote directory containing downloadable waf tools. The missing tools can be downloaded by using::

	$ waf configure --download
"""

remote_locs = ['waflib/extras', 'waflib/Tools']
"""
Remote directories for use with :py:const:`waflib.Context.remote_repo`
"""

g_module = None
"""
Module representing the main wscript file (see :py:const:`waflib.Context.run_dir`)
"""

STDOUT = 1
STDERR = -1
BOTH   = 0

classes = []
"""
List of :py:class:`waflib.Context.Context` subclasses that can be used as waf commands. The classes
are added automatically by a metaclass.
"""


def create_context(cmd_name, *k, **kw):
	"""
	Create a new :py:class:`waflib.Context.Context` instance corresponding to the given command.
	Used in particular by :py:func:`waflib.Scripting.run_command`

	:param cmd_name: command
	:type cmd_name: string
	:param k: arguments to give to the context class initializer
	:type k: list
	:param k: keyword arguments to give to the context class initializer
	:type k: dict
	"""
	global classes
	for x in classes:
		if x.cmd == cmd_name:
			return x(*k, **kw)
	ctx = Context(*k, **kw)
	ctx.fun = cmd_name
	return ctx

class store_context(type):
	"""
	Metaclass for storing the command classes into the list :py:const:`waflib.Context.classes`
	Context classes must provide an attribute 'cmd' representing the command to execute
	"""
	def __init__(cls, name, bases, dict):
		super(store_context, cls).__init__(name, bases, dict)
		name = cls.__name__

		if name == 'ctx' or name == 'Context':
			return

		try:
			cls.cmd
		except AttributeError:
			raise Errors.WafError('Missing command for the context class %r (cmd)' % name)

		if not getattr(cls, 'fun', None):
			cls.fun = cls.cmd

		global classes
		classes.insert(0, cls)

ctx = store_context('ctx', (object,), {})
"""Base class for the :py:class:`waflib.Context.Context` classes"""

class Context(ctx):
	"""
	Default context for waf commands, and base class for new command contexts.

	Context objects are passed to top-level functions::

		def foo(ctx):
			print(ctx.__class__.__name__) # waflib.Context.Context

	Subclasses must define the attribute 'cmd':

	:param cmd: command to execute as in ``waf cmd``
	:type cmd: string
	:param fun: function name to execute when the command is called
	:type fun: string

	.. inheritance-diagram:: waflib.Context.Context waflib.Build.BuildContext waflib.Build.InstallContext waflib.Build.UninstallContext waflib.Build.StepContext waflib.Build.ListContext waflib.Configure.ConfigurationContext waflib.Scripting.Dist waflib.Scripting.DistCheck waflib.Build.CleanContext

	"""

	errors = Errors
	"""
	Shortcut to :py:mod:`waflib.Errors` provided for convenience
	"""

	tools = {}
	"""
	A cache for modules (wscript files) read by :py:meth:`Context.Context.load`
	"""

	def __init__(self, **kw):
		try:
			rd = kw['run_dir']
		except KeyError:
			global run_dir
			rd = run_dir

		# binds the context to the nodes in use to avoid a context singleton
		self.node_class = type("Nod3", (waflib.Node.Node,), {})
		self.node_class.__module__ = "waflib.Node"
		self.node_class.ctx = self

		self.root = self.node_class('', None)
		self.cur_script = None
		self.path = self.root.find_dir(rd)

		self.stack_path = []
		self.exec_dict = {'ctx':self, 'conf':self, 'bld':self, 'opt':self}
		self.logger = None

	def __hash__(self):
		"""
		Return a hash value for storing context objects in dicts or sets. The value is not persistent.

		:return: hash value
		:rtype: int
		"""
		return id(self)

	def finalize(self):
		"""
		Use to free resources such as open files potentially held by the logger
		"""
		try:
			logger = self.logger
		except AttributeError:
			pass
		else:
			Logs.free_logger(logger)
			delattr(self, 'logger')

	def load(self, tool_list, *k, **kw):
		"""
		Load a Waf tool as a module, and try calling the function named :py:const:`waflib.Context.Context.fun` from it.
		A ``tooldir`` value may be provided as a list of module paths.

		:type tool_list: list of string or space-separated string
		:param tool_list: list of Waf tools to use
		"""
		tools = Utils.to_list(tool_list)
		path = Utils.to_list(kw.get('tooldir', ''))
		with_sys_path = kw.get('with_sys_path', True)

		for t in tools:
			module = load_tool(t, path, with_sys_path=with_sys_path)
			fun = getattr(module, kw.get('name', self.fun), None)
			if fun:
				fun(self)

	def execute(self):
		"""
		Execute the command. Redefine this method in subclasses.
		"""
		global g_module
		self.recurse([os.path.dirname(g_module.root_path)])

	def pre_recurse(self, node):
		"""
		Method executed immediately before a folder is read by :py:meth:`waflib.Context.Context.recurse`. The node given is set
		as an attribute ``self.cur_script``, and as the current path ``self.path``

		:param node: script
		:type node: :py:class:`waflib.Node.Node`
		"""
		self.stack_path.append(self.cur_script)

		self.cur_script = node
		self.path = node.parent

	def post_recurse(self, node):
		"""
		Restore ``self.cur_script`` and ``self.path`` right after :py:meth:`waflib.Context.Context.recurse` terminates.

		:param node: script
		:type node: :py:class:`waflib.Node.Node`
		"""
		self.cur_script = self.stack_path.pop()
		if self.cur_script:
			self.path = self.cur_script.parent

	def recurse(self, dirs, name=None, mandatory=True, once=True, encoding=None):
		"""
		Run user code from the supplied list of directories.
		The directories can be either absolute, or relative to the directory
		of the wscript file. The methods :py:meth:`waflib.Context.Context.pre_recurse` and :py:meth:`waflib.Context.Context.post_recurse`
		are called immediately before and after a script has been executed.

		:param dirs: List of directories to visit
		:type dirs: list of string or space-separated string
		:param name: Name of function to invoke from the wscript
		:type  name: string
		:param mandatory: whether sub wscript files are required to exist
		:type  mandatory: bool
		:param once: read the script file once for a particular context
		:type once: bool
		"""
		try:
			cache = self.recurse_cache
		except AttributeError:
			cache = self.recurse_cache = {}

		for d in Utils.to_list(dirs):

			if not os.path.isabs(d):
				# absolute paths only
				d = os.path.join(self.path.abspath(), d)

			WSCRIPT     = os.path.join(d, WSCRIPT_FILE)
			WSCRIPT_FUN = WSCRIPT + '_' + (name or self.fun)

			node = self.root.find_node(WSCRIPT_FUN)
			if node and (not once or node not in cache):
				cache[node] = True
				self.pre_recurse(node)
				try:
					function_code = node.read('rU', encoding)
					exec(compile(function_code, node.abspath(), 'exec'), self.exec_dict)
				finally:
					self.post_recurse(node)
			elif not node:
				node = self.root.find_node(WSCRIPT)
				tup = (node, name or self.fun)
				if node and (not once or tup not in cache):
					cache[tup] = True
					self.pre_recurse(node)
					try:
						wscript_module = load_module(node.abspath(), encoding=encoding)
						user_function = getattr(wscript_module, (name or self.fun), None)
						if not user_function:
							if not mandatory:
								continue
							raise Errors.WafError('No function %s defined in %s' % (name or self.fun, node.abspath()))
						user_function(self)
					finally:
						self.post_recurse(node)
				elif not node:
					if not mandatory:
						continue
					try:
						os.listdir(d)
					except OSError:
						raise Errors.WafError('Cannot read the folder %r' % d)
					raise Errors.WafError('No wscript file in directory %s' % d)

	def exec_command(self, cmd, **kw):
		"""
		Execute a command and return the exit status. If the context has the attribute 'log',
		capture and log the process stderr/stdout for logging purposes::

			def run(tsk):
				ret = tsk.generator.bld.exec_command('touch foo.txt')
				return ret

		This method captures the standard/error outputs (Issue 1101), but it does not return the values
		unlike :py:meth:`waflib.Context.Context.cmd_and_log`

		:param cmd: command argument for subprocess.Popen
		:param kw: keyword arguments for subprocess.Popen. The parameters input/timeout will be passed to wait/communicate.
		"""
		subprocess = Utils.subprocess
		kw['shell'] = isinstance(cmd, str)
		Logs.debug('runner: %r' % (cmd,))
		Logs.debug('runner_env: kw=%s' % kw)

		if self.logger:
			self.logger.info(cmd)

		if 'stdout' not in kw:
			kw['stdout'] = subprocess.PIPE
		if 'stderr' not in kw:
			kw['stderr'] = subprocess.PIPE

		if Logs.verbose and not kw['shell'] and not Utils.check_exe(cmd[0]):
			raise Errors.WafError("Program %s not found!" % cmd[0])

		wargs = {}
		if 'timeout' in kw:
			if kw['timeout'] is not None:
				wargs['timeout'] = kw['timeout']
			del kw['timeout']
		if 'input' in kw:
			if kw['input']:
				wargs['input'] = kw['input']
				kw['stdin'] = Utils.subprocess.PIPE
			del kw['input']

		try:
			if kw['stdout'] or kw['stderr']:
				p = subprocess.Popen(cmd, **kw)
				(out, err) = p.communicate(**wargs)
				ret = p.returncode
			else:
				out, err = (None, None)
				ret = subprocess.Popen(cmd, **kw).wait(**wargs)
		except Exception as e:
			raise Errors.WafError('Execution failure: %s' % str(e), ex=e)

		if out:
			if not isinstance(out, str):
				out = out.decode(sys.stdout.encoding or 'iso8859-1')
			if self.logger:
				self.logger.debug('out: %s' % out)
			else:
				Logs.info(out, extra={'stream':sys.stdout, 'c1': ''})
		if err:
			if not isinstance(err, str):
				err = err.decode(sys.stdout.encoding or 'iso8859-1')
			if self.logger:
				self.logger.error('err: %s' % err)
			else:
				Logs.info(err, extra={'stream':sys.stderr, 'c1': ''})

		return ret

	def cmd_and_log(self, cmd, **kw):
		"""
		Execute a command and return stdout/stderr if the execution is successful.
		An exception is thrown when the exit status is non-0. In that case, both stderr and stdout
		will be bound to the WafError object::

			def configure(conf):
				out = conf.cmd_and_log(['echo', 'hello'], output=waflib.Context.STDOUT, quiet=waflib.Context.BOTH)
				(out, err) = conf.cmd_and_log(['echo', 'hello'], output=waflib.Context.BOTH)
				(out, err) = conf.cmd_and_log(cmd, input='\\n'.encode(), output=waflib.Context.STDOUT)
				try:
					conf.cmd_and_log(['which', 'someapp'], output=waflib.Context.BOTH)
				except Exception as e:
					print(e.stdout, e.stderr)

		:param cmd: args for subprocess.Popen
		:param kw: keyword arguments for subprocess.Popen. The parameters input/timeout will be passed to wait/communicate.
		"""
		subprocess = Utils.subprocess
		kw['shell'] = isinstance(cmd, str)
		Logs.debug('runner: %r' % (cmd,))

		if 'quiet' in kw:
			quiet = kw['quiet']
			del kw['quiet']
		else:
			quiet = None

		if 'output' in kw:
			to_ret = kw['output']
			del kw['output']
		else:
			to_ret = STDOUT

		if Logs.verbose and not kw['shell'] and not Utils.check_exe(cmd[0]):
			raise Errors.WafError("Program %s not found!" % cmd[0])

		kw['stdout'] = kw['stderr'] = subprocess.PIPE
		if quiet is None:
			self.to_log(cmd)

		wargs = {}
		if 'timeout' in kw:
			if kw['timeout'] is not None:
				wargs['timeout'] = kw['timeout']
			del kw['timeout']
		if 'input' in kw:
			if kw['input']:
				wargs['input'] = kw['input']
				kw['stdin'] = Utils.subprocess.PIPE
			del kw['input']

		try:
			p = subprocess.Popen(cmd, **kw)
			(out, err) = p.communicate(**wargs)
		except Exception as e:
			raise Errors.WafError('Execution failure: %s' % str(e), ex=e)

		if not isinstance(out, str):
			out = out.decode(sys.stdout.encoding or 'iso8859-1')
		if not isinstance(err, str):
			err = err.decode(sys.stdout.encoding or 'iso8859-1')

		if out and quiet != STDOUT and quiet != BOTH:
			self.to_log('out: %s' % out)
		if err and quiet != STDERR and quiet != BOTH:
			self.to_log('err: %s' % err)

		if p.returncode:
			e = Errors.WafError('Command %r returned %r' % (cmd, p.returncode))
			e.returncode = p.returncode
			e.stderr = err
			e.stdout = out
			raise e

		if to_ret == BOTH:
			return (out, err)
		elif to_ret == STDERR:
			return err
		return out

	def fatal(self, msg, ex=None):
		"""
		Raise a configuration error to interrupt the execution immediately::

			def configure(conf):
				conf.fatal('a requirement is missing')

		:param msg: message to display
		:type msg: string
		:param ex: optional exception object
		:type ex: exception
		"""
		if self.logger:
			self.logger.info('from %s: %s' % (self.path.abspath(), msg))
		try:
			msg = '%s\n(complete log in %s)' % (msg, self.logger.handlers[0].baseFilename)
		except Exception:
			pass
		raise self.errors.ConfigurationError(msg, ex=ex)

	def to_log(self, msg):
		"""
		Log some information to the logger (if present), or to stderr. If the message is empty,
		it is not printed::

			def build(bld):
				bld.to_log('starting the build')

		When in doubt, override this method, or provide a logger on the context class.

		:param msg: message
		:type msg: string
		"""
		if not msg:
			return
		if self.logger:
			self.logger.info(msg)
		else:
			sys.stderr.write(str(msg))
			sys.stderr.flush()


	def msg(self, *k, **kw):
		"""
		Print a configuration message of the form ``msg: result``.
		The second part of the message will be in colors. The output
		can be disabled easly by setting ``in_msg`` to a positive value::

			def configure(conf):
				self.in_msg = 1
				conf.msg('Checking for library foo', 'ok')
				# no output

		:param msg: message to display to the user
		:type msg: string
		:param result: result to display
		:type result: string or boolean
		:param color: color to use, see :py:const:`waflib.Logs.colors_lst`
		:type color: string
		"""
		try:
			msg = kw['msg']
		except KeyError:
			msg = k[0]

		self.start_msg(msg, **kw)

		try:
			result = kw['result']
		except KeyError:
			result = k[1]

		color = kw.get('color', None)
		if not isinstance(color, str):
			color = result and 'GREEN' or 'YELLOW'

		self.end_msg(result, color, **kw)

	def start_msg(self, *k, **kw):
		"""
		Print the beginning of a 'Checking for xxx' message. See :py:meth:`waflib.Context.Context.msg`
		"""
		if kw.get('quiet', None):
			return

		msg = kw.get('msg', None) or k[0]
		try:
			if self.in_msg:
				self.in_msg += 1
				return
		except AttributeError:
			self.in_msg = 0
		self.in_msg += 1

		try:
			self.line_just = max(self.line_just, len(msg))
		except AttributeError:
			self.line_just = max(40, len(msg))
		for x in (self.line_just * '-', msg):
			self.to_log(x)
		Logs.pprint('NORMAL', "%s :" % msg.ljust(self.line_just), sep='')

	def end_msg(self, *k, **kw):
		"""Print the end of a 'Checking for' message. See :py:meth:`waflib.Context.Context.msg`"""
		if kw.get('quiet', None):
			return
		self.in_msg -= 1
		if self.in_msg:
			return

		result = kw.get('result', None) or k[0]

		defcolor = 'GREEN'
		if result == True:
			msg = 'ok'
		elif result == False:
			msg = 'not found'
			defcolor = 'YELLOW'
		else:
			msg = str(result)

		self.to_log(msg)
		try:
			color = kw['color']
		except KeyError:
			if len(k) > 1 and k[1] in Logs.colors_lst:
				# compatibility waf 1.7
				color = k[1]
			else:
				color = defcolor
		Logs.pprint(color, msg)

	def load_special_tools(self, var, ban=[]):
		global waf_dir
		if os.path.isdir(waf_dir):
			lst = self.root.find_node(waf_dir).find_node('waflib/extras').ant_glob(var)
			for x in lst:
				if not x.name in ban:
					load_tool(x.name.replace('.py', ''))
		else:
			from zipfile import PyZipFile
			waflibs = PyZipFile(waf_dir)
			lst = waflibs.namelist()
			for x in lst:
				if not re.match("waflib/extras/%s" % var.replace("*", ".*"), var):
					continue
				f = os.path.basename(x)
				doban = False
				for b in ban:
					r = b.replace("*", ".*")
					if re.match(r, f):
						doban = True
				if not doban:
					f = f.replace('.py', '')
					load_tool(f)

cache_modules = {}
"""
Dictionary holding already loaded modules, keyed by their absolute path.
The modules are added automatically by :py:func:`waflib.Context.load_module`
"""

def load_module(path, encoding=None):
	"""
	Load a source file as a python module.

	:param path: file path
	:type path: string
	:return: Loaded Python module
	:rtype: module
	"""
	try:
		return cache_modules[path]
	except KeyError:
		pass

	module = imp.new_module(WSCRIPT_FILE)
	try:
		code = Utils.readf(path, m='rU', encoding=encoding)
	except EnvironmentError:
		raise Errors.WafError('Could not read the file %r' % path)

	module_dir = os.path.dirname(path)
	sys.path.insert(0, module_dir)

	try    : exec(compile(code, path, 'exec'), module.__dict__)
	finally: sys.path.remove(module_dir)

	cache_modules[path] = module

	return module

def load_tool(tool, tooldir=None, ctx=None, with_sys_path=True):
	"""
	Import a Waf tool (python module), and store it in the dict :py:const:`waflib.Context.Context.tools`

	:type  tool: string
	:param tool: Name of the tool
	:type  tooldir: list
	:param tooldir: List of directories to search for the tool module
	:type  with_sys_path: boolean
	:param with_sys_path: whether or not to search the regular sys.path, besides waf_dir and potentially given tooldirs
	"""
	if tool == 'java':
		tool = 'javaw' # jython
	else:
		tool = tool.replace('++', 'xx')

	origSysPath = sys.path
	if not with_sys_path: sys.path = []
	try:
		if tooldir:
			assert isinstance(tooldir, list)
			sys.path = tooldir + sys.path
			try:
				__import__(tool)
			finally:
				for d in tooldir:
					sys.path.remove(d)
			ret = sys.modules[tool]
			Context.tools[tool] = ret
			return ret
		else:
			if not with_sys_path: sys.path.insert(0, waf_dir)
			try:
				for x in ('waflib.Tools.%s', 'waflib.extras.%s', 'waflib.%s', '%s'):
					try:
						__import__(x % tool)
						break
					except ImportError:
						x = None
				if x is None: # raise an exception
					__import__(tool)
			finally:
				if not with_sys_path: sys.path.remove(waf_dir)
			ret = sys.modules[x % tool]
			Context.tools[tool] = ret
			return ret
	finally:
		if not with_sys_path: sys.path += origSysPath

