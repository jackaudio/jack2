#!/usr/bin/env python
# encoding: utf-8
# Tom Wambold tom5760 gmail.com 2009
# Thomas Nagy 2010

"""
Go as a language may look nice, but its toolchain is one of the worse a developer
has ever seen. It keeps changing though, and I would like to believe that it will get
better eventually, but the crude reality is that this tool and the examples are
getting broken every few months.

If you have been lured into trying to use Go, you should stick to their Makefiles.
"""

import os, platform

from waflib import Utils, Task, TaskGen
from waflib.TaskGen import feature, extension, after_method, before_method
from waflib.Tools.ccroot import link_task, stlink_task, propagate_uselib_vars, process_use

class go(Task.Task):
	run_str = '${GOC} ${GOCFLAGS} ${CPPPATH_ST:INCPATHS} -o ${TGT} ${SRC}'

class gopackage(stlink_task):
	run_str = '${GOP} grc ${TGT} ${SRC}'

class goprogram(link_task):
	run_str = '${GOL} ${GOLFLAGS} -o ${TGT} ${SRC}'
	inst_to = '${BINDIR}'
	chmod   = Utils.O755

class cgopackage(stlink_task):
	color   = 'YELLOW'
	inst_to = '${LIBDIR}'
	ext_in  = ['.go']
	ext_out = ['.a']

	def run(self):
		src_dir = self.generator.bld.path
		source  = self.inputs
		target  = self.outputs[0].change_ext('')

		#print ("--> %s" % self.outputs)
		#print ('++> %s' % self.outputs[1])
		bld_dir = self.outputs[1]
		bld_dir.mkdir()
		obj_dir = bld_dir.make_node('_obj')
		obj_dir.mkdir()

		bld_srcs = []
		for s in source:
			# FIXME: it seems gomake/cgo stumbles on filenames like a/b/c.go
			# -> for the time being replace '/' with '_'...
			#b = bld_dir.make_node(s.path_from(src_dir))
			b = bld_dir.make_node(s.path_from(src_dir).replace(os.sep,'_'))
			b.parent.mkdir()
			#print ('++> %s' % (s.path_from(src_dir),))
			try:
				try:os.remove(b.abspath())
				except Exception:pass
				os.symlink(s.abspath(), b.abspath())
			except Exception:
				# if no support for symlinks, copy the file from src
				b.write(s.read())
			bld_srcs.append(b)
			#print("--|> [%s]" % b.abspath())
			b.sig = Utils.h_file(b.abspath())
			pass
		#self.set_inputs(bld_srcs)
		#self.generator.bld.raw_deps[self.uid()] = [self.signature()] + bld_srcs
		makefile_node = bld_dir.make_node("Makefile")
		makefile_tmpl = '''\
# Copyright 2009 The Go Authors.  All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file. ---

include $(GOROOT)/src/Make.inc

TARG=%(target)s

GCIMPORTS= %(gcimports)s

CGOFILES=\\
\t%(source)s

CGO_CFLAGS= %(cgo_cflags)s

CGO_LDFLAGS= %(cgo_ldflags)s

include $(GOROOT)/src/Make.pkg

%%: install %%.go
	$(GC) $*.go
	$(LD) -o $@ $*.$O

''' % {
'gcimports': ' '.join(l for l in self.env['GOCFLAGS']),
'cgo_cflags' : ' '.join(l for l in self.env['GOCFLAGS']),
'cgo_ldflags': ' '.join(l for l in self.env['GOLFLAGS']),
'target': target.path_from(obj_dir),
'source': ' '.join([b.path_from(bld_dir) for b in bld_srcs])
}
		makefile_node.write(makefile_tmpl)
		#print ("::makefile: %s"%makefile_node.abspath())
		cmd = Utils.subst_vars('gomake ${GOMAKE_FLAGS}', self.env).strip()
		o = self.outputs[0].change_ext('.gomake.log')
		fout_node = bld_dir.find_or_declare(o.name)
		fout = open(fout_node.abspath(), 'w')
		rc = self.generator.bld.exec_command(
		 cmd,
		 stdout=fout,
		 stderr=fout,
		 cwd=bld_dir.abspath(),
		)
		if rc != 0:
			import waflib.Logs as msg
			msg.error('** error running [%s] (cgo-%s)' % (cmd, target))
			msg.error(fout_node.read())
			return rc
		self.generator.bld.read_stlib(
		 target,
		 paths=[obj_dir.abspath(),],
		)
		tgt = self.outputs[0]
		if tgt.parent != obj_dir:
			install_dir = os.path.join('${LIBDIR}',
				tgt.parent.path_from(obj_dir))
		else:
			install_dir = '${LIBDIR}'
		#print('===> %s (%s)' % (tgt.abspath(), install_dir))
		self.generator.bld.install_files(
		 install_dir,
		 tgt.abspath(),
		 relative_trick=False,
		 postpone=False,
		)
		return rc

