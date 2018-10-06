#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2005-2010 (ita)

"""
C/C++/D configuration helpers
"""

import os, re, shlex
from waflib import Build, Utils, Task, Options, Logs, Errors, Runner
from waflib.TaskGen import after_method, feature
from waflib.Configure import conf

WAF_CONFIG_H   = 'config.h'
"""default name for the config.h file"""

DEFKEYS = 'define_key'
INCKEYS = 'include_key'

cfg_ver = {
	'atleast-version': '>=',
	'exact-version': '==',
	'max-version': '<=',
}

SNIP_FUNCTION = '''
int main(int argc, char **argv) {
	void (*p)();
	(void)argc; (void)argv;
	p=(void(*)())(%s);
	return !p;
}
'''
"""Code template for checking for functions"""

SNIP_TYPE = '''
int main(int argc, char **argv) {
	(void)argc; (void)argv;
	if ((%(type_name)s *) 0) return 0;
	if (sizeof (%(type_name)s)) return 0;
	return 1;
}
'''
"""Code template for checking for types"""

SNIP_EMPTY_PROGRAM = '''
int main(int argc, char **argv) {
	(void)argc; (void)argv;
	return 0;
}
'''

SNIP_FIELD = '''
int main(int argc, char **argv) {
	char *off;
	(void)argc; (void)argv;
	off = (char*) &((%(type_name)s*)0)->%(field_name)s;
	return (size_t) off < sizeof(%(type_name)s);
}
'''

MACRO_TO_DESTOS = {
'__linux__'                                      : 'linux',
'__GNU__'                                        : 'gnu', # hurd
'__FreeBSD__'                                    : 'freebsd',
'__NetBSD__'                                     : 'netbsd',
'__OpenBSD__'                                    : 'openbsd',
'__sun'                                          : 'sunos',
'__hpux'                                         : 'hpux',
'__sgi'                                          : 'irix',
'_AIX'                                           : 'aix',
'__CYGWIN__'                                     : 'cygwin',
'__MSYS__'                                       : 'cygwin',
'_UWIN'                                          : 'uwin',
'_WIN64'                                         : 'win32',
'_WIN32'                                         : 'win32',
# Note about darwin: this is also tested with 'defined __APPLE__ && defined __MACH__' somewhere below in this file.
'__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__'  : 'darwin', 
'__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__' : 'darwin', # iphone
'__QNX__'                                        : 'qnx',
'__native_client__'                              : 'nacl' # google native client platform
}

MACRO_TO_DEST_CPU = {
'__x86_64__'  : 'x86_64',
'__amd64__'   : 'x86_64',
'__i386__'    : 'x86',
'__ia64__'    : 'ia',
'__mips__'    : 'mips',
'__sparc__'   : 'sparc',
'__alpha__'   : 'alpha',
'__aarch64__' : 'aarch64',
'__thumb__'   : 'thumb',
'__arm__'     : 'arm',
'__hppa__'    : 'hppa',
'__powerpc__' : 'powerpc',
'__ppc__'     : 'powerpc',
'__convex__'  : 'convex',
'__m68k__'    : 'm68k',
'__s390x__'   : 's390x',
'__s390__'    : 's390',
'__sh__'      : 'sh',
}

@conf
def parse_flags(self, line, uselib_store, env=None, force_static=False, posix=None):
	"""
	Parse the flags from the input lines, and add them to the relevant use variables::

		def configure(conf):
			conf.parse_flags('-O3', 'FOO')
			# conf.env.CXXFLAGS_FOO = ['-O3']
			# conf.env.CFLAGS_FOO = ['-O3']

	:param line: flags
	:type line: string
	:param uselib_store: where to add the flags
	:type uselib_store: string
	:param env: config set or conf.env by default
	:type env: :py:class:`waflib.ConfigSet.ConfigSet`
	"""

	assert(isinstance(line, str))

	env = env or self.env

	# Issue 811 and 1371
	if posix is None:
		posix = True
		if '\\' in line:
			posix = ('\\ ' in line) or ('\\\\' in line)

	lex = shlex.shlex(line, posix=posix)
	lex.whitespace_split = True
	lex.commenters = ''
	lst = list(lex)

	# append_unique is not always possible
	# for example, apple flags may require both -arch i386 and -arch ppc
	app = env.append_value
	appu = env.append_unique
	uselib = uselib_store
	static = False
	while lst:
		x = lst.pop(0)
		st = x[:2]
		ot = x[2:]

		if st == '-I' or st == '/I':
			if not ot: ot = lst.pop(0)
			appu('INCLUDES_' + uselib, [ot])
		elif st == '-i':
			tmp = [x, lst.pop(0)]
			app('CFLAGS', tmp)
			app('CXXFLAGS', tmp)
		elif st == '-D' or (env.CXX_NAME == 'msvc' and st == '/D'): # not perfect but..
			if not ot: ot = lst.pop(0)
			app('DEFINES_' + uselib, [ot])
		elif st == '-l':
			if not ot: ot = lst.pop(0)
			prefix = (force_static or static) and 'STLIB_' or 'LIB_'
			appu(prefix + uselib, [ot])
		elif st == '-L':
			if not ot: ot = lst.pop(0)
			prefix = (force_static or static) and 'STLIBPATH_' or 'LIBPATH_'
			appu(prefix + uselib, [ot])
		elif x.startswith('/LIBPATH:'):
			prefix = (force_static or static) and 'STLIBPATH_' or 'LIBPATH_'
			appu(prefix + uselib, [x.replace('/LIBPATH:', '')])
		elif x == '-pthread' or x.startswith('+') or x.startswith('-std'):
			app('CFLAGS_' + uselib, [x])
			app('CXXFLAGS_' + uselib, [x])
			app('LINKFLAGS_' + uselib, [x])
		elif x == '-framework':
			appu('FRAMEWORK_' + uselib, [lst.pop(0)])
		elif x.startswith('-F'):
			appu('FRAMEWORKPATH_' + uselib, [x[2:]])
		elif x == '-Wl,-rpath' or x == '-Wl,-R':
			app('RPATH_' + uselib, lst.pop(0).lstrip('-Wl,'))
		elif x.startswith('-Wl,-R,'):
			app('RPATH_' + uselib, x[7:])
		elif x.startswith('-Wl,-R'):
			app('RPATH_' + uselib, x[6:])
		elif x.startswith('-Wl,-rpath,'):
			app('RPATH_' + uselib, x[11:])
		elif x == '-Wl,-Bstatic' or x == '-Bstatic':
			static = True
		elif x == '-Wl,-Bdynamic' or x == '-Bdynamic':
			static = False
		elif x.startswith('-Wl'):
			app('LINKFLAGS_' + uselib, [x])
		elif x.startswith('-m') or x.startswith('-f') or x.startswith('-dynamic'):
			app('CFLAGS_' + uselib, [x])
			app('CXXFLAGS_' + uselib, [x])
		elif x.startswith('-bundle'):
			app('LINKFLAGS_' + uselib, [x])
		elif x.startswith('-undefined') or x.startswith('-Xlinker'):
			arg = lst.pop(0)
			app('LINKFLAGS_' + uselib, [x, arg])
		elif x.startswith('-arch') or x.startswith('-isysroot'):
			tmp = [x, lst.pop(0)]
			app('CFLAGS_' + uselib, tmp)
			app('CXXFLAGS_' + uselib, tmp)
			app('LINKFLAGS_' + uselib, tmp)
		elif x.endswith('.a') or x.endswith('.so') or x.endswith('.dylib') or x.endswith('.lib'):
			appu('LINKFLAGS_' + uselib, [x]) # not cool, #762

