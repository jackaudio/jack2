#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2006-2010 (ita)

"""
TeX/LaTeX/PDFLaTeX/XeLaTeX support

Example::

	def configure(conf):
		conf.load('tex')
		if not conf.env.LATEX:
			conf.fatal('The program LaTex is required')

	def build(bld):
		bld(
			features = 'tex',
			type     = 'latex', # pdflatex or xelatex
			source   = 'document.ltx', # mandatory, the source
			outs     = 'ps', # 'pdf' or 'ps pdf'
			deps     = 'crossreferencing.lst', # to give dependencies directly
			prompt   = 1, # 0 for the batch mode
			)

Notes:

- To configure with a special program, use::

     $ PDFLATEX=luatex waf configure

- This tool doesn't use the target attribute of the task generator
  (``bld(target=...)``); the target file name is built from the source
  base name and the out type(s)

"""

import os, re
from waflib import Utils, Task, Errors, Logs, Node
from waflib.TaskGen import feature, before_method

re_bibunit = re.compile(r'\\(?P<type>putbib)\[(?P<file>[^\[\]]*)\]',re.M)
def bibunitscan(self):
	"""
	Parse the inputs and try to find the *bibunit* dependencies

	:return: list of bibunit files
	:rtype: list of :py:class:`waflib.Node.Node`
	"""
	node = self.inputs[0]

	nodes = []
	if not node: return nodes

	code = node.read()

	for match in re_bibunit.finditer(code):
		path = match.group('file')
		if path:
			for k in ('', '.bib'):
				# add another loop for the tex include paths?
				Logs.debug('tex: trying %s%s' % (path, k))
				fi = node.parent.find_resource(path + k)
				if fi:
					nodes.append(fi)
					# no break, people are crazy
			else:
				Logs.debug('tex: could not find %s' % path)

	Logs.debug("tex: found the following bibunit files: %s" % nodes)
	return nodes

exts_deps_tex = ['', '.ltx', '.tex', '.bib', '.pdf', '.png', '.eps', '.ps', '.sty']
"""List of typical file extensions included in latex files"""

exts_tex = ['.ltx', '.tex']
"""List of typical file extensions that contain latex"""

re_tex = re.compile(r'\\(?P<type>usepackage|RequirePackage|include|bibliography([^\[\]{}]*)|putbib|includegraphics|input|import|bringin|lstinputlisting)(\[[^\[\]]*\])?{(?P<file>[^{}]*)}',re.M)
"""Regexp for expressions that may include latex files"""

g_bibtex_re = re.compile('bibdata', re.M)
"""Regexp for bibtex files"""

g_glossaries_re = re.compile('\\@newglossary', re.M)
"""Regexp for expressions that create glossaries"""

