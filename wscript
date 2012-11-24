#! /usr/bin/env python
# encoding: utf-8
from __future__ import print_function

import os
import Utils
import Options
import subprocess
g_maxlen = 40
import shutil
import Task
import re
import Logs
import sys

VERSION='1.9.9.5'
APPNAME='jack'
JACK_API_VERSION = '0.1.0'

# these variables are mandatory ('/' are converted automatically)
top = '.'
out = 'build'

def display_msg(msg, status = None, color = None):
    sr = msg
    global g_maxlen
    g_maxlen = max(g_maxlen, len(msg))
    if status:
        Logs.pprint('NORMAL', "%s :" % msg.ljust(g_maxlen), sep=' ')
        Logs.pprint(color, status)
    else:
        print("%s" % msg.ljust(g_maxlen))

def display_feature(msg, build):
    if build:
        display_msg(msg, "yes", 'GREEN')
    else:
        display_msg(msg, "no", 'YELLOW')

def create_svnversion_task(bld, header='svnversion.h', define=None):
    cmd = '../svnversion_regenerate.sh ${TGT}'
    if define:
        cmd += " " + define

    def post_run(self):
        sg = Utils.h_file(self.outputs[0].abspath(self.env))
        #print sg.encode('hex')
        Build.bld.node_sigs[self.env.variant()][self.outputs[0].id] = sg

    bld(
            rule = cmd,
            name = 'svnversion',
            runnable_status = Task.RUN_ME,
            before = 'c',
            color = 'BLUE',
            post_run = post_run,
            target = [bld.path.find_or_declare(header)]
    )

def options(opt):
    # options provided by the modules
    opt.tool_options('compiler_cxx')
    opt.tool_options('compiler_cc')

    opt.add_option('--libdir', type='string', help="Library directory [Default: <prefix>/lib]")
    opt.add_option('--libdir32', type='string', help="32bit Library directory [Default: <prefix>/lib32]")
    opt.add_option('--mandir', type='string', help="Manpage directory [Default: <prefix>/share/man/man1]")
    opt.add_option('--dbus', action='store_true', default=False, help='Enable D-Bus JACK (jackdbus)')
    opt.add_option('--classic', action='store_true', default=False, help='Force enable standard JACK (jackd) even if D-Bus JACK (jackdbus) is enabled too')
    opt.add_option('--doxygen', action='store_true', default=False, help='Enable build of doxygen documentation')
    opt.add_option('--profile', action='store_true', default=False, help='Build with engine profiling')
    opt.add_option('--mixed', action='store_true', default=False, help='Build with 32/64 bits mixed mode')
    opt.add_option('--clients', default=64, type="int", dest="clients", help='Maximum number of JACK clients')
    opt.add_option('--ports-per-application', default=768, type="int", dest="application_ports", help='Maximum number of ports per application')
    opt.add_option('--debug', action='store_true', default=False, dest='debug', help='Build debuggable binaries')
    opt.add_option('--firewire', action='store_true', default=False, help='Enable FireWire driver (FFADO)')
    opt.add_option('--freebob', action='store_true', default=False, help='Enable FreeBob driver')
    opt.add_option('--alsa', action='store_true', default=False, help='Enable ALSA driver')
    opt.add_option('--autostart', type='string', default="default", help='Autostart method. Possible values: "default", "classic", "dbus", "none"')
    opt.sub_options('dbus')