@conf
def validate_cfg(self, kw):
	"""
	Search for the program *pkg-config* if missing, and validate the parameters to pass to
	:py:func:`waflib.Tools.c_config.exec_cfg`.

	:param path: the **-config program to use** (default is *pkg-config*)
	:type path: list of string
	:param msg: message to display to describe the test executed
	:type msg: string
	:param okmsg: message to display when the test is successful
	:type okmsg: string
	:param errmsg: message to display in case of error
	:type errmsg: string
	"""
	if not 'path' in kw:
		if not self.env.PKGCONFIG:
			self.find_program('pkg-config', var='PKGCONFIG')
		kw['path'] = self.env.PKGCONFIG

	# pkg-config version
	if 'atleast_pkgconfig_version' in kw:
		if not 'msg' in kw:
			kw['msg'] = 'Checking for pkg-config version >= %r' % kw['atleast_pkgconfig_version']
		return

	if not 'okmsg' in kw:
		kw['okmsg'] = 'yes'
	if not 'errmsg' in kw:
		kw['errmsg'] = 'not found'

	if 'modversion' in kw:
		if not 'msg' in kw:
			kw['msg'] = 'Checking for %r version' % kw['modversion']
		return

	# checking for the version of a module, for the moment, one thing at a time
	for x in cfg_ver.keys():
		y = x.replace('-', '_')
		if y in kw:
			if not 'package' in kw:
				raise ValueError('%s requires a package' % x)

			if not 'msg' in kw:
				kw['msg'] = 'Checking for %r %s %s' % (kw['package'], cfg_ver[x], kw[y])
			return

	if not 'define_name' in kw:
		pkgname = kw.get('uselib_store', kw['package'].upper())
		kw['define_name'] = self.have_define(pkgname)

	if not 'uselib_store' in kw:
		self.undefine(kw['define_name'])

	if not 'msg' in kw:
		kw['msg'] = 'Checking for %r' % (kw['package'] or kw['path'])

