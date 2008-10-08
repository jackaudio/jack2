#! /usr/bin/env python
# encoding: utf-8

import os
import Params
import commands
from Configure import g_maxlen
#g_maxlen = 40
import shutil
import Task
import re

VERSION='1.9.0'
APPNAME='jack'
JACK_API_VERSION = '0.1.0'

# these variables are mandatory ('/' are converted automatically)
srcdir = '.'
blddir = 'build'

def display_msg(msg, status = None, color = None):
    sr = msg
    global g_maxlen
    g_maxlen = max(g_maxlen, len(msg))
    if status:
        print "%s :" % msg.ljust(g_maxlen),
        Params.pprint(color, status)
    else:
        print "%s" % msg.ljust(g_maxlen)

def display_feature(msg, build):
    if build:
        display_msg(msg, "yes", 'GREEN')
    else:
        display_msg(msg, "no", 'YELLOW')

def create_svnversion_gen(bld, header='svnversion.h', define=None):
    cmd = '../svnversion_regenerate.sh ${TGT}'
    if define:
        cmd += " " + define
    cls = Task.simple_task_type('svnversion', cmd, color='BLUE')
    cls.must_run = lambda self: True
    #cls.before = 'cxx'

    def sg(self):
        rt = Params.h_file(self.m_outputs[0].abspath(self.env()))
        return rt
    cls.signature = sg

    #def se(self):
    #    r = sg(self)
    #    return (r, r, r, r, r)
    #cls.cache_sig = property(sg, None)
    cls.cache_sig = None

    tsk = cls('svnversion', bld.env().copy())
    tsk.m_inputs = []
    tsk.m_outputs = [bld.path.find_or_declare(header)]
    tsk.prio = 1 # execute this task first

def set_options(opt):
    # options provided by the modules
    opt.tool_options('compiler_cxx')
    opt.tool_options('compiler_cc')

    opt.add_option('--dbus', action='store_true', default=False, help='Enable D-Bus JACK (jackdbus)')
    opt.add_option('--doxygen', action='store_true', default=False, help='Enable build of doxygen documentation')
    opt.add_option('--monitor', action='store_true', default=False, help='Build with monitoring records')
    opt.add_option('--clients', default=64, type="int", dest="clients", help='Maximum number of JACK clients')
    opt.add_option('--ports', default=512, type="int", dest="ports", help='Maximum number of ports')
    opt.sub_options('dbus')

def configure(conf):
    platform = conf.detect_platform()
    conf.env['IS_MACOSX'] = platform == 'darwin'
    conf.env['IS_LINUX'] = platform == 'linux'

    if conf.env['IS_LINUX']:
        Params.pprint('CYAN', "Linux detected")

    if conf.env['IS_MACOSX']:
        Params.pprint('CYAN', "MacOS X detected")

    conf.check_tool('compiler_cxx')
    conf.check_tool('compiler_cc')

    conf.sub_config('common')
    if conf.env['IS_LINUX']:
        conf.sub_config('linux')
    if Params.g_options.dbus:
        conf.sub_config('dbus')
    conf.sub_config('example-clients')

    conf.env['LIB_PTHREAD'] = ['pthread']
    conf.env['LIB_DL'] = ['dl']
    conf.env['LIB_RT'] = ['rt']
    conf.env['JACK_API_VERSION'] = JACK_API_VERSION
    conf.env['JACK_VERSION'] = VERSION

    conf.env['BUILD_DOXYGEN_DOCS'] = Params.g_options.doxygen
    conf.env['BUILD_WITH_MONITOR'] = Params.g_options.monitor

    conf.define('CLIENT_NUM', Params.g_options.clients)
    conf.define('PORT_NUM', Params.g_options. ports)

    conf.define('ADDON_DIR', os.path.normpath(conf.env['PREFIX'] + '/lib/jack'))
    conf.define('JACK_LOCATION', os.path.normpath(conf.env['PREFIX'] + '/bin'))
    conf.define('USE_POSIX_SHM', 1)
    conf.define('JACKMP', 1)
    if conf.env['BUILD_JACKDBUS'] == True:
        conf.define('JACK_DBUS', 1)
    if conf.env['BUILD_WITH_MONITOR'] == True:
        conf.define('JACK_MONITOR', 1)
    conf.write_config_header('config.h')

    svnrev = None
    if os.access('svnversion.h', os.R_OK):
        data = file('svnversion.h').read()
        m = re.match(r'^#define SVN_VERSION "([^"]*)"$', data)
        if m != None:
            svnrev = m.group(1)

    print
    display_msg("==================")
    version_msg = "JACK " + VERSION
    if svnrev:
        version_msg += " exported from r" + svnrev
    else:
        version_msg += " svn revision will checked and eventually updated during build"
    print version_msg

    print "Build with a maximum of %d JACK clients" % conf.env['CLIENT_NUM']
    print "Build with a maximum of %d ports" % conf.env['PORT_NUM']
 
    display_msg("Install prefix", conf.env['PREFIX'], 'CYAN')
    display_msg("Drivers directory", conf.env['ADDON_DIR'], 'CYAN')
    display_feature('Build doxygen documentation', conf.env['BUILD_DOXYGEN_DOCS'])
    display_feature('Build with monitoring records', conf.env['BUILD_WITH_MONITOR'])
    
    if conf.env['IS_LINUX']:
        display_feature('Build with ALSA support', conf.env['BUILD_DRIVER_ALSA'] == True)
        display_feature('Build with FireWire (FreeBob) support', conf.env['BUILD_DRIVER_FREEBOB'] == True)
        display_feature('Build with FireWire (FFADO) support', conf.env['BUILD_DRIVER_FFADO'] == True)
        display_feature('Build D-Bus JACK (jackdbus)', conf.env['BUILD_JACKDBUS'] == True)
    if conf.env['BUILD_JACKDBUS'] == True:
        display_msg('D-Bus service install directory', conf.env['DBUS_SERVICES_DIR'], 'CYAN')
        #display_msg('Settings persistence', xxx)

        if conf.env['DBUS_SERVICES_DIR'] != conf.env['DBUS-1_SESSION_BUS_SERVICES_DIR'][0]:
            print
            print Params.g_colors['RED'] + "WARNING: D-Bus session services directory as reported by pkg-config is"
            print Params.g_colors['RED'] + "WARNING:",
            print Params.g_colors['CYAN'] + conf.env['DBUS-1_SESSION_BUS_SERVICES_DIR'][0]
            print Params.g_colors['RED'] + 'WARNING: but service file will be installed in'
            print Params.g_colors['RED'] + "WARNING:",
            print Params.g_colors['CYAN'] + conf.env['DBUS_SERVICES_DIR']
            print Params.g_colors['RED'] + 'WARNING: You may need to adjust your D-Bus configuration after installing jackdbus'
            print 'WARNING: You can override dbus service install directory'
            print 'WARNING: with --enable-pkg-config-dbus-service-dir option to this script'
            print Params.g_colors['NORMAL'],
    print

