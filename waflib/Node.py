#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2005-2010 (ita)

"""
Node: filesystem structure, contains lists of nodes

#. Each file/folder is represented by exactly one node.

#. Some potential class properties are stored on :py:class:`waflib.Build.BuildContext` : nodes to depend on, etc.
   Unused class members can increase the `.wafpickle` file size sensibly.

#. Node objects should never be created directly, use
   the methods :py:func:`Node.make_node` or :py:func:`Node.find_node`

#. The methods :py:func:`Node.find_resource`, :py:func:`Node.find_dir` :py:func:`Node.find_or_declare` should be
   used when a build context is present

#. Each instance of :py:class:`waflib.Context.Context` has a unique :py:class:`Node` subclass.
   (:py:class:`waflib.Node.Nod3`, see the :py:class:`waflib.Context.Context` initializer). A reference to the context owning a node is held as self.ctx
"""

import os, re, sys, shutil
from waflib import Utils, Errors

exclude_regs = '''
**/*~
**/#*#
**/.#*
**/%*%
**/._*
**/CVS
**/CVS/**
**/.cvsignore
**/SCCS
**/SCCS/**
**/vssver.scc
**/.svn
**/.svn/**
**/BitKeeper
**/.git
**/.git/**
**/.gitignore
**/.bzr
**/.bzrignore
**/.bzr/**
**/.hg
**/.hg/**
**/_MTN
**/_MTN/**
**/.arch-ids
**/{arch}
**/_darcs
**/_darcs/**
**/.intlcache
**/.DS_Store'''
"""
Ant patterns for files and folders to exclude while doing the
recursive traversal in :py:meth:`waflib.Node.Node.ant_glob`
"""

# TODO remove in waf 1.9
split_path = Utils.split_path
split_path_unix = Utils.split_path_unix
split_path_cygwin = Utils.split_path_cygwin
split_path_win32 = Utils.split_path_win32