@conf
def exec_cfg(self, kw):
	"""
	Execute the program *pkg-config*:

	* if atleast_pkgconfig_version is given, check that pkg-config has the version n and return
	* if modversion is given, then return the module version
	* else, execute the *-config* program with the *args* and *variables* given, and set the flags on the *conf.env.FLAGS_name* variable

	:param atleast_pkgconfig_version: minimum pkg-config version to use (disable other tests)
	:type atleast_pkgconfig_version: string
	:param package: package name, for example *gtk+-2.0*
	:type package: string
	:param uselib_store: if the test is successful, define HAVE\_*name*. It is also used to define *conf.env.FLAGS_name* variables.
	:type uselib_store: string
	:param modversion: if provided, return the version of the given module and define *name*\_VERSION
	:type modversion: string
	:param args: arguments to give to *package* when retrieving flags
	:type args: list of string
	:param variables: return the values of particular variables
	:type variables: list of string
	:param define_variable: additional variables to define (also in conf.env.PKG_CONFIG_DEFINES)
	:type define_variable: dict(string: string)
	"""

	path = Utils.to_list(kw['path'])
	env = self.env.env or None
	def define_it():
		pkgname = kw.get('uselib_store', kw['package'].upper())
		if kw.get('global_define'):
			# compatibility, replace by pkgname in WAF 1.9?
			self.define(self.have_define(kw['package']), 1, False)
		else:
			self.env.append_unique('DEFINES_%s' % pkgname, "%s=1" % self.have_define(pkgname))
		self.env[self.have_define(pkgname)] = 1

	# pkg-config version
	if 'atleast_pkgconfig_version' in kw:
		cmd = path + ['--atleast-pkgconfig-version=%s' % kw['atleast_pkgconfig_version']]
		self.cmd_and_log(cmd, env=env)
		if not 'okmsg' in kw:
			kw['okmsg'] = 'yes'
		return

	# checking for the version of a module
	for x in cfg_ver:
		y = x.replace('-', '_')
		if y in kw:
			self.cmd_and_log(path + ['--%s=%s' % (x, kw[y]), kw['package']], env=env)
			if not 'okmsg' in kw:
				kw['okmsg'] = 'yes'
			define_it()
			break

	# retrieving the version of a module
	if 'modversion' in kw:
		version = self.cmd_and_log(path + ['--modversion', kw['modversion']], env=env).strip()
		self.define('%s_VERSION' % Utils.quote_define_name(kw.get('uselib_store', kw['modversion'])), version)
		return version

	lst = [] + path

	defi = kw.get('define_variable', None)
	if not defi:
		defi = self.env.PKG_CONFIG_DEFINES or {}
	for key, val in defi.items():
		lst.append('--define-variable=%s=%s' % (key, val))

	static = kw.get('force_static', False)
	if 'args' in kw:
		args = Utils.to_list(kw['args'])
		if '--static' in args or '--static-libs' in args:
			static = True
		lst += args

	# tools like pkgconf expect the package argument after the -- ones -_-
	lst.extend(Utils.to_list(kw['package']))

	# retrieving variables of a module
	if 'variables' in kw:
		v = kw.get('env', self.env)
		uselib = kw.get('uselib_store', kw['package'].upper())
		vars = Utils.to_list(kw['variables'])
		for v in vars:
			val = self.cmd_and_log(lst + ['--variable=' + v], env=env).strip()
			var = '%s_%s' % (uselib, v)
			v[var] = val
		if not 'okmsg' in kw:
			kw['okmsg'] = 'yes'
		return

	# so we assume the command-line will output flags to be parsed afterwards
	ret = self.cmd_and_log(lst, env=env)
	if not 'okmsg' in kw:
		kw['okmsg'] = 'yes'

	define_it()
	self.parse_flags(ret, kw.get('uselib_store', kw['package'].upper()), kw.get('env', self.env), force_static=static, posix=kw.get('posix', None))
	return ret

@conf
def check_cfg(self, *k, **kw):
	"""
	Check for configuration flags using a **-config**-like program (pkg-config, sdl-config, etc).
	Encapsulate the calls to :py:func:`waflib.Tools.c_config.validate_cfg` and :py:func:`waflib.Tools.c_config.exec_cfg`

	A few examples::

		def configure(conf):
			conf.load('compiler_c')
			conf.check_cfg(package='glib-2.0', args='--libs --cflags')
			conf.check_cfg(package='glib-2.0', uselib_store='GLIB', atleast_version='2.10.0',
				args='--cflags --libs')
			conf.check_cfg(package='pango')
			conf.check_cfg(package='pango', uselib_store='MYPANGO', args=['--cflags', '--libs'])
			conf.check_cfg(package='pango',
				args=['pango >= 0.1.0', 'pango < 9.9.9', '--cflags', '--libs'],
				msg="Checking for 'pango 0.1.0'")
			conf.check_cfg(path='sdl-config', args='--cflags --libs', package='', uselib_store='SDL')
			conf.check_cfg(path='mpicc', args='--showme:compile --showme:link',
				package='', uselib_store='OPEN_MPI', mandatory=False)
			# variables
			conf.check_cfg(package='gtk+-2.0', variables=['includedir', 'prefix'], uselib_store='FOO')
			print(conf.env.FOO_includedir)
	"""
	if k:
		lst = k[0].split()
		kw['package'] = lst[0]
		kw['args'] = ' '.join(lst[1:])

	self.validate_cfg(kw)
	if 'msg' in kw:
		self.start_msg(kw['msg'], **kw)
	ret = None
	try:
		ret = self.exec_cfg(kw)
	except self.errors.WafError:
		if 'errmsg' in kw:
			self.end_msg(kw['errmsg'], 'YELLOW', **kw)
		if Logs.verbose > 1:
			raise
		else:
			self.fatal('The configuration failed')
	else:
		if not ret:
			ret = True
		kw['success'] = ret
		if 'okmsg' in kw:
			self.end_msg(self.ret_msg(kw['okmsg'], kw), **kw)

	return ret

def build_fun(bld):
	if bld.kw['compile_filename']:
		node = bld.srcnode.make_node(bld.kw['compile_filename'])
		node.write(bld.kw['code'])

	o = bld(features=bld.kw['features'], source=bld.kw['compile_filename'], target='testprog')

	for k, v in bld.kw.items():
		setattr(o, k, v)

	if not bld.kw.get('quiet', None):
		bld.conf.to_log("==>\n%s\n<==" % bld.kw['code'])

