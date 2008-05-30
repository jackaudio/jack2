#! /usr/bin/env python
# encoding: utf-8

import Params

VERSION='1.9.0'
APPNAME='jack'

# these variables are mandatory ('/' are converted automatically)
srcdir = '.'
blddir = 'build'

def set_options(opt):
    # options provided by the modules
    opt.tool_options('compiler_cxx')
    opt.tool_options('compiler_cc')

    #opt.add_option('--dbus', action='store_true', default=False, help='Compile D-Bus JACK')

def configure(conf):
    conf.check_tool('compiler_cxx')
    conf.check_tool('compiler_cc')

    #if Params.g_options['dbus']:
    #    conf.sub_config('linux/dbus')

    conf.env['LIB_PTHREAD'] = ['pthread']
    conf.env['LIB_DL'] = ['dl']
    conf.env['LIB_RT'] = ['rt']

    conf.define('ADDON_DIR', '/blabla')
    conf.define('JACK_LOCATION', conf.env['PREFIX'] + '/bin')
    conf.define('SOCKET_RPC_FIFO_SEMA', 1)
    conf.define('__SMP__', 1)
    conf.define('USE_POSIX_SHM', 1)
    conf.write_config_header('config.h')

    #print Params.g_options
    #print conf.env

def build(bld):
    # process subfolders from here
    bld.add_subdirs([
        'common',
        'linux',
#        'linux/dbus',
        ])