class Node(object):
	"""
	This class is organized in two parts

	* The basic methods meant for filesystem access (compute paths, create folders, etc)
	* The methods bound to a :py:class:`waflib.Build.BuildContext` (require ``bld.srcnode`` and ``bld.bldnode``)

	The Node objects are not thread safe in any way.
	"""
	dict_class = dict
	__slots__ = ('name', 'sig', 'children', 'parent', 'cache_abspath', 'cache_isdir', 'cache_sig')
	def __init__(self, name, parent):
		self.name = name
		self.parent = parent

		if parent:
			if name in parent.children:
				raise Errors.WafError('node %s exists in the parent files %r already' % (name, parent))
			parent.children[name] = self

	def __setstate__(self, data):
		"Deserializes from data"
		self.name = data[0]
		self.parent = data[1]
		if data[2] is not None:
			# Issue 1480
			self.children = self.dict_class(data[2])
		if data[3] is not None:
			self.sig = data[3]

	def __getstate__(self):
		"Serialize the node info"
		return (self.name, self.parent, getattr(self, 'children', None), getattr(self, 'sig', None))

	def __str__(self):
		"String representation (name), for debugging purposes"
		return self.name

	def __repr__(self):
		"String representation (abspath), for debugging purposes"
		return self.abspath()

	def __hash__(self):
		"Node hash, used for storage in dicts. This hash is not persistent."
		return id(self)

	def __eq__(self, node):
		"Node comparison, based on the IDs"
		return id(self) == id(node)

	def __copy__(self):
		"Implemented to prevent nodes from being copied (raises an exception)"
		raise Errors.WafError('nodes are not supposed to be copied')

	def read(self, flags='r', encoding='ISO8859-1'):
		"""
		Return the contents of the file represented by this node::

			def build(bld):
				bld.path.find_node('wscript').read()

		:type  fname: string
		:param fname: Path to file
		:type  m: string
		:param m: Open mode
		:rtype: string
		:return: File contents
		"""
		return Utils.readf(self.abspath(), flags, encoding)

	def write(self, data, flags='w', encoding='ISO8859-1'):
		"""
		Write some text to the physical file represented by this node::

			def build(bld):
				bld.path.make_node('foo.txt').write('Hello, world!')

		:type  data: string
		:param data: data to write
		:type  flags: string
		:param flags: Write mode
		"""
		Utils.writef(self.abspath(), data, flags, encoding)

	def read_json(self, convert=True, encoding='utf-8'):
		"""
		Read and parse the contents of this node as JSON::

			def build(bld):
				bld.path.find_node('abc.json').read_json()

		Note that this by default automatically decodes unicode strings on Python2, unlike what the Python JSON module does.

		:type  convert: boolean
		:param convert: Prevents decoding of unicode strings on Python2
		:type  encoding: string
		:param encoding: The encoding of the file to read. This default to UTF8 as per the JSON standard
		:rtype: object
		:return: Parsed file contents
		"""
		import json # Python 2.6 and up
		object_pairs_hook = None
		if convert and sys.hexversion < 0x3000000:
			try:
				_type = unicode
			except NameError:
				_type = str

			def convert(value):
				if isinstance(value, list):
					return [convert(element) for element in value]
				elif isinstance(value, _type):
					return str(value)
				else:
					return value

			def object_pairs(pairs):
				return dict((str(pair[0]), convert(pair[1])) for pair in pairs)

			object_pairs_hook = object_pairs

		return json.loads(self.read(encoding=encoding), object_pairs_hook=object_pairs_hook)

	def write_json(self, data, pretty=True):
		"""
		Writes a python object as JSON to disk. Files are always written as UTF8 as per the JSON standard::

			def build(bld):
				bld.path.find_node('xyz.json').write_json(199)

		:type  data: object
		:param data: The data to write to disk
		:type  pretty: boolean
		:param pretty: Determines if the JSON will be nicely space separated
		"""
		import json # Python 2.6 and up
		indent = 2
		separators = (',', ': ')
		sort_keys = pretty
		newline = os.linesep
		if not pretty:
			indent = None
			separators = (',', ':')
			newline = ''
		output = json.dumps(data, indent=indent, separators=separators, sort_keys=sort_keys) + newline
		self.write(output, encoding='utf-8')

	def chmod(self, val):
		"""
		Change file/dir permissions::

			def build(bld):
				bld.path.chmod(493) # 0755
		"""
		os.chmod(self.abspath(), val)

	def delete(self):
		"""Delete the file/folder, and remove this node from the tree. Do not use this object after calling this method."""
		try:
			try:
				if hasattr(self, 'children'):
					shutil.rmtree(self.abspath())
				else:
					os.remove(self.abspath())
			except OSError as e:
				if os.path.exists(self.abspath()):
					raise e
		finally:
			self.evict()

	def evict(self):
		"""Internal - called when a node is removed"""
		del self.parent.children[self.name]

	def suffix(self):
		"""Return the file extension"""
		k = max(0, self.name.rfind('.'))
		return self.name[k:]

	def height(self):
		"""Depth in the folder hierarchy from the filesystem root or from all the file drives"""
		d = self
		val = -1
		while d:
			d = d.parent
			val += 1
		return val

	def listdir(self):
		"""List the folder contents"""
		lst = Utils.listdir(self.abspath())
		lst.sort()
		return lst

	def mkdir(self):
		"""
		Create a folder represented by this node, creating intermediate nodes as needed
		An exception will be raised only when the folder cannot possibly exist there
		"""
		if getattr(self, 'cache_isdir', None):
			return

		try:
			self.parent.mkdir()
		except OSError:
			pass

		if self.name:
			try:
				os.makedirs(self.abspath())
			except OSError:
				pass

			if not os.path.isdir(self.abspath()):
				raise Errors.WafError('Could not create the directory %s' % self.abspath())

			try:
				self.children
			except AttributeError:
				self.children = self.dict_class()

		self.cache_isdir = True

	def find_node(self, lst):
		"""
		Find a node on the file system (files or folders), create intermediate nodes as needed

		:param lst: path
		:type lst: string or list of string
		"""

		if isinstance(lst, str):
			lst = [x for x in split_path(lst) if x and x != '.']

		cur = self
		for x in lst:
			if x == '..':
				cur = cur.parent or cur
				continue

			try:
				ch = cur.children
			except AttributeError:
				cur.children = self.dict_class()
			else:
				try:
					cur = ch[x]
					continue
				except KeyError:
					pass

			# optimistic: create the node first then look if it was correct to do so
			cur = self.__class__(x, cur)
			try:
				os.stat(cur.abspath())
			except OSError:
				cur.evict()
				return None

		ret = cur

		try:
			os.stat(ret.abspath())
		except OSError:
			ret.evict()
			return None

		try:
			while not getattr(cur.parent, 'cache_isdir', None):
				cur = cur.parent
				cur.cache_isdir = True
		except AttributeError:
			pass

		return ret

	def make_node(self, lst):
		"""
		Find or create a node without looking on the filesystem

		:param lst: path
		:type lst: string or list of string
		"""
		if isinstance(lst, str):
			lst = [x for x in split_path(lst) if x and x != '.']

		cur = self
		for x in lst:
			if x == '..':
				cur = cur.parent or cur
				continue

			if getattr(cur, 'children', {}):
				if x in cur.children:
					cur = cur.children[x]
					continue
			else:
				cur.children = self.dict_class()
			cur = self.__class__(x, cur)
		return cur

	def search_node(self, lst):
		"""
		Search for a node without looking on the filesystem

		:param lst: path
		:type lst: string or list of string
		"""
		if isinstance(lst, str):
			lst = [x for x in split_path(lst) if x and x != '.']

		cur = self
		for x in lst:
			if x == '..':
				cur = cur.parent or cur
			else:
				try:
					cur = cur.children[x]
				except (AttributeError, KeyError):
					return None
		return cur

	def path_from(self, node):
		"""
		Path of this node seen from the other::

			def build(bld):
				n1 = bld.path.find_node('foo/bar/xyz.txt')
				n2 = bld.path.find_node('foo/stuff/')
				n1.path_from(n2) # '../bar/xyz.txt'

		:param node: path to use as a reference
		:type node: :py:class:`waflib.Node.Node`
		"""

		c1 = self
		c2 = node

		c1h = c1.height()
		c2h = c2.height()

		lst = []
		up = 0

		while c1h > c2h:
			lst.append(c1.name)
			c1 = c1.parent
			c1h -= 1

		while c2h > c1h:
			up += 1
			c2 = c2.parent
			c2h -= 1

		while id(c1) != id(c2):
			lst.append(c1.name)
			up += 1

			c1 = c1.parent
			c2 = c2.parent

		if c1.parent:
			for i in range(up):
				lst.append('..')
		else:
			if lst and not Utils.is_win32:
				lst.append('')
		lst.reverse()
		return os.sep.join(lst) or '.'

	def abspath(self):
		"""
		Absolute path. A cache is kept in the context as ``cache_node_abspath``
		"""
		try:
			return self.cache_abspath
		except AttributeError:
			pass
		# think twice before touching this (performance + complexity + correctness)

		if not self.parent:
			val = os.sep
		elif not self.parent.name:
			val = os.sep + self.name
		else:
			val = self.parent.abspath() + os.sep + self.name
		self.cache_abspath = val
		return val

	if Utils.is_win32:
		def abspath(self):
			try:
				return self.cache_abspath
			except AttributeError:
				pass
			if not self.parent:
				val = ''
			elif not self.parent.name:
				val = self.name + os.sep
			else:
				val = self.parent.abspath().rstrip(os.sep) + os.sep + self.name
			self.cache_abspath = val
			return val

	def is_child_of(self, node):
		"""
		Does this node belong to the subtree node?::

			def build(bld):
				node = bld.path.find_node('wscript')
				node.is_child_of(bld.path) # True

		:param node: path to use as a reference
		:type node: :py:class:`waflib.Node.Node`
		"""
		p = self
		diff = self.height() - node.height()
		while diff > 0:
			diff -= 1
			p = p.parent
		return id(p) == id(node)

	def ant_iter(self, accept=None, maxdepth=25, pats=[], dir=False, src=True, remove=True):
		"""
		Semi-private and recursive method used by ant_glob.

		:param accept: function used for accepting/rejecting a node, returns the patterns that can be still accepted in recursion
		:type accept: function
		:param maxdepth: maximum depth in the filesystem (25)
		:type maxdepth: int
		:param pats: list of patterns to accept and list of patterns to exclude
		:type pats: tuple
		:param dir: return folders too (False by default)
		:type dir: bool
		:param src: return files (True by default)
		:type src: bool
		:param remove: remove files/folders that do not exist (True by default)
		:type remove: bool
		"""
		dircont = self.listdir()
		dircont.sort()

		try:
			lst = set(self.children.keys())
		except AttributeError:
			self.children = self.dict_class()
		else:
			if remove:
				for x in lst - set(dircont):
					self.children[x].evict()

		for name in dircont:
			npats = accept(name, pats)
			if npats and npats[0]:
				accepted = [] in npats[0]

				node = self.make_node([name])

				isdir = os.path.isdir(node.abspath())
				if accepted:
					if isdir:
						if dir:
							yield node
					else:
						if src:
							yield node

				if getattr(node, 'cache_isdir', None) or isdir:
					node.cache_isdir = True
					if maxdepth:
						for k in node.ant_iter(accept=accept, maxdepth=maxdepth - 1, pats=npats, dir=dir, src=src, remove=remove):
							yield k
		raise StopIteration

	def ant_glob(self, *k, **kw):
		"""
		This method is used for finding files across folders. It behaves like ant patterns:

		* ``**/*`` find all files recursively
		* ``**/*.class`` find all files ending by .class
		* ``..`` find files having two dot characters

		For example::

			def configure(cfg):
				cfg.path.ant_glob('**/*.cpp') # find all .cpp files
				cfg.root.ant_glob('etc/*.txt') # using the filesystem root can be slow
				cfg.path.ant_glob('*.cpp', excl=['*.c'], src=True, dir=False)

		For more information see http://ant.apache.org/manual/dirtasks.html

		The nodes that correspond to files and folders that do not exist will be removed. To prevent this
		behaviour, pass 'remove=False'

		:param incl: ant patterns or list of patterns to include
		:type incl: string or list of strings
		:param excl: ant patterns or list of patterns to exclude
		:type excl: string or list of strings
		:param dir: return folders too (False by default)
		:type dir: bool
		:param src: return files (True by default)
		:type src: bool
		:param remove: remove files/folders that do not exist (True by default)
		:type remove: bool
		:param maxdepth: maximum depth of recursion
		:type maxdepth: int
		:param ignorecase: ignore case while matching (False by default)
		:type ignorecase: bool
		"""

		src = kw.get('src', True)
		dir = kw.get('dir', False)

		excl = kw.get('excl', exclude_regs)
		incl = k and k[0] or kw.get('incl', '**')
		reflags = kw.get('ignorecase', 0) and re.I

		def to_pat(s):
			lst = Utils.to_list(s)
			ret = []
			for x in lst:
				x = x.replace('\\', '/').replace('//', '/')
				if x.endswith('/'):
					x += '**'
				lst2 = x.split('/')
				accu = []
				for k in lst2:
					if k == '**':
						accu.append(k)
					else:
						k = k.replace('.', '[.]').replace('*','.*').replace('?', '.').replace('+', '\\+')
						k = '^%s$' % k
						try:
							#print "pattern", k
							accu.append(re.compile(k, flags=reflags))
						except Exception as e:
							raise Errors.WafError("Invalid pattern: %s" % k, e)
				ret.append(accu)
			return ret

		def filtre(name, nn):
			ret = []
			for lst in nn:
				if not lst:
					pass
				elif lst[0] == '**':
					ret.append(lst)
					if len(lst) > 1:
						if lst[1].match(name):
							ret.append(lst[2:])
					else:
						ret.append([])
				elif lst[0].match(name):
					ret.append(lst[1:])
			return ret

		def accept(name, pats):
			nacc = filtre(name, pats[0])
			nrej = filtre(name, pats[1])
			if [] in nrej:
				nacc = []
			return [nacc, nrej]

		ret = [x for x in self.ant_iter(accept=accept, pats=[to_pat(incl), to_pat(excl)], maxdepth=kw.get('maxdepth', 25), dir=dir, src=src, remove=kw.get('remove', True))]
		if kw.get('flat', False):
			return ' '.join([x.path_from(self) for x in ret])

		return ret

	# --------------------------------------------------------------------------------
	# the following methods require the source/build folders (bld.srcnode/bld.bldnode)
	# using a subclass is a possibility, but is that really necessary?
	# --------------------------------------------------------------------------------

	def is_src(self):
		"""
		True if the node is below the source directory
		note: !is_src does not imply is_bld()

		:rtype: bool
		"""
		cur = self
		x = id(self.ctx.srcnode)
		y = id(self.ctx.bldnode)
		while cur.parent:
			if id(cur) == y:
				return False
			if id(cur) == x:
				return True
			cur = cur.parent
		return False

	def is_bld(self):
		"""
		True if the node is below the build directory
		note: !is_bld does not imply is_src

		:rtype: bool
		"""
		cur = self
		y = id(self.ctx.bldnode)
		while cur.parent:
			if id(cur) == y:
				return True
			cur = cur.parent
		return False

	def get_src(self):
		"""
		Return the equivalent src node (or self if not possible)

		:rtype: :py:class:`waflib.Node.Node`
		"""
		cur = self
		x = id(self.ctx.srcnode)
		y = id(self.ctx.bldnode)
		lst = []
		while cur.parent:
			if id(cur) == y:
				lst.reverse()
				return self.ctx.srcnode.make_node(lst)
			if id(cur) == x:
				return self
			lst.append(cur.name)
			cur = cur.parent
		return self

	def get_bld(self):
		"""
		Return the equivalent bld node (or self if not possible)

		:rtype: :py:class:`waflib.Node.Node`
		"""
		cur = self
		x = id(self.ctx.srcnode)
		y = id(self.ctx.bldnode)
		lst = []
		while cur.parent:
			if id(cur) == y:
				return self
			if id(cur) == x:
				lst.reverse()
				return self.ctx.bldnode.make_node(lst)
			lst.append(cur.name)
			cur = cur.parent
		# the file is external to the current project, make a fake root in the current build directory
		lst.reverse()
		if lst and Utils.is_win32 and len(lst[0]) == 2 and lst[0].endswith(':'):
			lst[0] = lst[0][0]
		return self.ctx.bldnode.make_node(['__root__'] + lst)

	def find_resource(self, lst):
		"""
		Try to find a declared build node or a source file

		:param lst: path
		:type lst: string or list of string
		"""
		if isinstance(lst, str):
			lst = [x for x in split_path(lst) if x and x != '.']

		node = self.get_bld().search_node(lst)
		if not node:
			self = self.get_src()
			node = self.find_node(lst)
		if node:
			if os.path.isdir(node.abspath()):
				return None
		return node

	def find_or_declare(self, lst):
		"""
		if 'self' is in build directory, try to return an existing node
		if no node is found, go to the source directory
		try to find an existing node in the source directory
		if no node is found, create it in the build directory

		:param lst: path
		:type lst: string or list of string
		"""
		if isinstance(lst, str):
			lst = [x for x in split_path(lst) if x and x != '.']

		node = self.get_bld().search_node(lst)
		if node:
			if not os.path.isfile(node.abspath()):
				node.sig = None
				node.parent.mkdir()
			return node
		self = self.get_src()
		node = self.find_node(lst)
		if node:
			if not os.path.isfile(node.abspath()):
				node.sig = None
				node.parent.mkdir()
			return node
		node = self.get_bld().make_node(lst)
		node.parent.mkdir()
		return node

	def find_dir(self, lst):
		"""
		Search for a folder in the filesystem

		:param lst: path
		:type lst: string or list of string
		"""
		if isinstance(lst, str):
			lst = [x for x in split_path(lst) if x and x != '.']

		node = self.find_node(lst)
		try:
			if not os.path.isdir(node.abspath()):
				return None
		except (OSError, AttributeError):
			# the node might be None, and raise an AttributeError
			return None
		return node

	# helpers for building things
	def change_ext(self, ext, ext_in=None):
		"""
		:return: A build node of the same path, but with a different extension
		:rtype: :py:class:`waflib.Node.Node`
		"""
		name = self.name
		if ext_in is None:
			k = name.rfind('.')
			if k >= 0:
				name = name[:k] + ext
			else:
				name = name + ext
		else:
			name = name[:- len(ext_in)] + ext

		return self.parent.find_or_declare([name])

	def bldpath(self):
		"Path seen from the build directory default/src/foo.cpp"
		return self.path_from(self.ctx.bldnode)

	def srcpath(self):
		"Path seen from the source directory ../src/foo.cpp"
		return self.path_from(self.ctx.srcnode)

	def relpath(self):
		"If a file in the build directory, bldpath, else srcpath"
		cur = self
		x = id(self.ctx.bldnode)
		while cur.parent:
			if id(cur) == x:
				return self.bldpath()
			cur = cur.parent
		return self.srcpath()

	def bld_dir(self):
		"Build path without the file name"
		return self.parent.bldpath()

	def get_bld_sig(self):
		"""
		Node signature, assuming the file is in the build directory
		"""
		try:
			return self.cache_sig
		except AttributeError:
			pass

		if not self.is_bld() or self.ctx.bldnode is self.ctx.srcnode:
			self.sig = Utils.h_file(self.abspath())
		self.cache_sig = ret = self.sig
		return ret

pickle_lock = Utils.threading.Lock()
"""Lock mandatory for thread-safe node serialization"""

class Nod3(Node):
	"""Mandatory subclass for thread-safe node serialization"""
	pass # do not remove


