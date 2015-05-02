#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2006-2010 (ita)

"""
C# support. A simple example::

	def configure(conf):
		conf.load('cs')
	def build(bld):
		bld(features='cs', source='main.cs', gen='foo')

Note that the configuration may compile C# snippets::

	FRAG = '''
	namespace Moo {
		public class Test { public static int Main(string[] args) { return 0; } }
	}'''
	def configure(conf):
		conf.check(features='cs', fragment=FRAG, compile_filename='test.cs', gen='test.exe',
			bintype='exe', csflags=['-pkg:gtk-sharp-2.0'], msg='Checking for Gtksharp support')
"""

from waflib import Utils, Task, Options, Logs, Errors
from waflib.TaskGen import before_method, after_method, feature
from waflib.Tools import ccroot
from waflib.Configure import conf
import os, tempfile

ccroot.USELIB_VARS['cs'] = set(['CSFLAGS', 'ASSEMBLIES', 'RESOURCES'])
ccroot.lib_patterns['csshlib'] = ['%s']

@feature('cs')
@before_method('process_source')
def apply_cs(self):
	"""
	Create a C# task bound to the attribute *cs_task*. There can be only one C# task by task generator.
	"""
	cs_nodes = []
	no_nodes = []
	for x in self.to_nodes(self.source):
		if x.name.endswith('.cs'):
			cs_nodes.append(x)
		else:
			no_nodes.append(x)
	self.source = no_nodes

	bintype = getattr(self, 'bintype', self.gen.endswith('.dll') and 'library' or 'exe')
	self.cs_task = tsk = self.create_task('mcs', cs_nodes, self.path.find_or_declare(self.gen))
	tsk.env.CSTYPE = '/target:%s' % bintype
	tsk.env.OUT = '/out:%s' % tsk.outputs[0].abspath()
	self.env.append_value('CSFLAGS', '/platform:%s' % getattr(self, 'platform', 'anycpu'))

	inst_to = getattr(self, 'install_path', bintype=='exe' and '${BINDIR}' or '${LIBDIR}')
	if inst_to:
		# note: we are making a copy, so the files added to cs_task.outputs won't be installed automatically
		mod = getattr(self, 'chmod', bintype=='exe' and Utils.O755 or Utils.O644)
		self.install_task = self.bld.install_files(inst_to, self.cs_task.outputs[:], env=self.env, chmod=mod)

@feature('cs')
@after_method('apply_cs')
def use_cs(self):
	"""
	C# applications honor the **use** keyword::

		def build(bld):
			bld(features='cs', source='My.cs', bintype='library', gen='my.dll', name='mylib')
			bld(features='cs', source='Hi.cs', includes='.', bintype='exe', gen='hi.exe', use='mylib', name='hi')
	"""
	names = self.to_list(getattr(self, 'use', []))
	get = self.bld.get_tgen_by_name
	for x in names:
		try:
			y = get(x)
		except Errors.WafError:
			self.env.append_value('CSFLAGS', '/reference:%s' % x)
			continue
		y.post()

		tsk = getattr(y, 'cs_task', None) or getattr(y, 'link_task', None)
		if not tsk:
			self.bld.fatal('cs task has no link task for use %r' % self)
		self.cs_task.dep_nodes.extend(tsk.outputs) # dependency
		self.cs_task.set_run_after(tsk) # order (redundant, the order is infered from the nodes inputs/outputs)
		self.env.append_value('CSFLAGS', '/reference:%s' % tsk.outputs[0].abspath())

@feature('cs')
@after_method('apply_cs', 'use_cs')
def debug_cs(self):
	"""
	The C# targets may create .mdb or .pdb files::

		def build(bld):
			bld(features='cs', source='My.cs', bintype='library', gen='my.dll', csdebug='full')
			# csdebug is a value in (True, 'full', 'pdbonly')
	"""
	csdebug = getattr(self, 'csdebug', self.env.CSDEBUG)
	if not csdebug:
		return

	node = self.cs_task.outputs[0]
	if self.env.CS_NAME == 'mono':
		out = node.parent.find_or_declare(node.name + '.mdb')
	else:
		out = node.change_ext('.pdb')
	self.cs_task.outputs.append(out)
	try:
		self.install_task.source.append(out)
	except AttributeError:
		pass

	if csdebug == 'pdbonly':
		val = ['/debug+', '/debug:pdbonly']
	elif csdebug == 'full':
		val = ['/debug+', '/debug:full']
	else:
		val = ['/debug-']
	self.env.append_value('CSFLAGS', val)