@conf
def validate_c(self, kw):
	"""
	pre-check the parameters that will be given to :py:func:`waflib.Configure.run_build`

	:param compiler: c or cxx (tries to guess what is best)
	:type compiler: string
	:param type: cprogram, cshlib, cstlib - not required if *features are given directly*
	:type type: binary to create
	:param feature: desired features for the task generator that will execute the test, for example ``cxx cxxstlib``
	:type feature: list of string
	:param fragment: provide a piece of code for the test (default is to let the system create one)
	:type fragment: string
	:param uselib_store: define variables after the test is executed (IMPORTANT!)
	:type uselib_store: string
	:param use: parameters to use for building (just like the normal *use* keyword)
	:type use: list of string
	:param define_name: define to set when the check is over
	:type define_name: string
	:param execute: execute the resulting binary
	:type execute: bool
	:param define_ret: if execute is set to True, use the execution output in both the define and the return value
	:type define_ret: bool
	:param header_name: check for a particular header
	:type header_name: string
	:param auto_add_header_name: if header_name was set, add the headers in env.INCKEYS so the next tests will include these headers
	:type auto_add_header_name: bool
	"""

	if not 'build_fun' in kw:
		kw['build_fun'] = build_fun

	if not 'env' in kw:
		kw['env'] = self.env.derive()
	env = kw['env']

	if not 'compiler' in kw and not 'features' in kw:
		kw['compiler'] = 'c'
		if env['CXX_NAME'] and Task.classes.get('cxx', None):
			kw['compiler'] = 'cxx'
			if not self.env['CXX']:
				self.fatal('a c++ compiler is required')
		else:
			if not self.env['CC']:
				self.fatal('a c compiler is required')

	if not 'compile_mode' in kw:
		kw['compile_mode'] = 'c'
		if 'cxx' in Utils.to_list(kw.get('features',[])) or kw.get('compiler', '') == 'cxx':
			kw['compile_mode'] = 'cxx'

	if not 'type' in kw:
		kw['type'] = 'cprogram'

	if not 'features' in kw:
		if not 'header_name' in kw or kw.get('link_header_test', True):
			kw['features'] = [kw['compile_mode'], kw['type']] # "c ccprogram"
		else:
			kw['features'] = [kw['compile_mode']]
	else:
		kw['features'] = Utils.to_list(kw['features'])

	if not 'compile_filename' in kw:
		kw['compile_filename'] = 'test.c' + ((kw['compile_mode'] == 'cxx') and 'pp' or '')


	def to_header(dct):
		if 'header_name' in dct:
			dct = Utils.to_list(dct['header_name'])
			return ''.join(['#include <%s>\n' % x for x in dct])
		return ''

	#OSX
	if 'framework_name' in kw:
		fwkname = kw['framework_name']
		if not 'uselib_store' in kw:
			kw['uselib_store'] = fwkname.upper()

		if not kw.get('no_header', False):
			if not 'header_name' in kw:
				kw['header_name'] = []
			fwk = '%s/%s.h' % (fwkname, fwkname)
			if kw.get('remove_dot_h', None):
				fwk = fwk[:-2]
			kw['header_name'] = Utils.to_list(kw['header_name']) + [fwk]

		kw['msg'] = 'Checking for framework %s' % fwkname
		kw['framework'] = fwkname
		#kw['frameworkpath'] = set it yourself

	if 'function_name' in kw:
		fu = kw['function_name']
		if not 'msg' in kw:
			kw['msg'] = 'Checking for function %s' % fu
		kw['code'] = to_header(kw) + SNIP_FUNCTION % fu
		if not 'uselib_store' in kw:
			kw['uselib_store'] = fu.upper()
		if not 'define_name' in kw:
			kw['define_name'] = self.have_define(fu)

	elif 'type_name' in kw:
		tu = kw['type_name']
		if not 'header_name' in kw:
			kw['header_name'] = 'stdint.h'
		if 'field_name' in kw:
			field = kw['field_name']
			kw['code'] = to_header(kw) + SNIP_FIELD % {'type_name' : tu, 'field_name' : field}
			if not 'msg' in kw:
				kw['msg'] = 'Checking for field %s in %s' % (field, tu)
			if not 'define_name' in kw:
				kw['define_name'] = self.have_define((tu + '_' + field).upper())
		else:
			kw['code'] = to_header(kw) + SNIP_TYPE % {'type_name' : tu}
			if not 'msg' in kw:
				kw['msg'] = 'Checking for type %s' % tu
			if not 'define_name' in kw:
				kw['define_name'] = self.have_define(tu.upper())

	elif 'header_name' in kw:
		if not 'msg' in kw:
			kw['msg'] = 'Checking for header %s' % kw['header_name']

		l = Utils.to_list(kw['header_name'])
		assert len(l)>0, 'list of headers in header_name is empty'

		kw['code'] = to_header(kw) + SNIP_EMPTY_PROGRAM

		if not 'uselib_store' in kw:
			kw['uselib_store'] = l[0].upper()

		if not 'define_name' in kw:
			kw['define_name'] = self.have_define(l[0])

	if 'lib' in kw:
		if not 'msg' in kw:
			kw['msg'] = 'Checking for library %s' % kw['lib']
		if not 'uselib_store' in kw:
			kw['uselib_store'] = kw['lib'].upper()

	if 'stlib' in kw:
		if not 'msg' in kw:
			kw['msg'] = 'Checking for static library %s' % kw['stlib']
		if not 'uselib_store' in kw:
			kw['uselib_store'] = kw['stlib'].upper()

	if 'fragment' in kw:
		# an additional code fragment may be provided to replace the predefined code
		# in custom headers
		kw['code'] = kw['fragment']
		if not 'msg' in kw:
			kw['msg'] = 'Checking for code snippet'
		if not 'errmsg' in kw:
			kw['errmsg'] = 'no'

	for (flagsname,flagstype) in (('cxxflags','compiler'), ('cflags','compiler'), ('linkflags','linker')):
		if flagsname in kw:
			if not 'msg' in kw:
				kw['msg'] = 'Checking for %s flags %s' % (flagstype, kw[flagsname])
			if not 'errmsg' in kw:
				kw['errmsg'] = 'no'

	if not 'execute' in kw:
		kw['execute'] = False
	if kw['execute']:
		kw['features'].append('test_exec')
		kw['chmod'] = 493

	if not 'errmsg' in kw:
		kw['errmsg'] = 'not found'

	if not 'okmsg' in kw:
		kw['okmsg'] = 'yes'

	if not 'code' in kw:
		kw['code'] = SNIP_EMPTY_PROGRAM

	# if there are headers to append automatically to the next tests
	if self.env[INCKEYS]:
		kw['code'] = '\n'.join(['#include <%s>' % x for x in self.env[INCKEYS]]) + '\n' + kw['code']

	# in case defines lead to very long command-lines
	if kw.get('merge_config_header', False) or env.merge_config_header:
		kw['code'] = '%s\n\n%s' % (self.get_config_header(), kw['code'])
		env.DEFINES = [] # modify the copy

	if not kw.get('success'): kw['success'] = None

	if 'define_name' in kw:
		self.undefine(kw['define_name'])
	if not 'msg' in kw:
		self.fatal('missing "msg" in conf.check(...)')