def configure(conf):
    conf.load('compiler_cxx')
    conf.load('compiler_cc')
    platform = sys.platform
    conf.env['IS_MACOSX'] = platform == 'darwin'
    conf.env['IS_LINUX'] = platform == 'linux' or platform == 'linux2' or platform == 'posix'
    conf.env['IS_SUN'] = platform == 'sunos'
    # GNU/kFreeBSD and GNU/Hurd are treated as Linux
    if platform.startswith('gnu0') or platform.startswith('gnukfreebsd'):
        conf.env['IS_LINUX'] = True

    if conf.env['IS_LINUX']:
        Logs.pprint('CYAN', "Linux detected")

    if conf.env['IS_MACOSX']:
        Logs.pprint('CYAN', "MacOS X detected")

    if conf.env['IS_SUN']:
        Logs.pprint('CYAN', "SunOS detected")

    if conf.env['IS_LINUX']:
        conf.check_tool('compiler_cxx')
        conf.check_tool('compiler_cc')

    if conf.env['IS_MACOSX']:
        conf.check_tool('compiler_cxx')
        conf.check_tool('compiler_cc')

    # waf 1.5 : check_tool('compiler_cxx') and check_tool('compiler_cc') do not work correctly, so explicit use of gcc and g++
    if conf.env['IS_SUN']:
        conf.check_tool('g++')
        conf.check_tool('gcc')

    #if conf.env['IS_SUN']:
    #   conf.check_tool('compiler_cxx')
    #   conf.check_tool('compiler_cc')
 
    conf.env.append_unique('CXXFLAGS', '-Wall')
    conf.env.append_unique('CFLAGS', '-Wall')

    conf.sub_config('common')
    if conf.env['IS_LINUX']:
        conf.sub_config('linux')
        if Options.options.alsa and not conf.env['BUILD_DRIVER_ALSA']:
            conf.fatal('ALSA driver was explicitly requested but cannot be built')
        if Options.options.freebob and not conf.env['BUILD_DRIVER_FREEBOB']:
            conf.fatal('FreeBob driver was explicitly requested but cannot be built')
        if Options.options.firewire and not conf.env['BUILD_DRIVER_FFADO']:
            conf.fatal('FFADO driver was explicitly requested but cannot be built')
        conf.env['BUILD_DRIVER_ALSA'] = Options.options.alsa
        conf.env['BUILD_DRIVER_FFADO'] = Options.options.firewire
        conf.env['BUILD_DRIVER_FREEBOB'] = Options.options.freebob
    if Options.options.dbus:
        conf.sub_config('dbus')
        if conf.env['BUILD_JACKDBUS'] != True:
            conf.fatal('jackdbus was explicitly requested but cannot be built')

    conf.check_cc(header_name='samplerate.h', define_name="HAVE_SAMPLERATE")

    if conf.is_defined('HAVE_SAMPLERATE'):
        conf.env['LIB_SAMPLERATE'] = ['samplerate']

    conf.sub_config('example-clients')

    if conf.check_cfg(package='celt', atleast_version='0.11.0', args='--cflags --libs', mandatory=False):
        conf.define('HAVE_CELT', 1)
        conf.define('HAVE_CELT_API_0_11', 1)
        conf.define('HAVE_CELT_API_0_8', 0)
        conf.define('HAVE_CELT_API_0_7', 0)
        conf.define('HAVE_CELT_API_0_5', 0)
    elif conf.check_cfg(package='celt', atleast_version='0.8.0', args='--cflags --libs', mandatory=False):
        conf.define('HAVE_CELT', 1)
        conf.define('HAVE_CELT_API_0_11', 0)
        conf.define('HAVE_CELT_API_0_8', 1)
        conf.define('HAVE_CELT_API_0_7', 0)
        conf.define('HAVE_CELT_API_0_5', 0)
    elif conf.check_cfg(package='celt', atleast_version='0.7.0', args='--cflags --libs', mandatory=False):
        conf.define('HAVE_CELT', 1)
        conf.define('HAVE_CELT_API_0_11', 0)
        conf.define('HAVE_CELT_API_0_8', 0)
        conf.define('HAVE_CELT_API_0_7', 1)
        conf.define('HAVE_CELT_API_0_5', 0)
    elif conf.check_cfg(package='celt', atleast_version='0.5.0', args='--cflags --libs', mandatory=False):
        conf.define('HAVE_CELT', 1)
        conf.define('HAVE_CELT_API_0_11', 0)
        conf.define('HAVE_CELT_API_0_8', 0)
        conf.define('HAVE_CELT_API_0_7', 0)
        conf.define('HAVE_CELT_API_0_5', 1)
    else:
        conf.define('HAVE_CELT', 0)
        conf.define('HAVE_CELT_API_0_11', 0)
        conf.define('HAVE_CELT_API_0_8', 0)
        conf.define('HAVE_CELT_API_0_7', 0)
        conf.define('HAVE_CELT_API_0_5', 0)

    conf.env['WITH_OPUS'] = False
    if conf.check_cfg(package='opus', atleast_version='0.9.0' , args='--cflags --libs', mandatory=False):
        if conf.check_cc(header_name='opus/opus_custom.h', mandatory=False):
            conf.define('HAVE_OPUS', 1)
            conf.env['WITH_OPUS'] = True


    conf.env['LIB_PTHREAD'] = ['pthread']
    conf.env['LIB_DL'] = ['dl']
    conf.env['LIB_RT'] = ['rt']
    conf.env['LIB_M'] = ['m']
    conf.env['LIB_STDC++'] = ['stdc++']
    conf.env['JACK_API_VERSION'] = JACK_API_VERSION
    conf.env['JACK_VERSION'] = VERSION

    conf.env['BUILD_DOXYGEN_DOCS'] = Options.options.doxygen
    conf.env['BUILD_WITH_PROFILE'] = Options.options.profile
    conf.env['BUILD_WITH_32_64'] = Options.options.mixed
    conf.env['BUILD_CLASSIC'] = Options.options.classic
    conf.env['BUILD_DEBUG'] = Options.options.debug

    if conf.env['BUILD_JACKDBUS']:
        conf.env['BUILD_JACKD'] = conf.env['BUILD_CLASSIC']
    else:
        conf.env['BUILD_JACKD'] = True

    if Options.options.libdir:
        conf.env['LIBDIR'] = Options.options.libdir
    else:
        conf.env['LIBDIR'] = conf.env['PREFIX'] + '/lib'

    if Options.options.mandir:
        conf.env['MANDIR'] = Options.options.mandir
    else:
        conf.env['MANDIR'] = conf.env['PREFIX'] + '/share/man/man1'

    if conf.env['BUILD_DEBUG']:
        conf.env.append_unique('CXXFLAGS', '-g')
        conf.env.append_unique('CFLAGS', '-g')
        conf.env.append_unique('LINKFLAGS', '-g')

    if not Options.options.autostart in ["default", "classic", "dbus", "none"]:
        conf.fatal("Invalid autostart value \"" + Options.options.autostart + "\"")

    if Options.options.autostart == "default":
        if conf.env['BUILD_JACKDBUS'] == True and conf.env['BUILD_JACKD'] == False:
            conf.env['AUTOSTART_METHOD'] = "dbus"
        else:
            conf.env['AUTOSTART_METHOD'] = "classic"
    else:
        conf.env['AUTOSTART_METHOD'] = Options.options.autostart

    if conf.env['AUTOSTART_METHOD'] == "dbus" and not conf.env['BUILD_JACKDBUS']:
        conf.fatal("D-Bus autostart mode was specified but jackdbus will not be built")
    if conf.env['AUTOSTART_METHOD'] == "classic" and not conf.env['BUILD_JACKD']:
        conf.fatal("Classic autostart mode was specified but jackd will not be built")

    if conf.env['AUTOSTART_METHOD'] == "dbus":
        conf.define('USE_LIBDBUS_AUTOLAUNCH', 1)
    elif conf.env['AUTOSTART_METHOD'] == "classic":
        conf.define('USE_CLASSIC_AUTOLAUNCH', 1)

    conf.define('CLIENT_NUM', Options.options.clients)
    conf.define('PORT_NUM_FOR_CLIENT', Options.options.application_ports)

    conf.env['ADDON_DIR'] = os.path.normpath(os.path.join(conf.env['LIBDIR'], 'jack'))
    conf.define('ADDON_DIR', conf.env['ADDON_DIR'])
    conf.define('JACK_LOCATION', os.path.normpath(os.path.join(conf.env['PREFIX'], 'bin')))
    conf.define('USE_POSIX_SHM', 1)
    conf.define('JACKMP', 1)
    if conf.env['BUILD_JACKDBUS'] == True:
        conf.define('JACK_DBUS', 1)
    if conf.env['BUILD_WITH_PROFILE'] == True:
        conf.define('JACK_MONITOR', 1)
    conf.write_config_header('config.h', remove=False)

    svnrev = None
    if os.access('svnversion.h', os.R_OK):
        data = file('svnversion.h').read()
        m = re.match(r'^#define SVN_VERSION "([^"]*)"$', data)
        if m != None:
            svnrev = m.group(1)

    if Options.options.mixed == True:
        env_variant2 = conf.env.copy()
        conf.set_env_name('lib32', env_variant2)
        env_variant2.set_variant('lib32')
        conf.setenv('lib32')
        conf.env.append_unique('CXXFLAGS', '-m32')
        conf.env.append_unique('CFLAGS', '-m32')
        conf.env.append_unique('LINKFLAGS', '-m32')
        if Options.options.libdir32:
            conf.env['LIBDIR'] = Options.options.libdir32
        else:
            conf.env['LIBDIR'] = conf.env['PREFIX'] + '/lib32'
        conf.write_config_header('config.h')

    print()
    display_msg("==================")
    version_msg = "JACK " + VERSION
    if svnrev:
        version_msg += " exported from r" + svnrev
    else:
        version_msg += " svn revision will checked and eventually updated during build"
    print(version_msg)

    print("Build with a maximum of %d JACK clients" % Options.options.clients)
    print("Build with a maximum of %d ports per application" % Options.options.application_ports)
 
    display_msg("Install prefix", conf.env['PREFIX'], 'CYAN')
    display_msg("Library directory", conf.env['LIBDIR'], 'CYAN')
    display_msg("Drivers directory", conf.env['ADDON_DIR'], 'CYAN')
    display_feature('Build debuggable binaries', conf.env['BUILD_DEBUG'])
    display_msg('C compiler flags', repr(conf.env['CFLAGS']))
    display_msg('C++ compiler flags', repr(conf.env['CXXFLAGS']))
    display_msg('Linker flags', repr(conf.env['LINKFLAGS']))
    display_feature('Build doxygen documentation', conf.env['BUILD_DOXYGEN_DOCS'])
    display_feature('Build Opus netjack2', conf.env['WITH_OPUS'])
    display_feature('Build with engine profiling', conf.env['BUILD_WITH_PROFILE'])
    display_feature('Build with 32/64 bits mixed mode', conf.env['BUILD_WITH_32_64'])

    display_feature('Build standard JACK (jackd)', conf.env['BUILD_JACKD'])
    display_feature('Build D-Bus JACK (jackdbus)', conf.env['BUILD_JACKDBUS'])
    display_msg('Autostart method', conf.env['AUTOSTART_METHOD'])

    if conf.env['BUILD_JACKDBUS'] and conf.env['BUILD_JACKD']:
        print(Logs.colors.RED + 'WARNING !! mixing both jackd and jackdbus may cause issues:' + Logs.colors.NORMAL)
        print(Logs.colors.RED + 'WARNING !! jackdbus does not use .jackdrc nor qjackctl settings' + Logs.colors.NORMAL)

    if conf.env['IS_LINUX']:
        display_feature('Build with ALSA support', conf.env['BUILD_DRIVER_ALSA'] == True)
        display_feature('Build with FireWire (FreeBob) support', conf.env['BUILD_DRIVER_FREEBOB'] == True)
        display_feature('Build with FireWire (FFADO) support', conf.env['BUILD_DRIVER_FFADO'] == True)
       
    if conf.env['BUILD_JACKDBUS'] == True:
        display_msg('D-Bus service install directory', conf.env['DBUS_SERVICES_DIR'], 'CYAN')
        #display_msg('Settings persistence', xxx)

        if conf.env['DBUS_SERVICES_DIR'] != conf.env['DBUS_SERVICES_DIR_REAL']:
            print()
            print(Logs.colors.RED + "WARNING: D-Bus session services directory as reported by pkg-config is")
            print(Logs.colors.RED + "WARNING:", end=' ')
            print(Logs.colors.CYAN + conf.env['DBUS_SERVICES_DIR_REAL'])
            print(Logs.colors.RED + 'WARNING: but service file will be installed in')
            print(Logs.colors.RED + "WARNING:", end=' ')
            print(Logs.colors.CYAN + conf.env['DBUS_SERVICES_DIR'])
            print(Logs.colors.RED + 'WARNING: You may need to adjust your D-Bus configuration after installing jackdbus')
            print('WARNING: You can override dbus service install directory')
            print('WARNING: with --enable-pkg-config-dbus-service-dir option to this script')
            print(Logs.colors.NORMAL, end=' ')
    print()

