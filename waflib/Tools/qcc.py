#!/usr/bin/env python

import os
import tempfile
import re

from waflib import Logs, Task, Utils
from waflib.Tools import ar
from waflib.Tools import c_config

def to_list(xx):
    if isinstance(xx, list):
        return xx
    else:
        return [xx]

def configure(conf):
    v = conf.env

    targetname = '-Vgcc_ntoarmv7le'

    # Use a tool-configured OS environment variable set if available
    environ = getattr(conf.env, 'env', os.environ)
    if len(environ) > 0:
        conf.find_program('qcc', environ=environ)
    else:
        conf.find_program('qcc')
    conf.find_ar()

    # Basic compiler executable
    v['CC']         = to_list(v['QCC']) + [targetname, '-lang-c']
    v['CXX']        = to_list(v['QCC']) + [targetname, '-lang-c++']
    v['CC_NAME']    = 'qcc'
    v['CXX_NAME']    = 'qcc'

    # Target binary format
    v['DEST_BINFMT']    = 'elf'

    # Other cookie-crumbs traditionally set by the Waf compiler tool
    v['DEST_CPU']       = 'arm'
    v['DEST_OS']        = 'qnx'

    # Source filename, destination filename
    v['CC_SRC_F']       = []
    v['CXX_SRC_F']      = list(v['CC_SRC_F'])
    v['CC_TGT_F']       = ['-c', '-o']
    v['CXX_TGT_F']      = list(v['CC_TGT_F'])
    v['CCLNK_SRC_F']    = []
    v['CXXLNK_SRC_F']   = list(v['CCLNK_SRC_F'])
    v['CCLNK_TGT_F']    = ['-o']
    v['CXXLNK_TGT_F']   = list(v['CCLNK_TGT_F'])

    # Linker
    v['LINK_CC']    = list(v['CC'])
    v['LINK_CXX']   = list(v['CXX'])

    # Search paths
    v['CPPPATH_ST']     = '-I%s'
    v['DEFINES_ST']     = '-D%s'
    v['LIB_ST']         = '-l%s'
    v['LIBPATH_ST']     = '-L%s'
    v['STLIB_ST']       = '-l%s'
    v['STLIBPATH_ST']   = '-L%s'

    # whole program
    v['cprogram_PATTERN']   = '%s'
    v['cxxprogram_PATTERN'] = '%s'
    v['SHLIB_MARKER']       = '-Bdynamic'
    v['STLIB_MARKER']       = '-Bstatic'

    # static library
    v['cstlib_PATTERN']     = 'lib%s.a'
    v['cxxstlib_PATTERN']   = 'lib%s.a'

    # shared library
    v['CFLAGS_cshlib']      = ['-shared']
    v['CXXFLAGS_cxxshlib']  = ['-shared']
    v['LINKFLAGS_cshlib']   = ['-shared']
    v['LINKFLAGS_cxxshlib'] = ['-shared']
    v['SONAME_ST']          = '-Wl,-h%s'
    v['cshlib_PATTERN']     = 'lib%s.so'
    v['cxxshlib_PATTERN']   = 'lib%s.so'

    # linker options
    v['RPATH_ST']   = '-Wl,-rpath,%s'

    # Basic waf language stuff
    conf.cc_load_tools()
    conf.cxx_load_tools()
    conf.cc_add_flags()
    conf.cxx_add_flags()
    conf.link_add_flags()
