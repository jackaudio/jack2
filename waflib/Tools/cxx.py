#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2005-2010 (ita)

"Base for c++ programs and libraries"

from waflib import TaskGen, Task
from waflib.Tools import c_preproc
from waflib.Tools.ccroot import link_task, stlink_task

@TaskGen.extension('.cpp','.cc','.cxx','.C','.c++')
def cxx_hook(self, node):
	"Bind the c++ file extensions to the creation of a :py:class:`waflib.Tools.cxx.cxx` instance"
	return self.create_compiled_task('cxx', node)

if not '.c' in TaskGen.task_gen.mappings:
	TaskGen.task_gen.mappings['.c'] = TaskGen.task_gen.mappings['.cpp']

class cxx(Task.Task):
	"Compile C++ files into object files"
	run_str = '${CXX} ${ARCH_ST:ARCH} ${CXXFLAGS} ${CPPFLAGS} ${FRAMEWORKPATH_ST:FRAMEWORKPATH} ${CPPPATH_ST:INCPATHS} ${DEFINES_ST:DEFINES} ${CXX_SRC_F}${SRC} ${CXX_TGT_F}${TGT[0].abspath()}'
	vars    = ['CXXDEPS'] # unused variable to depend on, just in case
	ext_in  = ['.h'] # set the build order easily by using ext_out=['.h']
	scan    = c_preproc.scan

class cxxprogram(link_task):
	"Link object files into a c++ program"
	run_str = '${LINK_CXX} ${LINKFLAGS} ${CXXLNK_SRC_F}${SRC} ${CXXLNK_TGT_F}${TGT[0].abspath()} ${RPATH_ST:RPATH} ${FRAMEWORKPATH_ST:FRAMEWORKPATH} ${FRAMEWORK_ST:FRAMEWORK} ${ARCH_ST:ARCH} ${STLIB_MARKER} ${STLIBPATH_ST:STLIBPATH} ${STLIB_ST:STLIB} ${SHLIB_MARKER} ${LIBPATH_ST:LIBPATH} ${LIB_ST:LIB} ${LDFLAGS}'
	vars    = ['LINKDEPS']
	ext_out = ['.bin']
	inst_to = '${BINDIR}'

class cxxshlib(cxxprogram):
	"Link object files into a c++ shared library"
	inst_to = '${LIBDIR}'

class cxxstlib(stlink_task):
	"Link object files into a c++ static library"
	pass # do not remove

