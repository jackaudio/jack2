#! /usr/bin/env python
# encoding: utf-8

import Params
import commands

VERSION='1.9.0'
APPNAME='jack'

# these variables are mandatory ('/' are converted automatically)
srcdir = '.'
blddir = 'build'

def fetch_svn_revision(path):
    cmd = "LANG= "
    cmd += "svnversion "
    cmd += path
    return commands.getoutput(cmd)

def set_options(opt):
    # options provided by the modules
    opt.tool_options('compiler_cxx')
    opt.tool_options('compiler_cc')

def configure(conf):
    conf.check_tool('compiler_cxx')
    conf.check_tool('compiler_cc')

    conf.sub_config('linux/dbus')

    conf.env['LIB_PTHREAD'] = ['pthread']
    conf.env['LIB_DL'] = ['dl']
    conf.env['LIB_RT'] = ['rt']

    conf.define('ADDON_DIR', conf.env['PREFIX'] + '/lib/jack')
    conf.define('JACK_LOCATION', conf.env['PREFIX'] + '/bin')
    conf.define('SOCKET_RPC_FIFO_SEMA', 1)
    conf.define('__SMP__', 1)
    conf.define('USE_POSIX_SHM', 1)
    conf.define('JACK_SVNREVISION', fetch_svn_revision('.'))
    conf.write_config_header('config.h')

    #print Params.g_options
    #print conf.env

def build(bld):
    # process subfolders from here
    bld.add_subdirs('common')
    bld.add_subdirs('linux')

    bld.add_subdirs('linux/dbus')
