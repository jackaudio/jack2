#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2006-2010 (ita)

"""
Support for translation tools such as msgfmt and intltool

Usage::

	def configure(conf):
		conf.load('gnu_dirs intltool')

	def build(bld):
		# process the .po files into .gmo files, and install them in LOCALEDIR
		bld(features='intltool_po', appname='myapp', podir='po', install_path="${LOCALEDIR}")

		# process an input file, substituting the translations from the po dir
		bld(
			features  = "intltool_in",
			podir     = "../po",
			style     = "desktop",
			flags     = ["-u"],
			source    = 'kupfer.desktop.in',
			install_path = "${DATADIR}/applications",
		)

Usage of the :py:mod:`waflib.Tools.gnu_dirs` is recommended, but not obligatory.
"""

import os, re
from waflib import Configure, Context, TaskGen, Task, Utils, Runner, Options, Build, Logs
import waflib.Tools.ccroot
from waflib.TaskGen import feature, before_method, taskgen_method
from waflib.Logs import error
from waflib.Configure import conf

_style_flags = {
	'ba': '-b',
	'desktop': '-d',
	'keys': '-k',
	'quoted': '--quoted-style',
	'quotedxml': '--quotedxml-style',
	'rfc822deb': '-r',
	'schemas': '-s',
	'xml': '-x',
}

@taskgen_method
def ensure_localedir(self):
	"""
	Expand LOCALEDIR from DATAROOTDIR/locale if possible, or fallback to PREFIX/share/locale
	"""
	# use the tool gnu_dirs to provide options to define this
	if not self.env.LOCALEDIR:
		if self.env.DATAROOTDIR:
			self.env.LOCALEDIR = os.path.join(self.env.DATAROOTDIR, 'locale')
		else:
			self.env.LOCALEDIR = os.path.join(self.env.PREFIX, 'share', 'locale')

@before_method('process_source')
@feature('intltool_in')
def apply_intltool_in_f(self):
	"""
	Create tasks to translate files by intltool-merge::

		def build(bld):
			bld(
				features  = "intltool_in",
				podir     = "../po",
				style     = "desktop",
				flags     = ["-u"],
				source    = 'kupfer.desktop.in',
				install_path = "${DATADIR}/applications",
			)

	:param podir: location of the .po files
	:type podir: string
	:param source: source files to process
	:type source: list of string
	:param style: the intltool-merge mode of operation, can be one of the following values:
	  ``ba``, ``desktop``, ``keys``, ``quoted``, ``quotedxml``, ``rfc822deb``, ``schemas`` and ``xml``.
	  See the ``intltool-merge`` man page for more information about supported modes of operation.
	:type style: string
	:param flags: compilation flags ("-quc" by default)
	:type flags: list of string
	:param install_path: installation path
	:type install_path: string
	"""
	try: self.meths.remove('process_source')
	except ValueError: pass

	self.ensure_localedir()

	podir = getattr(self, 'podir', '.')
	podirnode = self.path.find_dir(podir)
	if not podirnode:
		error("could not find the podir %r" % podir)
		return

	cache = getattr(self, 'intlcache', '.intlcache')
	self.env.INTLCACHE = [os.path.join(str(self.path.get_bld()), podir, cache)]
	self.env.INTLPODIR = podirnode.bldpath()
	self.env.append_value('INTLFLAGS', getattr(self, 'flags', self.env.INTLFLAGS_DEFAULT))

	if '-c' in self.env.INTLFLAGS:
		self.bld.fatal('Redundant -c flag in intltool task %r' % self)

	style = getattr(self, 'style', None)
	if style:
		try:
			style_flag = _style_flags[style]
		except KeyError:
			self.bld.fatal('intltool_in style "%s" is not valid' % style)

		self.env.append_unique('INTLFLAGS', [style_flag])

	for i in self.to_list(self.source):
		node = self.path.find_resource(i)

		task = self.create_task('intltool', node, node.change_ext(''))
		inst = getattr(self, 'install_path', None)
		if inst:
			self.bld.install_files(inst, task.outputs)

@feature('intltool_po')
def apply_intltool_po(self):
	"""
	Create tasks to process po files::

		def build(bld):
			bld(features='intltool_po', appname='myapp', podir='po', install_path="${LOCALEDIR}")

	The relevant task generator arguments are:

	:param podir: directory of the .po files
	:type podir: string
	:param appname: name of the application
	:type appname: string
	:param install_path: installation directory
	:type install_path: string

	The file LINGUAS must be present in the directory pointed by *podir* and list the translation files to process.
	"""
	try: self.meths.remove('process_source')
	except ValueError: pass

	self.ensure_localedir()

	appname = getattr(self, 'appname', getattr(Context.g_module, Context.APPNAME, 'set_your_app_name'))
	podir = getattr(self, 'podir', '.')
	inst = getattr(self, 'install_path', '${LOCALEDIR}')

	linguas = self.path.find_node(os.path.join(podir, 'LINGUAS'))
	if linguas:
		# scan LINGUAS file for locales to process
		file = open(linguas.abspath())
		langs = []
		for line in file.readlines():
			# ignore lines containing comments
			if not line.startswith('#'):
				langs += line.split()
		file.close()
		re_linguas = re.compile('[-a-zA-Z_@.]+')
		for lang in langs:
			# Make sure that we only process lines which contain locales
			if re_linguas.match(lang):
				node = self.path.find_resource(os.path.join(podir, re_linguas.match(lang).group() + '.po'))
				task = self.create_task('po', node, node.change_ext('.mo'))

				if inst:
					filename = task.outputs[0].name
					(langname, ext) = os.path.splitext(filename)
					inst_file = inst + os.sep + langname + os.sep + 'LC_MESSAGES' + os.sep + appname + '.mo'
					self.bld.install_as(inst_file, task.outputs[0], chmod=getattr(self, 'chmod', Utils.O644), env=task.env)

	else:
		Logs.pprint('RED', "Error no LINGUAS file found in po directory")

class po(Task.Task):
	"""
	Compile .po files into .gmo files
	"""
	run_str = '${MSGFMT} -o ${TGT} ${SRC}'
	color   = 'BLUE'

class intltool(Task.Task):
	"""
	Let intltool-merge translate an input file
	"""
	run_str = '${INTLTOOL} ${INTLFLAGS} ${INTLCACHE_ST:INTLCACHE} ${INTLPODIR} ${SRC} ${TGT}'
	color   = 'BLUE'

@conf
def find_msgfmt(conf):
	conf.find_program('msgfmt', var='MSGFMT')

@conf
def find_intltool_merge(conf):
	if not conf.env.PERL:
		conf.find_program('perl', var='PERL')
	conf.env.INTLCACHE_ST = '--cache=%s'
	conf.env.INTLFLAGS_DEFAULT = ['-q', '-u']
	conf.find_program('intltool-merge', interpreter='PERL', var='INTLTOOL')

def configure(conf):
	"""
	Detect the program *msgfmt* and set *conf.env.MSGFMT*.
	Detect the program *intltool-merge* and set *conf.env.INTLTOOL*.
	It is possible to set INTLTOOL in the environment, but it must not have spaces in it::

		$ INTLTOOL="/path/to/the program/intltool" waf configure

	If a C/C++ compiler is present, execute a compilation test to find the header *locale.h*.
	"""
	conf.find_msgfmt()
	conf.find_intltool_merge()

	if conf.env.CC or conf.env.CXX:
		conf.check(header_name='locale.h')

