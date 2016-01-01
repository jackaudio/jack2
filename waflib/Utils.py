#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2005-2010 (ita)

"""
Utilities and platform-specific fixes

The portability fixes try to provide a consistent behavior of the Waf API
through Python versions 2.3 to 3.X and across different platforms (win32, linux, etc)
"""

import os, sys, errno, traceback, inspect, re, shutil, datetime, gc, platform
import subprocess # <- leave this!

from collections import deque, defaultdict

try:
	import _winreg as winreg
except ImportError:
	try:
		import winreg
	except ImportError:
		winreg = None

from waflib import Errors

try:
	from collections import UserDict
except ImportError:
	from UserDict import UserDict

try:
	from hashlib import md5
except ImportError:
	try:
		from md5 import md5
	except ImportError:
		# never fail to enable fixes from another module
		pass

try:
	import threading
except ImportError:
	if not 'JOBS' in os.environ:
		# no threading :-(
		os.environ['JOBS'] = '1'

	class threading(object):
		"""
			A fake threading class for platforms lacking the threading module.
			Use ``waf -j1`` on those platforms
		"""
		pass
	class Lock(object):
		"""Fake Lock class"""
		def acquire(self):
			pass
		def release(self):
			pass
	threading.Lock = threading.Thread = Lock
else:
	run_old = threading.Thread.run
	def run(*args, **kwargs):
		try:
			run_old(*args, **kwargs)
		except (KeyboardInterrupt, SystemExit):
			raise
		except Exception:
			sys.excepthook(*sys.exc_info())
	threading.Thread.run = run

SIG_NIL = 'iluvcuteoverload'.encode()
"""Arbitrary null value for a md5 hash. This value must be changed when the hash value is replaced (size)"""

O644 = 420
"""Constant representing the permissions for regular files (0644 raises a syntax error on python 3)"""

O755 = 493
"""Constant representing the permissions for executable files (0755 raises a syntax error on python 3)"""

rot_chr = ['\\', '|', '/', '-']
"List of characters to use when displaying the throbber (progress bar)"

rot_idx = 0
"Index of the current throbber character (progress bar)"

try:
	from collections import OrderedDict as ordered_iter_dict
except ImportError:
	class ordered_iter_dict(dict):
		def __init__(self, *k, **kw):
			self.lst = []
			dict.__init__(self, *k, **kw)
		def clear(self):
			dict.clear(self)
			self.lst = []
		def __setitem__(self, key, value):
			dict.__setitem__(self, key, value)
			try:
				self.lst.remove(key)
			except ValueError:
				pass
			self.lst.append(key)
		def __delitem__(self, key):
			dict.__delitem__(self, key)
			try:
				self.lst.remove(key)
			except ValueError:
				pass
		def __iter__(self):
			for x in self.lst:
				yield x
		def keys(self):
			return self.lst

is_win32 = os.sep == '\\' or sys.platform == 'win32' # msys2

def readf(fname, m='r', encoding='ISO8859-1'):
	"""
	Read an entire file into a string, use this function instead of os.open() whenever possible.

	In practice the wrapper node.read(..) should be preferred to this function::

		def build(ctx):
			from waflib import Utils
			txt = Utils.readf(self.path.find_node('wscript').abspath())
			txt = ctx.path.find_node('wscript').read()

	:type  fname: string
	:param fname: Path to file
	:type  m: string
	:param m: Open mode
	:type encoding: string
	:param encoding: encoding value, only used for python 3
	:rtype: string
	:return: Content of the file
	"""

	if sys.hexversion > 0x3000000 and not 'b' in m:
		m += 'b'
		f = open(fname, m)
		try:
			txt = f.read()
		finally:
			f.close()
		if encoding:
			txt = txt.decode(encoding)
		else:
			txt = txt.decode()
	else:
		f = open(fname, m)
		try:
			txt = f.read()
		finally:
			f.close()
	return txt