@conf
def post_check(self, *k, **kw):
	"Set the variables after a test executed in :py:func:`waflib.Tools.c_config.check` was run successfully"

	is_success = 0
	if kw['execute']:
		if kw['success'] is not None:
			if kw.get('define_ret', False):
				is_success = kw['success']
			else:
				is_success = (kw['success'] == 0)
	else:
		is_success = (kw['success'] == 0)

	if 'define_name' in kw:
		# TODO simplify!
		if 'header_name' in kw or 'function_name' in kw or 'type_name' in kw or 'fragment' in kw:
			if kw['execute'] and kw.get('define_ret', None) and isinstance(is_success, str):
				self.define(kw['define_name'], is_success, quote=kw.get('quote', 1))
			else:
				self.define_cond(kw['define_name'], is_success)
		else:
			self.define_cond(kw['define_name'], is_success)

		# consistency with check_cfg
		if kw.get('global_define', None):
			self.env[kw['define_name']] = is_success

	if 'header_name' in kw:
		if kw.get('auto_add_header_name', False):
			self.env.append_value(INCKEYS, Utils.to_list(kw['header_name']))

	if is_success and 'uselib_store' in kw:
		from waflib.Tools import ccroot

		# TODO see get_uselib_vars from ccroot.py
		_vars = set([])
		for x in kw['features']:
			if x in ccroot.USELIB_VARS:
				_vars |= ccroot.USELIB_VARS[x]

		for k in _vars:
			lk = k.lower()
			if lk in kw:
				val = kw[lk]
				# remove trailing slash
				if isinstance(val, str):
					val = val.rstrip(os.path.sep)
				self.env.append_unique(k + '_' + kw['uselib_store'], Utils.to_list(val))
	return is_success

@conf
def check(self, *k, **kw):
	"""
	Perform a configuration test by calling :py:func:`waflib.Configure.run_build`.
	For the complete list of parameters, see :py:func:`waflib.Tools.c_config.validate_c`.
	To force a specific compiler, pass "compiler='c'" or "compiler='cxx'" in the arguments

	Besides build targets, complete builds can be given though a build function. All files will
	be written to a temporary directory::

		def build(bld):
			lib_node = bld.srcnode.make_node('libdir/liblc1.c')
			lib_node.parent.mkdir()
			lib_node.write('#include <stdio.h>\\nint lib_func(void) { FILE *f = fopen("foo", "r");}\\n', 'w')
			bld(features='c cshlib', source=[lib_node], linkflags=conf.env.EXTRA_LDFLAGS, target='liblc')
		conf.check(build_fun=build, msg=msg)
	"""
	self.validate_c(kw)
	self.start_msg(kw['msg'], **kw)
	ret = None
	try:
		ret = self.run_build(*k, **kw)
	except self.errors.ConfigurationError:
		self.end_msg(kw['errmsg'], 'YELLOW', **kw)
		if Logs.verbose > 1:
			raise
		else:
			self.fatal('The configuration failed')
	else:
		kw['success'] = ret

	ret = self.post_check(*k, **kw)
	if not ret:
		self.end_msg(kw['errmsg'], 'YELLOW', **kw)
		self.fatal('The configuration failed %r' % ret)
	else:
		self.end_msg(self.ret_msg(kw['okmsg'], kw), **kw)
	return ret

class test_exec(Task.Task):
	"""
	A task for executing a programs after they are built. See :py:func:`waflib.Tools.c_config.test_exec_fun`.
	"""
	color = 'PINK'
	def run(self):
		if getattr(self.generator, 'rpath', None):
			if getattr(self.generator, 'define_ret', False):
				self.generator.bld.retval = self.generator.bld.cmd_and_log([self.inputs[0].abspath()])
			else:
				self.generator.bld.retval = self.generator.bld.exec_command([self.inputs[0].abspath()])
		else:
			env = self.env.env or {}
			env.update(dict(os.environ))
			for var in ('LD_LIBRARY_PATH', 'DYLD_LIBRARY_PATH', 'PATH'):
				env[var] = self.inputs[0].parent.abspath() + os.path.pathsep + env.get(var, '')
			if getattr(self.generator, 'define_ret', False):
				self.generator.bld.retval = self.generator.bld.cmd_and_log([self.inputs[0].abspath()], env=env)
			else:
				self.generator.bld.retval = self.generator.bld.exec_command([self.inputs[0].abspath()], env=env)

