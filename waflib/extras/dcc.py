#!/usr/bin/env python
# encoding: utf-8
# Jérôme Carretero, 2011 (zougloub)

from waflib import Configure, Options, Utils
from waflib.Tools import ccroot
from waflib.Configure import conf

@conf
def find_dcc(conf):
	cc = conf.find_program(['dcc'], var='CC', path_list=getattr(Options.options, 'diabbindir', ""))
	conf.env.CC_NAME = 'dcc'

@conf
def find_dld(conf):
	ld = conf.find_program(['dld'], var='LINK_CC', path_list=getattr(Options.options, 'diabbindir', ""))
	conf.env.LINK_CC_NAME = 'dld'

@conf
def find_dar(conf):
	ar = conf.find_program(['dar'], var='DAR', path_list=getattr(Options.options, 'diabbindir', ""))
	conf.env.AR = ar
	conf.env.AR_NAME = 'dar'
	conf.env.ARFLAGS = 'rcs'

@conf
def find_ddump(conf):
	prg = conf.find_program(['ddump'], var='DDUMP', path_list=getattr(Options.options, 'diabbindir', ""))

@conf
def dcc_common_flags(conf):
	v = conf.env
	v['CC_SRC_F']            = []
	v['CC_TGT_F']            = ['-c', '-o']

	# linker
	if not v['LINK_CC']: v['LINK_CC'] = v['CC']
	v['CCLNK_SRC_F']         = []
	v['CCLNK_TGT_F']         = ['-o']
	v['CPPPATH_ST']          = '-I%s'
	v['DEFINES_ST']          = '-D%s'

	v['LIB_ST']              = '-l:%s' # template for adding libs
	v['LIBPATH_ST']          = '-L%s' # template for adding libpaths
	v['STLIB_ST']            = '-l:%s'
	v['STLIBPATH_ST']        = '-L%s'
	v['RPATH_ST']            = '-Wl,-rpath,%s'
	#v['STLIB_MARKER']        = '-Wl,-Bstatic'

	# program
	v['cprogram_PATTERN']    = '%s.elf'

	# static lib
	v['LINKFLAGS_cstlib']    = ['-Wl,-Bstatic']
	v['cstlib_PATTERN']      = 'lib%s.a'

def configure(conf):
	conf.find_dcc()
	conf.find_dar()
	conf.find_dld()
	conf.find_ddump()
	conf.dcc_common_flags()
	conf.cc_load_tools()
	conf.cc_add_flags()
	conf.link_add_flags()

def options(opt):
	"""
	Add the ``--with-diab-bindir`` command-line options.
	"""
	opt.add_option('--with-diab-bindir', type='string', dest='diabbindir', help = 'Specify alternate diab bin folder', default="")