def writef(fname, data, m='w', encoding='ISO8859-1'):
	"""
	Write an entire file from a string, use this function instead of os.open() whenever possible.

	In practice the wrapper node.write(..) should be preferred to this function::

		def build(ctx):
			from waflib import Utils
			txt = Utils.writef(self.path.make_node('i_like_kittens').abspath(), 'some data')
			self.path.make_node('i_like_kittens').write('some data')

	:type  fname: string
	:param fname: Path to file
	:type   data: string
	:param  data: The contents to write to the file
	:type  m: string
	:param m: Open mode
	:type encoding: string
	:param encoding: encoding value, only used for python 3
	"""
	if sys.hexversion > 0x3000000 and not 'b' in m:
		data = data.encode(encoding)
		m += 'b'
	f = open(fname, m)
	try:
		f.write(data)
	finally:
		f.close()

def h_file(fname):
	"""
	Compute a hash value for a file by using md5. This method may be replaced by
	a faster version if necessary. The following uses the file size and the timestamp value::

		import stat
		from waflib import Utils
		def h_file(fname):
			st = os.stat(fname)
			if stat.S_ISDIR(st[stat.ST_MODE]): raise IOError('not a file')
			m = Utils.md5()
			m.update(str(st.st_mtime))
			m.update(str(st.st_size))
			m.update(fname)
			return m.digest()
		Utils.h_file = h_file

	:type fname: string
	:param fname: path to the file to hash
	:return: hash of the file contents
	"""
	f = open(fname, 'rb')
	m = md5()
	try:
		while fname:
			fname = f.read(200000)
			m.update(fname)
	finally:
		f.close()
	return m.digest()

def readf_win32(f, m='r', encoding='ISO8859-1'):
	flags = os.O_NOINHERIT | os.O_RDONLY
	if 'b' in m:
		flags |= os.O_BINARY
	if '+' in m:
		flags |= os.O_RDWR
	try:
		fd = os.open(f, flags)
	except OSError:
		raise IOError('Cannot read from %r' % f)

	if sys.hexversion > 0x3000000 and not 'b' in m:
		m += 'b'
		f = os.fdopen(fd, m)
		try:
			txt = f.read()
		finally:
			f.close()
		if encoding:
			txt = txt.decode(encoding)
		else:
			txt = txt.decode()
	else:
		f = os.fdopen(fd, m)
		try:
			txt = f.read()
		finally:
			f.close()
	return txt

def writef_win32(f, data, m='w', encoding='ISO8859-1'):
	if sys.hexversion > 0x3000000 and not 'b' in m:
		data = data.encode(encoding)
		m += 'b'
	flags = os.O_CREAT | os.O_TRUNC | os.O_WRONLY | os.O_NOINHERIT
	if 'b' in m:
		flags |= os.O_BINARY
	if '+' in m:
		flags |= os.O_RDWR
	try:
		fd = os.open(f, flags)
	except OSError:
		raise IOError('Cannot write to %r' % f)
	f = os.fdopen(fd, m)
	try:
		f.write(data)
	finally:
		f.close()

def h_file_win32(fname):
	try:
		fd = os.open(fname, os.O_BINARY | os.O_RDONLY | os.O_NOINHERIT)
	except OSError:
		raise IOError('Cannot read from %r' % fname)
	f = os.fdopen(fd, 'rb')
	m = md5()
	try:
		while fname:
			fname = f.read(200000)
			m.update(fname)
	finally:
		f.close()
	return m.digest()

# always save these
readf_unix = readf
writef_unix = writef
h_file_unix = h_file
if hasattr(os, 'O_NOINHERIT') and sys.hexversion < 0x3040000:
	# replace the default functions
	readf = readf_win32
	writef = writef_win32
	h_file = h_file_win32

try:
	x = ''.encode('hex')
except LookupError:
	import binascii
	def to_hex(s):
		ret = binascii.hexlify(s)
		if not isinstance(ret, str):
			ret = ret.decode('utf-8')
		return ret
else:
	def to_hex(s):
		return s.encode('hex')

to_hex.__doc__ = """
Return the hexadecimal representation of a string

:param s: string to convert
:type s: string
"""