@feature('test_exec')
@after_method('apply_link')
def test_exec_fun(self):
	"""
	The feature **test_exec** is used to create a task that will to execute the binary
	created (link task output) during the build. The exit status will be set
	on the build context, so only one program may have the feature *test_exec*.
	This is used by configuration tests::

		def configure(conf):
			conf.check(execute=True)
	"""
	self.create_task('test_exec', self.link_task.outputs[0])

@conf
def check_cxx(self, *k, **kw):
	# DO NOT USE
	kw['compiler'] = 'cxx'
	return self.check(*k, **kw)

@conf
def check_cc(self, *k, **kw):
	# DO NOT USE
	kw['compiler'] = 'c'
	return self.check(*k, **kw)

@conf
def define(self, key, val, quote=True):
	"""
	Store a single define and its state into conf.env.DEFINES. If the value is True, False or None it is cast to 1 or 0.

	:param key: define name
	:type key: string
	:param val: value
	:type val: int or string
	:param quote: enclose strings in quotes (yes by default)
	:type quote: bool
	"""
	assert key and isinstance(key, str)

	if val is True:
		val = 1
	elif val in (False, None):
		val = 0

	if isinstance(val, int) or isinstance(val, float):
		s = '%s=%s'
	else:
		s = quote and '%s="%s"' or '%s=%s'
	app = s % (key, str(val))

	ban = key + '='
	lst = self.env['DEFINES']
	for x in lst:
		if x.startswith(ban):
			lst[lst.index(x)] = app
			break
	else:
		self.env.append_value('DEFINES', app)

	self.env.append_unique(DEFKEYS, key)

@conf
def undefine(self, key):
	"""
	Remove a define from conf.env.DEFINES

	:param key: define name
	:type key: string
	"""
	assert key and isinstance(key, str)

	ban = key + '='
	lst = [x for x in self.env['DEFINES'] if not x.startswith(ban)]
	self.env['DEFINES'] = lst
	self.env.append_unique(DEFKEYS, key)

@conf
def define_cond(self, key, val):
	"""
	Conditionally define a name::

		def configure(conf):
			conf.define_cond('A', True)
			# equivalent to:
			# if val: conf.define('A', 1)
			# else: conf.undefine('A')

	:param key: define name
	:type key: string
	:param val: value
	:type val: int or string
	"""
	assert key and isinstance(key, str)

	if val:
		self.define(key, 1)
	else:
		self.undefine(key)

@conf
def is_defined(self, key):
	"""
	:param key: define name
	:type key: string
	:return: True if the define is set
	:rtype: bool
	"""
	assert key and isinstance(key, str)

	ban = key + '='
	for x in self.env['DEFINES']:
		if x.startswith(ban):
			return True
	return False

@conf
def get_define(self, key):
	"""
	:param key: define name
	:type key: string
	:return: the value of a previously stored define or None if it is not set
	"""
	assert key and isinstance(key, str)

	ban = key + '='
	for x in self.env['DEFINES']:
		if x.startswith(ban):
			return x[len(ban):]
	return None

@conf
def have_define(self, key):
	"""
	:param key: define name
	:type key: string
	:return: the input key prefixed by *HAVE_* and substitute any invalid characters.
	:rtype: string
	"""
	return (self.env.HAVE_PAT or 'HAVE_%s') % Utils.quote_define_name(key)

@conf
def write_config_header(self, configfile='', guard='', top=False, defines=True, headers=False, remove=True, define_prefix=''):
	"""
	Write a configuration header containing defines and includes::

		def configure(cnf):
			cnf.define('A', 1)
			cnf.write_config_header('config.h')

	This function only adds include guards (if necessary), consult
	:py:func:`waflib.Tools.c_config.get_config_header` for details on the body.

	:param configfile: path to the file to create (relative or absolute)
	:type configfile: string
	:param guard: include guard name to add, by default it is computed from the file name
	:type guard: string
	:param top: write the configuration header from the build directory (default is from the current path)
	:type top: bool
	:param defines: add the defines (yes by default)
	:type defines: bool
	:param headers: add #include in the file
	:type headers: bool
	:param remove: remove the defines after they are added (yes by default, works like in autoconf)
	:type remove: bool
	:type define_prefix: string
	:param define_prefix: prefix all the defines in the file with a particular prefix
	"""
	if not configfile: configfile = WAF_CONFIG_H
	waf_guard = guard or 'W_%s_WAF' % Utils.quote_define_name(configfile)

	node = top and self.bldnode or self.path.get_bld()
	node = node.make_node(configfile)
	node.parent.mkdir()

	lst = ['/* WARNING! All changes made to this file will be lost! */\n']
	lst.append('#ifndef %s\n#define %s\n' % (waf_guard, waf_guard))
	lst.append(self.get_config_header(defines, headers, define_prefix=define_prefix))
	lst.append('\n#endif /* %s */\n' % waf_guard)

	node.write('\n'.join(lst))

	# config files must not be removed on "waf clean"
	self.env.append_unique(Build.CFG_FILES, [node.abspath()])

	if remove:
		for key in self.env[DEFKEYS]:
			self.undefine(key)
		self.env[DEFKEYS] = []