class tex(Task.Task):
	"""
	Compile a tex/latex file.

	.. inheritance-diagram:: waflib.Tools.tex.latex waflib.Tools.tex.xelatex waflib.Tools.tex.pdflatex
	"""

	bibtex_fun, _ = Task.compile_fun('${BIBTEX} ${BIBTEXFLAGS} ${SRCFILE}', shell=False)
	bibtex_fun.__doc__ = """
	Execute the program **bibtex**
	"""

	makeindex_fun, _ = Task.compile_fun('${MAKEINDEX} ${MAKEINDEXFLAGS} ${SRCFILE}', shell=False)
	makeindex_fun.__doc__ = """
	Execute the program **makeindex**
	"""

	makeglossaries_fun, _ = Task.compile_fun('${MAKEGLOSSARIES} ${SRCFILE}', shell=False)
	makeglossaries_fun.__doc__ = """
	Execute the program **makeglossaries**
	"""

	def exec_command(self, cmd, **kw):
		"""
		Override :py:meth:`waflib.Task.Task.exec_command` to execute the command without buffering (latex may prompt for inputs)

		:return: the return code
		:rtype: int
		"""
		bld = self.generator.bld
		Logs.info('runner: %r' % cmd)
		try:
			if not kw.get('cwd', None):
				kw['cwd'] = bld.cwd
		except AttributeError:
			bld.cwd = kw['cwd'] = bld.variant_dir
		return Utils.subprocess.Popen(cmd, **kw).wait()

	def scan_aux(self, node):
		"""
		A recursive regex-based scanner that finds included auxiliary files.
		"""
		nodes = [node]
		re_aux = re.compile(r'\\@input{(?P<file>[^{}]*)}', re.M)

		def parse_node(node):
			code = node.read()
			for match in re_aux.finditer(code):
				path = match.group('file')
				found = node.parent.find_or_declare(path)
				if found and found not in nodes:
					Logs.debug('tex: found aux node ' + found.abspath())
					nodes.append(found)
					parse_node(found)

		parse_node(node)
		return nodes

	def scan(self):
		"""
		A recursive regex-based scanner that finds latex dependencies. It uses :py:attr:`waflib.Tools.tex.re_tex`

		Depending on your needs you might want:

		* to change re_tex::

			from waflib.Tools import tex
			tex.re_tex = myregex

		* or to change the method scan from the latex tasks::

			from waflib.Task import classes
			classes['latex'].scan = myscanfunction
		"""
		node = self.inputs[0]

		nodes = []
		names = []
		seen = []
		if not node: return (nodes, names)

		def parse_node(node):
			if node in seen:
				return
			seen.append(node)
			code = node.read()
			global re_tex
			for match in re_tex.finditer(code):

				multibib = match.group('type')
				if multibib and multibib.startswith('bibliography'):
					multibib = multibib[len('bibliography'):]
					if multibib.startswith('style'):
						continue
				else:
					multibib = None

				for path in match.group('file').split(','):
					if path:
						add_name = True
						found = None
						for k in exts_deps_tex:

							# issue 1067, scan in all texinputs folders
							for up in self.texinputs_nodes:
								Logs.debug('tex: trying %s%s' % (path, k))
								found = up.find_resource(path + k)
								if found:
									break


							for tsk in self.generator.tasks:
								if not found or found in tsk.outputs:
									break
							else:
								nodes.append(found)
								add_name = False
								for ext in exts_tex:
									if found.name.endswith(ext):
										parse_node(found)
										break

							# multibib stuff
							if found and multibib and found.name.endswith('.bib'):
								try:
									self.multibibs.append(found)
								except AttributeError:
									self.multibibs = [found]

							# no break, people are crazy
						if add_name:
							names.append(path)
		parse_node(node)

		for x in nodes:
			x.parent.get_bld().mkdir()

		Logs.debug("tex: found the following : %s and names %s" % (nodes, names))
		return (nodes, names)

	def check_status(self, msg, retcode):
		"""
		Check an exit status and raise an error with a particular message

		:param msg: message to display if the code is non-zero
		:type msg: string
		:param retcode: condition
		:type retcode: boolean
		"""
		if retcode != 0:
			raise Errors.WafError("%r command exit status %r" % (msg, retcode))

	def bibfile(self):
		"""
		Parse the *.aux* files to find bibfiles to process.
		If yes, execute :py:meth:`waflib.Tools.tex.tex.bibtex_fun`
		"""
		for aux_node in self.aux_nodes:
			try:
				ct = aux_node.read()
			except EnvironmentError:
				Logs.error('Error reading %s: %r' % aux_node.abspath())
				continue

			if g_bibtex_re.findall(ct):
				Logs.info('calling bibtex')

				self.env.env = {}
				self.env.env.update(os.environ)
				self.env.env.update({'BIBINPUTS': self.texinputs(), 'BSTINPUTS': self.texinputs()})
				self.env.SRCFILE = aux_node.name[:-4]
				self.check_status('error when calling bibtex', self.bibtex_fun())

		for node in getattr(self, 'multibibs', []):
			self.env.env = {}
			self.env.env.update(os.environ)
			self.env.env.update({'BIBINPUTS': self.texinputs(), 'BSTINPUTS': self.texinputs()})
			self.env.SRCFILE = node.name[:-4]
			self.check_status('error when calling bibtex', self.bibtex_fun())

	def bibunits(self):
		"""
		Parse the *.aux* file to find bibunit files. If there are bibunit files,
		execute :py:meth:`waflib.Tools.tex.tex.bibtex_fun`.
		"""
		try:
			bibunits = bibunitscan(self)
		except OSError:
			Logs.error('error bibunitscan')
		else:
			if bibunits:
				fn  = ['bu' + str(i) for i in range(1, len(bibunits) + 1)]
				if fn:
					Logs.info('calling bibtex on bibunits')

				for f in fn:
					self.env.env = {'BIBINPUTS': self.texinputs(), 'BSTINPUTS': self.texinputs()}
					self.env.SRCFILE = f
					self.check_status('error when calling bibtex', self.bibtex_fun())

	def makeindex(self):
		"""
		Look on the filesystem if there is a *.idx* file to process. If yes, execute
		:py:meth:`waflib.Tools.tex.tex.makeindex_fun`
		"""
		self.idx_node = self.inputs[0].change_ext('.idx')
		try:
			idx_path = self.idx_node.abspath()
			os.stat(idx_path)
		except OSError:
			Logs.info('index file %s absent, not calling makeindex' % idx_path)
		else:
			Logs.info('calling makeindex')

			self.env.SRCFILE = self.idx_node.name
			self.env.env = {}
			self.check_status('error when calling makeindex %s' % idx_path, self.makeindex_fun())

	def bibtopic(self):
		"""
		Additional .aux files from the bibtopic package
		"""
		p = self.inputs[0].parent.get_bld()
		if os.path.exists(os.path.join(p.abspath(), 'btaux.aux')):
			self.aux_nodes += p.ant_glob('*[0-9].aux')

	def makeglossaries(self):
		src_file = self.inputs[0].abspath()
		base_file = os.path.basename(src_file)
		base, _ = os.path.splitext(base_file)
		for aux_node in self.aux_nodes:
			try:
				ct = aux_node.read()
			except EnvironmentError:
				Logs.error('Error reading %s: %r' % aux_node.abspath())
				continue

			if g_glossaries_re.findall(ct):
				if not self.env.MAKEGLOSSARIES:
					raise Errors.WafError("The program 'makeglossaries' is missing!")
				Logs.warn('calling makeglossaries')
				self.env.SRCFILE = base
				self.check_status('error when calling makeglossaries %s' % base, self.makeglossaries_fun())
				return

	def texinputs(self):
		return os.pathsep.join([k.abspath() for k in self.texinputs_nodes]) + os.pathsep

	def run(self):
		"""
		Runs the TeX build process.

		It may require multiple passes, depending on the usage of cross-references,
		bibliographies, content susceptible of needing such passes.
		The appropriate TeX compiler is called until the *.aux* files stop changing.

		Makeindex and bibtex are called if necessary.
		"""
		env = self.env

		if not env['PROMPT_LATEX']:
			env.append_value('LATEXFLAGS', '-interaction=batchmode')
			env.append_value('PDFLATEXFLAGS', '-interaction=batchmode')
			env.append_value('XELATEXFLAGS', '-interaction=batchmode')

		# important, set the cwd for everybody
		self.cwd = self.inputs[0].parent.get_bld().abspath()

		Logs.info('first pass on %s' % self.__class__.__name__)

		# Hash .aux files before even calling the LaTeX compiler
		cur_hash = self.hash_aux_nodes()

		self.call_latex()

		# Find the .aux files again since bibtex processing can require it
		self.hash_aux_nodes()

		self.bibtopic()
		self.bibfile()
		self.bibunits()
		self.makeindex()
		self.makeglossaries()

		for i in range(10):
			# There is no need to call latex again if the .aux hash value has not changed
			prev_hash = cur_hash
			cur_hash = self.hash_aux_nodes()
			if not cur_hash:
				Logs.error('No aux.h to process')
			if cur_hash and cur_hash == prev_hash:
				break

			# run the command
			Logs.info('calling %s' % self.__class__.__name__)
			self.call_latex()

	def hash_aux_nodes(self):
		try:
			nodes = self.aux_nodes
		except AttributeError:
			try:
				self.aux_nodes = self.scan_aux(self.inputs[0].change_ext('.aux'))
			except IOError:
				return None
		return Utils.h_list([Utils.h_file(x.abspath()) for x in self.aux_nodes])

	def call_latex(self):
		self.env.env = {}
		self.env.env.update(os.environ)
		self.env.env.update({'TEXINPUTS': self.texinputs()})
		self.env.SRCFILE = self.inputs[0].abspath()
		self.check_status('error when calling latex', self.texfun())