def listdir_win32(s):
	"""
	List the contents of a folder in a portable manner.
	On Win32, return the list of drive letters: ['C:', 'X:', 'Z:']

	:type s: string
	:param s: a string, which can be empty on Windows
	"""
	if not s:
		try:
			import ctypes
		except ImportError:
			# there is nothing much we can do
			return [x + ':\\' for x in list('ABCDEFGHIJKLMNOPQRSTUVWXYZ')]
		else:
			dlen = 4 # length of "?:\\x00"
			maxdrives = 26
			buf = ctypes.create_string_buffer(maxdrives * dlen)
			ndrives = ctypes.windll.kernel32.GetLogicalDriveStringsA(maxdrives*dlen, ctypes.byref(buf))
			return [ str(buf.raw[4*i:4*i+2].decode('ascii')) for i in range(int(ndrives/dlen)) ]

	if len(s) == 2 and s[1] == ":":
		s += os.sep

	if not os.path.isdir(s):
		e = OSError('%s is not a directory' % s)
		e.errno = errno.ENOENT
		raise e
	return os.listdir(s)

listdir = os.listdir
if is_win32:
	listdir = listdir_win32

def num2ver(ver):
	"""
	Convert a string, tuple or version number into an integer. The number is supposed to have at most 4 digits::

		from waflib.Utils import num2ver
		num2ver('1.3.2') == num2ver((1,3,2)) == num2ver((1,3,2,0))

	:type ver: string or tuple of numbers
	:param ver: a version number
	"""
	if isinstance(ver, str):
		ver = tuple(ver.split('.'))
	if isinstance(ver, tuple):
		ret = 0
		for i in range(4):
			if i < len(ver):
				ret += 256**(3 - i) * int(ver[i])
		return ret
	return ver

def ex_stack():
	"""
	Extract the stack to display exceptions

	:return: a string represening the last exception
	"""
	exc_type, exc_value, tb = sys.exc_info()
	exc_lines = traceback.format_exception(exc_type, exc_value, tb)
	return ''.join(exc_lines)

def to_list(sth):
	"""
	Convert a string argument to a list by splitting on spaces, and pass
	through a list argument unchanged::

		from waflib.Utils import to_list
		lst = to_list("a b c d")

	:param sth: List or a string of items separated by spaces
	:rtype: list
	:return: Argument converted to list

	"""
	if isinstance(sth, str):
		return sth.split()
	else:
		return sth

def split_path_unix(path):
	return path.split('/')

def split_path_cygwin(path):
	if path.startswith('//'):
		ret = path.split('/')[2:]
		ret[0] = '/' + ret[0]
		return ret
	return path.split('/')

re_sp = re.compile('[/\\\\]')
def split_path_win32(path):
	if path.startswith('\\\\'):
		ret = re.split(re_sp, path)[2:]
		ret[0] = '\\' + ret[0]
		return ret
	return re.split(re_sp, path)

msysroot = None
def split_path_msys(path):
	if (path.startswith('/') or path.startswith('\\')) and not path.startswith('//') and not path.startswith('\\\\'):
		# msys paths can be in the form /usr/bin
		global msysroot
		if not msysroot:
			# msys has python 2.7 or 3, so we can use this
			msysroot = subprocess.check_output(['cygpath', '-w', '/']).decode(sys.stdout.encoding or 'iso8859-1')
			msysroot = msysroot.strip()
		path = os.path.normpath(msysroot + os.sep + path)
	return split_path_win32(path)

if sys.platform == 'cygwin':
	split_path = split_path_cygwin
elif is_win32:
	if os.environ.get('MSYSTEM', None):
		split_path = split_path_msys
	else:
		split_path = split_path_win32
else:
	split_path = split_path_unix

split_path.__doc__ = """
Split a path by / or \\. This function is not like os.path.split

:type  path: string
:param path: path to split
:return:     list of strings
"""

def check_dir(path):
	"""
	Ensure that a directory exists (similar to ``mkdir -p``).

	:type  path: string
	:param path: Path to directory
	"""
	if not os.path.isdir(path):
		try:
			os.makedirs(path)
		except OSError as e:
			if not os.path.isdir(path):
				raise Errors.WafError('Cannot create the folder %r' % path, ex=e)