@conf
def get_config_header(self, defines=True, headers=False, define_prefix=''):
	"""
	Create the contents of a ``config.h`` file from the defines and includes
	set in conf.env.define_key / conf.env.include_key. No include guards are added.

	A prelude will be added from the variable env.WAF_CONFIG_H_PRELUDE if provided. This
	can be used to insert complex macros or include guards::

		def configure(conf):
			conf.env.WAF_CONFIG_H_PRELUDE = '#include <unistd.h>\\n'
			conf.write_config_header('config.h')

	:param defines: write the defines values
	:type defines: bool
	:param headers: write include entries for each element in self.env.INCKEYS
	:type headers: bool
	:type define_prefix: string
	:param define_prefix: prefix all the defines with a particular prefix
	:return: the contents of a ``config.h`` file
	:rtype: string
	"""
	lst = []

	if self.env.WAF_CONFIG_H_PRELUDE:
		lst.append(self.env.WAF_CONFIG_H_PRELUDE)

	if headers:
		for x in self.env[INCKEYS]:
			lst.append('#include <%s>' % x)

	if defines:
		tbl = {}
		for k in self.env['DEFINES']:
			a, _, b = k.partition('=')
			tbl[a] = b

		for k in self.env[DEFKEYS]:
			try:
				txt = '#define %s%s %s' % (define_prefix, k, tbl[k])
			except KeyError:
				txt = '/* #undef %s%s */' % (define_prefix, k)
			lst.append(txt)
	return "\n".join(lst)

@conf
def cc_add_flags(conf):
	"""
	Add CFLAGS / CPPFLAGS from os.environ to conf.env
	"""
	conf.add_os_flags('CPPFLAGS', dup=False)
	conf.add_os_flags('CFLAGS', dup=False)

@conf
def cxx_add_flags(conf):
	"""
	Add CXXFLAGS / CPPFLAGS from os.environ to conf.env
	"""
	conf.add_os_flags('CPPFLAGS', dup=False)
	conf.add_os_flags('CXXFLAGS', dup=False)

@conf
def link_add_flags(conf):
	"""
	Add LINKFLAGS / LDFLAGS from os.environ to conf.env
	"""
	conf.add_os_flags('LINKFLAGS', dup=False)
	conf.add_os_flags('LDFLAGS', dup=False)

@conf
def cc_load_tools(conf):
	"""
	Load the c tool
	"""
	if not conf.env.DEST_OS:
		conf.env.DEST_OS = Utils.unversioned_sys_platform()
	conf.load('c')

@conf
def cxx_load_tools(conf):
	"""
	Load the cxx tool
	"""
	if not conf.env.DEST_OS:
		conf.env.DEST_OS = Utils.unversioned_sys_platform()
	conf.load('cxx')

@conf
def get_cc_version(conf, cc, gcc=False, icc=False, clang=False):
	"""
	Run the preprocessor to determine the compiler version

	The variables CC_VERSION, DEST_OS, DEST_BINFMT and DEST_CPU will be set in *conf.env*
	"""
	cmd = cc + ['-dM', '-E', '-']
	env = conf.env.env or None
	try:
		out, err = conf.cmd_and_log(cmd, output=0, input='\n'.encode(), env=env)
	except Exception:
		conf.fatal('Could not determine the compiler version %r' % cmd)

	if gcc:
		if out.find('__INTEL_COMPILER') >= 0:
			conf.fatal('The intel compiler pretends to be gcc')
		if out.find('__GNUC__') < 0 and out.find('__clang__') < 0:
			conf.fatal('Could not determine the compiler type')

	if icc and out.find('__INTEL_COMPILER') < 0:
		conf.fatal('Not icc/icpc')

	if clang and out.find('__clang__') < 0:
		conf.fatal('Not clang/clang++')
	if not clang and out.find('__clang__') >= 0:
		conf.fatal('Could not find gcc/g++ (only Clang), if renamed try eg: CC=gcc48 CXX=g++48 waf configure')

	k = {}
	if icc or gcc or clang:
		out = out.splitlines()
		for line in out:
			lst = shlex.split(line)
			if len(lst)>2:
				key = lst[1]
				val = lst[2]
				k[key] = val

		def isD(var):
			return var in k

		# Some documentation is available at http://predef.sourceforge.net
		# The names given to DEST_OS must match what Utils.unversioned_sys_platform() returns.
		if not conf.env.DEST_OS:
			conf.env.DEST_OS = ''
		for i in MACRO_TO_DESTOS:
			if isD(i):
				conf.env.DEST_OS = MACRO_TO_DESTOS[i]
				break
		else:
			if isD('__APPLE__') and isD('__MACH__'):
				conf.env.DEST_OS = 'darwin'
			elif isD('__unix__'): # unix must be tested last as it's a generic fallback
				conf.env.DEST_OS = 'generic'

		if isD('__ELF__'):
			conf.env.DEST_BINFMT = 'elf'
		elif isD('__WINNT__') or isD('__CYGWIN__') or isD('_WIN32'):
			conf.env.DEST_BINFMT = 'pe'
			conf.env.LIBDIR = conf.env.BINDIR
		elif isD('__APPLE__'):
			conf.env.DEST_BINFMT = 'mac-o'

		if not conf.env.DEST_BINFMT:
			# Infer the binary format from the os name.
			conf.env.DEST_BINFMT = Utils.destos_to_binfmt(conf.env.DEST_OS)

		for i in MACRO_TO_DEST_CPU:
			if isD(i):
				conf.env.DEST_CPU = MACRO_TO_DEST_CPU[i]
				break

		Logs.debug('ccroot: dest platform: ' + ' '.join([conf.env[x] or '?' for x in ('DEST_OS', 'DEST_BINFMT', 'DEST_CPU')]))
		if icc:
			ver = k['__INTEL_COMPILER']
			conf.env['CC_VERSION'] = (ver[:-2], ver[-2], ver[-1])
		else:
			if isD('__clang__') and isD('__clang_major__'):
				conf.env['CC_VERSION'] = (k['__clang_major__'], k['__clang_minor__'], k['__clang_patchlevel__'])
			else:
				# older clang versions and gcc
				conf.env['CC_VERSION'] = (k['__GNUC__'], k['__GNUC_MINOR__'], k.get('__GNUC_PATCHLEVEL__', '0'))
	return k