@extension('.go')
def compile_go(self, node):
	#print('*'*80, self.name)
	if not ('cgopackage' in self.features):
		return self.create_compiled_task('go', node)
	#print ('compile_go-cgo...')
	bld_dir = node.parent.get_bld()
	obj_dir = bld_dir.make_node('_obj')
	target  = obj_dir.make_node(node.change_ext('.a').name)
	return self.create_task('cgopackage', node, node.change_ext('.a'))

@feature('gopackage', 'goprogram', 'cgopackage')
@before_method('process_source')
def go_compiler_is_foobar(self):
	if self.env.GONAME == 'gcc':
		return
	self.source = self.to_nodes(self.source)
	src = []
	go = []
	for node in self.source:
		if node.name.endswith('.go'):
			go.append(node)
		else:
			src.append(node)
	self.source = src
	if not ('cgopackage' in self.features):
		#print('--> [%s]... (%s)' % (go[0], getattr(self, 'target', 'N/A')))
		tsk = self.create_compiled_task('go', go[0])
		tsk.inputs.extend(go[1:])
	else:
		#print ('+++ [%s] +++' % self.target)
		bld_dir = self.path.get_bld().make_node('cgopackage--%s' % self.target.replace(os.sep,'_'))
		obj_dir = bld_dir.make_node('_obj')
		target  = obj_dir.make_node(self.target+'.a')
		tsk = self.create_task('cgopackage', go, [target, bld_dir])
		self.link_task = tsk

@feature('gopackage', 'goprogram', 'cgopackage')
@after_method('process_source', 'apply_incpaths',)
def go_local_libs(self):
	names = self.to_list(getattr(self, 'use', []))
	#print ('== go-local-libs == [%s] == use: %s' % (self.name, names))
	for name in names:
		tg = self.bld.get_tgen_by_name(name)
		if not tg:
			raise Utils.WafError('no target of name %r necessary for %r in go uselib local' % (name, self))
		tg.post()
		#print ("-- tg[%s]: %s" % (self.name,name))
		lnk_task = getattr(tg, 'link_task', None)
		if lnk_task:
			for tsk in self.tasks:
				if isinstance(tsk, (go, gopackage, cgopackage)):
					tsk.set_run_after(lnk_task)
					tsk.dep_nodes.extend(lnk_task.outputs)
			path = lnk_task.outputs[0].parent.abspath()
			if isinstance(lnk_task, (go, gopackage)):
				# handle hierarchical packages
				path = lnk_task.generator.path.get_bld().abspath()
			elif isinstance(lnk_task, (cgopackage,)):
				# handle hierarchical cgopackages
				cgo_obj_dir = lnk_task.outputs[1].find_or_declare('_obj')
				path = cgo_obj_dir.abspath()
			# recursively add parent GOCFLAGS...
			self.env.append_unique('GOCFLAGS',
			 getattr(lnk_task.env, 'GOCFLAGS',[]))
			# ditto for GOLFLAGS...
			self.env.append_unique('GOLFLAGS',
			 getattr(lnk_task.env, 'GOLFLAGS',[]))
			self.env.append_unique('GOCFLAGS', ['-I%s' % path])
			self.env.append_unique('GOLFLAGS', ['-L%s' % path])
		for n in getattr(tg, 'includes_nodes', []):
			self.env.append_unique('GOCFLAGS', ['-I%s' % n.abspath()])
		pass
	pass

def configure(conf):

	def set_def(var, val):
		if not conf.env[var]:
			conf.env[var] = val

	goarch = os.getenv('GOARCH')
	if goarch == '386':
		set_def('GO_PLATFORM', 'i386')
	elif goarch == 'amd64':
		set_def('GO_PLATFORM', 'x86_64')
	elif goarch == 'arm':
		set_def('GO_PLATFORM', 'arm')
	else:
		set_def('GO_PLATFORM', platform.machine())

	if conf.env.GO_PLATFORM == 'x86_64':
		set_def('GO_COMPILER', '6g')
		set_def('GO_LINKER', '6l')
	elif conf.env.GO_PLATFORM in ('i386', 'i486', 'i586', 'i686'):
		set_def('GO_COMPILER', '8g')
		set_def('GO_LINKER', '8l')
	elif conf.env.GO_PLATFORM == 'arm':
		set_def('GO_COMPILER', '5g')
		set_def('GO_LINKER', '5l')
		set_def('GO_EXTENSION', '.5')

	if not (conf.env.GO_COMPILER or conf.env.GO_LINKER):
		raise conf.fatal('Unsupported platform ' + platform.machine())

	set_def('GO_PACK', 'gopack')
	set_def('gopackage_PATTERN', '%s.a')
	set_def('CPPPATH_ST', '-I%s')

	set_def('GOMAKE_FLAGS', ['--quiet'])
	conf.find_program(conf.env.GO_COMPILER, var='GOC')
	conf.find_program(conf.env.GO_LINKER,   var='GOL')
	conf.find_program(conf.env.GO_PACK,     var='GOP')

	conf.find_program('cgo',                var='CGO')

TaskGen.feature('go')(process_use)
TaskGen.feature('go')(propagate_uselib_vars)