def check_exe(name, env=None):
	"""
	Ensure that a program exists

	:type name: string
	:param name: name or path to program
	:return: path of the program or None
	"""
	if not name:
		raise ValueError('Cannot execute an empty string!')
	def is_exe(fpath):
		return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

	fpath, fname = os.path.split(name)
	if fpath and is_exe(name):
		return os.path.abspath(name)
	else:
		env = env or os.environ
		for path in env["PATH"].split(os.pathsep):
			path = path.strip('"')
			exe_file = os.path.join(path, name)
			if is_exe(exe_file):
				return os.path.abspath(exe_file)
	return None

def def_attrs(cls, **kw):
	"""
	Set default attributes on a class instance

	:type cls: class
	:param cls: the class to update the given attributes in.
	:type kw: dict
	:param kw: dictionary of attributes names and values.
	"""
	for k, v in kw.items():
		if not hasattr(cls, k):
			setattr(cls, k, v)

def quote_define_name(s):
	"""
	Convert a string to an identifier suitable for C defines.

	:type  s: string
	:param s: String to convert
	:rtype: string
	:return: Identifier suitable for C defines
	"""
	fu = re.sub('[^a-zA-Z0-9]', '_', s)
	fu = re.sub('_+', '_', fu)
	fu = fu.upper()
	return fu

def h_list(lst):
	"""
	Hash lists. For tuples, using hash(tup) is much more efficient,
	except on python >= 3.3 where hash randomization assumes everybody is running a web application.

	:param lst: list to hash
	:type lst: list of strings
	:return: hash of the list
	"""
	m = md5()
	m.update(str(lst).encode())
	return m.digest()

def h_fun(fun):
	"""
	Hash functions

	:param fun: function to hash
	:type  fun: function
	:return: hash of the function
	"""
	try:
		return fun.code
	except AttributeError:
		try:
			h = inspect.getsource(fun)
		except IOError:
			h = "nocode"
		try:
			fun.code = h
		except AttributeError:
			pass
		return h

def h_cmd(ins):
	"""
	Task command hashes are calculated by calling this function. The inputs can be
	strings, functions, tuples/lists containing strings/functions
	"""
	# this function is not meant to be particularly fast
	if isinstance(ins, str):
		# a command is either a string
		ret = ins
	elif isinstance(ins, list) or isinstance(ins, tuple):
		# or a list of functions/strings
		ret = str([h_cmd(x) for x in ins])
	else:
		# or just a python function
		ret = str(h_fun(ins))
	if sys.hexversion > 0x3000000:
		ret = ret.encode('iso8859-1', 'xmlcharrefreplace')
	return ret

reg_subst = re.compile(r"(\\\\)|(\$\$)|\$\{([^}]+)\}")
def subst_vars(expr, params):
	"""
	Replace ${VAR} with the value of VAR taken from a dict or a config set::

		from waflib import Utils
		s = Utils.subst_vars('${PREFIX}/bin', env)

	:type  expr: string
	:param expr: String to perform substitution on
	:param params: Dictionary or config set to look up variable values.
	"""
	def repl_var(m):
		if m.group(1):
			return '\\'
		if m.group(2):
			return '$'
		try:
			# ConfigSet instances may contain lists
			return params.get_flat(m.group(3))
		except AttributeError:
			return params[m.group(3)]
		# if you get a TypeError, it means that 'expr' is not a string...
		# Utils.subst_vars(None, env)  will not work
	return reg_subst.sub(repl_var, expr)

def destos_to_binfmt(key):
	"""
	Return the binary format based on the unversioned platform name.

	:param key: platform name
	:type  key: string
	:return: string representing the binary format
	"""
	if key == 'darwin':
		return 'mac-o'
	elif key in ('win32', 'cygwin', 'uwin', 'msys'):
		return 'pe'
	return 'elf'