class latex(tex):
	texfun, vars = Task.compile_fun('${LATEX} ${LATEXFLAGS} ${SRCFILE}', shell=False)
class pdflatex(tex):
	texfun, vars =  Task.compile_fun('${PDFLATEX} ${PDFLATEXFLAGS} ${SRCFILE}', shell=False)
class xelatex(tex):
	texfun, vars = Task.compile_fun('${XELATEX} ${XELATEXFLAGS} ${SRCFILE}', shell=False)

class dvips(Task.Task):
	run_str = '${DVIPS} ${DVIPSFLAGS} ${SRC} -o ${TGT}'
	color   = 'BLUE'
	after   = ['latex', 'pdflatex', 'xelatex']

class dvipdf(Task.Task):
	run_str = '${DVIPDF} ${DVIPDFFLAGS} ${SRC} ${TGT}'
	color   = 'BLUE'
	after   = ['latex', 'pdflatex', 'xelatex']

class pdf2ps(Task.Task):
	run_str = '${PDF2PS} ${PDF2PSFLAGS} ${SRC} ${TGT}'
	color   = 'BLUE'
	after   = ['latex', 'pdflatex', 'xelatex']

@feature('tex')
@before_method('process_source')
def apply_tex(self):
	"""
	Create :py:class:`waflib.Tools.tex.tex` objects, and dvips/dvipdf/pdf2ps tasks if necessary (outs='ps', etc).
	"""
	if not getattr(self, 'type', None) in ('latex', 'pdflatex', 'xelatex'):
		self.type = 'pdflatex'

	tree = self.bld
	outs = Utils.to_list(getattr(self, 'outs', []))

	# prompt for incomplete files (else the batchmode is used)
	self.env['PROMPT_LATEX'] = getattr(self, 'prompt', 1)

	deps_lst = []

	if getattr(self, 'deps', None):
		deps = self.to_list(self.deps)
		for dep in deps:
			if isinstance(dep, str):
				n = self.path.find_resource(dep)
				if not n:
					self.bld.fatal('Could not find %r for %r' % (dep, self))
				if not n in deps_lst:
					deps_lst.append(n)
			elif isinstance(dep, Node.Node):
				deps_lst.append(dep)

	for node in self.to_nodes(self.source):

		if self.type == 'latex':
			task = self.create_task('latex', node, node.change_ext('.dvi'))
		elif self.type == 'pdflatex':
			task = self.create_task('pdflatex', node, node.change_ext('.pdf'))
		elif self.type == 'xelatex':
			task = self.create_task('xelatex', node, node.change_ext('.pdf'))

		task.env = self.env

		# add the manual dependencies
		if deps_lst:
			for n in deps_lst:
				if not n in task.dep_nodes:
					task.dep_nodes.append(n)

		# texinputs is a nasty beast
		if hasattr(self, 'texinputs_nodes'):
			task.texinputs_nodes = self.texinputs_nodes
		else:
			task.texinputs_nodes = [node.parent, node.parent.get_bld(), self.path, self.path.get_bld()]
			lst = os.environ.get('TEXINPUTS', '')
			if self.env.TEXINPUTS:
				lst += os.pathsep + self.env.TEXINPUTS
			if lst:
				lst = lst.split(os.pathsep)
			for x in lst:
				if x:
					if os.path.isabs(x):
						p = self.bld.root.find_node(x)
						if p:
							task.texinputs_nodes.append(p)
						else:
							Logs.error('Invalid TEXINPUTS folder %s' % x)
					else:
						Logs.error('Cannot resolve relative paths in TEXINPUTS %s' % x)

		if self.type == 'latex':
			if 'ps' in outs:
				tsk = self.create_task('dvips', task.outputs, node.change_ext('.ps'))
				tsk.env.env = dict(os.environ)
			if 'pdf' in outs:
				tsk = self.create_task('dvipdf', task.outputs, node.change_ext('.pdf'))
				tsk.env.env = dict(os.environ)
		elif self.type == 'pdflatex':
			if 'ps' in outs:
				self.create_task('pdf2ps', task.outputs, node.change_ext('.ps'))
	self.source = []

def configure(self):
	"""
	Try to find the programs tex, latex and others. Do not raise any error if they
	are not found.
	"""
	v = self.env
	for p in 'tex latex pdflatex xelatex bibtex dvips dvipdf ps2pdf makeindex pdf2ps makeglossaries'.split():
		try:
			self.find_program(p, var=p.upper())
		except self.errors.ConfigurationError:
			pass
	v['DVIPSFLAGS'] = '-Ppdf'