def build(bld):
    print("make[1]: Entering directory `" + os.getcwd() + "/" + out + "'")
    if not os.access('svnversion.h', os.R_OK):
        create_svnversion_task(bld)

   # process subfolders from here
    bld.add_subdirs('common')
    if bld.env['IS_LINUX']:
        bld.add_subdirs('linux')
        bld.add_subdirs('example-clients')
        bld.add_subdirs('tests')
        bld.add_subdirs('man')
        if bld.env['BUILD_JACKDBUS'] == True:
           bld.add_subdirs('dbus')
  
    if bld.env['IS_MACOSX']:
        bld.add_subdirs('macosx')
        bld.add_subdirs('example-clients')
        bld.add_subdirs('tests')
        if bld.env['BUILD_JACKDBUS'] == True:
            bld.add_subdirs('dbus')

    if bld.env['IS_SUN']:
        bld.add_subdirs('solaris')
        bld.add_subdirs('example-clients')
        bld.add_subdirs('tests')
        if bld.env['BUILD_JACKDBUS'] == True:
            bld.add_subdirs('dbus')

    if bld.env['BUILD_DOXYGEN_DOCS'] == True:
        share_dir = bld.env.get_destdir() + bld.env['PREFIX'] + '/share/jack-audio-connection-kit'
        html_docs_source_dir = "build/default/html"
        html_docs_install_dir = share_dir + '/reference/html/'
        if Options.commands['install']:
            if os.path.isdir(html_docs_install_dir):
                Logs.pprint('CYAN', "Removing old doxygen documentation installation...")
                shutil.rmtree(html_docs_install_dir)
                Logs.pprint('CYAN', "Removing old doxygen documentation installation done.")
            Logs.pprint('CYAN', "Installing doxygen documentation...")
            shutil.copytree(html_docs_source_dir, html_docs_install_dir)
            Logs.pprint('CYAN', "Installing doxygen documentation done.")
        elif Options.commands['uninstall']:
            Logs.pprint('CYAN', "Uninstalling doxygen documentation...")
            if os.path.isdir(share_dir):
                shutil.rmtree(share_dir)
            Logs.pprint('CYAN', "Uninstalling doxygen documentation done.")
        elif Options.commands['clean']:
            if os.access(html_docs_source_dir, os.R_OK):
                Logs.pprint('CYAN', "Removing doxygen generated documentation...")
                shutil.rmtree(html_docs_source_dir)
                Logs.pprint('CYAN', "Removing doxygen generated documentation done.")
        elif Options.commands['build']:
            if not os.access(html_docs_source_dir, os.R_OK):
                os.popen("doxygen").read()
            else:
                Logs.pprint('CYAN', "doxygen documentation already built.")

def dist_hook():
    os.remove('svnversion_regenerate.sh')
    os.system('../svnversion_regenerate.sh svnversion.h')