def unversioned_sys_platform():
	"""
	Return the unversioned platform name.
	Some Python platform names contain versions, that depend on
	the build environment, e.g. linux2, freebsd6, etc.
	This returns the name without the version number. Exceptions are
	os2 and win32, which are returned verbatim.

	:rtype: string
	:return: Unversioned platform name
	"""
	s = sys.platform
	if s.startswith('java'):
		# The real OS is hidden under the JVM.
		from java.lang import System
		s = System.getProperty('os.name')
		# see http://lopica.sourceforge.net/os.html for a list of possible values
		if s == 'Mac OS X':
			return 'darwin'
		elif s.startswith('Windows '):
			return 'win32'
		elif s == 'OS/2':
			return 'os2'
		elif s == 'HP-UX':
			return 'hp-ux'
		elif s in ('SunOS', 'Solaris'):
			return 'sunos'
		else: s = s.lower()

	# powerpc == darwin for our purposes
	if s == 'powerpc':
		return 'darwin'
	if s == 'win32' or s == 'os2':
		return s
	if s == 'cli' and os.name == 'nt':
		# ironpython is only on windows as far as we know
		return 'win32'
	return re.split('\d+$', s)[0]

def nada(*k, **kw):
	"""
	A function that does nothing

	:return: None
	"""
	pass

class Timer(object):
	"""
	Simple object for timing the execution of commands.
	Its string representation is the current time::

		from waflib.Utils import Timer
		timer = Timer()
		a_few_operations()
		s = str(timer)
	"""
	def __init__(self):
		self.start_time = datetime.datetime.utcnow()

	def __str__(self):
		delta = datetime.datetime.utcnow() - self.start_time
		days = delta.days
		hours, rem = divmod(delta.seconds, 3600)
		minutes, seconds = divmod(rem, 60)
		seconds += delta.microseconds * 1e-6
		result = ''
		if days:
			result += '%dd' % days
		if days or hours:
			result += '%dh' % hours
		if days or hours or minutes:
			result += '%dm' % minutes
		return '%s%.3fs' % (result, seconds)

if is_win32:
	old = shutil.copy2
	def copy2(src, dst):
		"""
		shutil.copy2 does not copy the file attributes on windows, so we
		hack into the shutil module to fix the problem
		"""
		old(src, dst)
		shutil.copystat(src, dst)
	setattr(shutil, 'copy2', copy2)

if os.name == 'java':
	# Jython cannot disable the gc but they can enable it ... wtf?
	try:
		gc.disable()
		gc.enable()
	except NotImplementedError:
		gc.disable = gc.enable

def read_la_file(path):
	"""
	Read property files, used by msvc.py

	:param path: file to read
	:type path: string
	"""
	sp = re.compile(r'^([^=]+)=\'(.*)\'$')
	dc = {}
	for line in readf(path).splitlines():
		try:
			_, left, right, _ = sp.split(line.strip())
			dc[left] = right
		except ValueError:
			pass
	return dc

def nogc(fun):
	"""
	Decorator: let a function disable the garbage collector during its execution.
	It is used in the build context when storing/loading the build cache file (pickle)

	:param fun: function to execute
	:type fun: function
	:return: the return value of the function executed
	"""
	def f(*k, **kw):
		try:
			gc.disable()
			ret = fun(*k, **kw)
		finally:
			gc.enable()
		return ret
	f.__doc__ = fun.__doc__
	return f

def run_once(fun):
	"""
	Decorator: let a function cache its results, use like this::

		@run_once
		def foo(k):
			return 345*2343

	:param fun: function to execute
	:type fun: function
	:return: the return value of the function executed
	"""
	cache = {}
	def wrap(k):
		try:
			return cache[k]
		except KeyError:
			ret = fun(k)
			cache[k] = ret
			return ret
	wrap.__cache__ = cache
	wrap.__name__ = fun.__name__
	return wrap

def get_registry_app_path(key, filename):
	if not winreg:
		return None
	try:
		result = winreg.QueryValue(key, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\%s.exe" % filename[0])
	except WindowsError:
		pass
	else:
		if os.path.isfile(result):
			return result

def lib64():
	# default settings for /usr/lib
	if os.sep == '/':
		if platform.architecture()[0] == '64bit':
			if os.path.exists('/usr/lib64') and not os.path.exists('/usr/lib32'):
				return '64'
	return ''

def sane_path(p):
	# private function for the time being!
	return os.path.abspath(os.path.expanduser(p))