@conf
def get_xlc_version(conf, cc):
	"""Get the compiler version"""

	cmd = cc + ['-qversion']
	try:
		out, err = conf.cmd_and_log(cmd, output=0)
	except Errors.WafError:
		conf.fatal('Could not find xlc %r' % cmd)

	# the intention is to catch the 8.0 in "IBM XL C/C++ Enterprise Edition V8.0 for AIX..."
	for v in (r"IBM XL C/C\+\+.* V(?P<major>\d*)\.(?P<minor>\d*)",):
		version_re = re.compile(v, re.I).search
		match = version_re(out or err)
		if match:
			k = match.groupdict()
			conf.env['CC_VERSION'] = (k['major'], k['minor'])
			break
	else:
		conf.fatal('Could not determine the XLC version.')

@conf
def get_suncc_version(conf, cc):
	"""Get the compiler version"""

	cmd = cc + ['-V']
	try:
		out, err = conf.cmd_and_log(cmd, output=0)
	except Errors.WafError as e:
		# Older versions of the compiler exit with non-zero status when reporting their version
		if not (hasattr(e, 'returncode') and hasattr(e, 'stdout') and hasattr(e, 'stderr')):
			conf.fatal('Could not find suncc %r' % cmd)
		out = e.stdout
		err = e.stderr

	version = (out or err)
	version = version.splitlines()[0]

	version_re = re.compile(r'cc:\s+sun\s+(c\+\+|c)\s+(?P<major>\d*)\.(?P<minor>\d*)', re.I).search
	match = version_re(version)
	if match:
		k = match.groupdict()
		conf.env['CC_VERSION'] = (k['major'], k['minor'])
	else:
		conf.fatal('Could not determine the suncc version.')

# ============ the --as-needed flag should added during the configuration, not at runtime =========

@conf
def add_as_needed(self):
	"""
	Add ``--as-needed`` to the *LINKFLAGS*
	On some platforms, it is a default flag.  In some cases (e.g., in NS-3) it is necessary to explicitly disable this feature with `-Wl,--no-as-needed` flag.
	"""
	if self.env.DEST_BINFMT == 'elf' and 'gcc' in (self.env.CXX_NAME, self.env.CC_NAME):
		self.env.append_unique('LINKFLAGS', '-Wl,--as-needed')

# ============ parallel configuration

class cfgtask(Task.TaskBase):
	"""
	A task that executes configuration tests
	make sure that the checks write to conf.env in a thread-safe manner

	for the moment it only executes conf.check
	"""
	def display(self):
		return ''

	def runnable_status(self):
		return Task.RUN_ME

	def uid(self):
		return Utils.SIG_NIL

	def run(self):
		conf = self.conf
		bld = Build.BuildContext(top_dir=conf.srcnode.abspath(), out_dir=conf.bldnode.abspath())
		bld.env = conf.env
		bld.init_dirs()
		bld.in_msg = 1 # suppress top-level start_msg
		bld.logger = self.logger
		try:
			bld.check(**self.args)
		except Exception:
			return 1

@conf
def multicheck(self, *k, **kw):
	"""
	Use tuples to perform parallel configuration tests
	"""
	self.start_msg(kw.get('msg', 'Executing %d configuration tests' % len(k)), **kw)

	class par(object):
		def __init__(self):
			self.keep = False
			self.returned_tasks = []
			self.task_sigs = {}
			self.progress_bar = 0
		def total(self):
			return len(tasks)
		def to_log(self, *k, **kw):
			return

	bld = par()
	tasks = []
	for dct in k:
		x = cfgtask(bld=bld)
		tasks.append(x)
		x.args = dct
		x.bld = bld
		x.conf = self
		x.args = dct

		# bind a logger that will keep the info in memory
		x.logger = Logs.make_mem_logger(str(id(x)), self.logger)

	def it():
		yield tasks
		while 1:
			yield []
	p = Runner.Parallel(bld, Options.options.jobs)
	p.biter = it()
	p.start()

	# flush the logs in order into the config.log
	for x in tasks:
		x.logger.memhandler.flush()

	if p.error:
		for x in p.error:
			if getattr(x, 'err_msg', None):
				self.to_log(x.err_msg)
				self.end_msg('fail', color='RED')
				raise Errors.WafError('There is an error in the library, read config.log for more information')

	for x in tasks:
		if x.hasrun != Task.SUCCESS:
			self.end_msg(kw.get('errmsg', 'no'), color='YELLOW', **kw)
			self.fatal(kw.get('fatalmsg', None) or 'One of the tests has failed, read config.log for more information')

	self.end_msg('ok', **kw)