def build(bld):
    if not os.access('svnversion.h', os.R_OK):
        create_svnversion_gen(bld)

    # process subfolders from here
    bld.add_subdirs('common')
    if bld.env()['IS_LINUX']:
        bld.add_subdirs('linux')
        if bld.env()['BUILD_JACKDBUS'] == True:
            bld.add_subdirs('dbus')
        bld.add_subdirs('example-clients')
        bld.add_subdirs('tests')
    if bld.env()['IS_MACOSX']:
        bld.add_subdirs('macosx')
        bld.add_subdirs('example-clients')
        bld.add_subdirs('tests')
        if bld.env()['BUILD_JACKDBUS'] == True:
            bld.add_subdirs('dbus')

    if bld.env()['BUILD_DOXYGEN_DOCS'] == True:
        share_dir = bld.env().get_destdir() + Params.g_build.env()['PREFIX'] + '/share/jack-audio-connection-kit'
        html_docs_source_dir = "build/default/html"
        html_docs_install_dir = share_dir + '/reference/html/'
        if Params.g_commands['install']:
            if os.path.isdir(html_docs_install_dir):
                Params.pprint('CYAN', "Removing old doxygen documentation installation...")
                shutil.rmtree(html_docs_install_dir)
                Params.pprint('CYAN', "Removing old doxygen documentation installation done.")
            Params.pprint('CYAN', "Installing doxygen documentation...")
            shutil.copytree(html_docs_source_dir, html_docs_install_dir)
            Params.pprint('CYAN', "Installing doxygen documentation done.")
        elif Params.g_commands['uninstall']:
            Params.pprint('CYAN', "Uninstalling doxygen documentation...")
            if os.path.isdir(share_dir):
                shutil.rmtree(share_dir)
            Params.pprint('CYAN', "Uninstalling doxygen documentation done.")
        elif Params.g_commands['clean']:
            if os.access(html_docs_source_dir, os.R_OK):
                Params.pprint('CYAN', "Removing doxygen generated documentation...")
                shutil.rmtree(html_docs_source_dir)
                Params.pprint('CYAN', "Removing doxygen generated documentation done.")
        elif Params.g_commands['build']:
            if not os.access(html_docs_source_dir, os.R_OK):
                os.popen("doxygen").read()
            else:
                Params.pprint('CYAN', "doxygen documentation already built.")

def dist_hook():
    os.remove('svnversion_regenerate.sh')
    os.system('../svnversion_regenerate.sh svnversion.h')