class mcs(Task.Task):
	"""
	Compile C# files
	"""
	color   = 'YELLOW'
	run_str = '${MCS} ${CSTYPE} ${CSFLAGS} ${ASS_ST:ASSEMBLIES} ${RES_ST:RESOURCES} ${OUT} ${SRC}'

	def exec_command(self, cmd, **kw):
		bld = self.generator.bld

		try:
			if not kw.get('cwd', None):
				kw['cwd'] = bld.cwd
		except AttributeError:
			bld.cwd = kw['cwd'] = bld.variant_dir

		try:
			tmp = None
			if isinstance(cmd, list) and len(' '.join(cmd)) >= 8192:
				program = cmd[0] #unquoted program name, otherwise exec_command will fail
				cmd = [self.quote_response_command(x) for x in cmd]
				(fd, tmp) = tempfile.mkstemp()
				os.write(fd, '\r\n'.join(i.replace('\\', '\\\\') for i in cmd[1:]).encode())
				os.close(fd)
				cmd = [program, '@' + tmp]
			# no return here, that's on purpose
			ret = self.generator.bld.exec_command(cmd, **kw)
		finally:
			if tmp:
				try:
					os.remove(tmp)
				except OSError:
					pass # anti-virus and indexers can keep the files open -_-
		return ret

	def quote_response_command(self, flag):
		# /noconfig is not allowed when using response files
		if flag.lower() == '/noconfig':
			return ''

		if flag.find(' ') > -1:
			for x in ('/r:', '/reference:', '/resource:', '/lib:', '/out:'):
				if flag.startswith(x):
					flag = '%s"%s"' % (x, '","'.join(flag[len(x):].split(',')))
					break
			else:
				flag = '"%s"' % flag
		return flag

def configure(conf):
	"""
	Find a C# compiler, set the variable MCS for the compiler and CS_NAME (mono or csc)
	"""
	csc = getattr(Options.options, 'cscbinary', None)
	if csc:
		conf.env.MCS = csc
	conf.find_program(['csc', 'mcs', 'gmcs'], var='MCS')
	conf.env.ASS_ST = '/r:%s'
	conf.env.RES_ST = '/resource:%s'

	conf.env.CS_NAME = 'csc'
	if str(conf.env.MCS).lower().find('mcs') > -1:
		conf.env.CS_NAME = 'mono'

def options(opt):
	"""
	Add a command-line option for the configuration::

		$ waf configure --with-csc-binary=/foo/bar/mcs
	"""
	opt.add_option('--with-csc-binary', type='string', dest='cscbinary')

class fake_csshlib(Task.Task):
	"""
	Task used for reading a foreign .net assembly and adding the dependency on it
	"""
	color   = 'YELLOW'
	inst_to = None

	def runnable_status(self):
		for x in self.outputs:
			x.sig = Utils.h_file(x.abspath())
		return Task.SKIP_ME

@conf
def read_csshlib(self, name, paths=[]):
	"""
	Read a foreign .net assembly for the *use* system::

		def build(bld):
			bld.read_csshlib('ManagedLibrary.dll', paths=[bld.env.mylibrarypath])
			bld(features='cs', source='Hi.cs', bintype='exe', gen='hi.exe', use='ManagedLibrary.dll')

	:param name: Name of the library
	:type name: string
	:param paths: Folders in which the library may be found
	:type paths: list of string
	:return: A task generator having the feature *fake_lib* which will call :py:func:`waflib.Tools.ccroot.process_lib`
	:rtype: :py:class:`waflib.TaskGen.task_gen`
	"""
	return self(name=name, features='fake_lib', lib_paths=paths, lib_type='csshlib')

