#! /usr/bin/python3
# encoding: utf-8
from __future__ import print_function

import os
import subprocess
import shutil
import re
import sys

from waflib import Logs, Options, Task, Utils
from waflib.Build import BuildContext, CleanContext, InstallContext, UninstallContext

VERSION='1.9.19'
APPNAME='jack'
JACK_API_VERSION = '0.1.0'

# these variables are mandatory ('/' are converted automatically)
top = '.'
out = 'build'

# lib32 variant name used when building in mixed mode
lib32 = 'lib32'

def display_feature(conf, msg, build):
    if build:
        conf.msg(msg, 'yes', color='GREEN')
    else:
        conf.msg(msg, 'no', color='YELLOW')

def check_for_celt(conf):
    found = False
    for version in ['11', '8', '7', '5']:
        define = 'HAVE_CELT_API_0_' + version
        if not found:
            try:
                conf.check_cfg(
                        package='celt >= 0.%s.0' % version,
                        args='--cflags --libs')
                found = True
                conf.define(define, 1)
                continue
            except conf.errors.ConfigurationError:
                pass
        conf.define(define, 0)

    if not found:
        raise conf.errors.ConfigurationError

def options(opt):
    # options provided by the modules
    opt.load('compiler_cxx')
    opt.load('compiler_c')
    opt.load('autooptions');

    opt.load('xcode6')

    opt.recurse('compat')

    # install directories
    opt.add_option('--htmldir', type='string', default=None, help='HTML documentation directory [Default: <prefix>/share/jack-audio-connection-kit/reference/html/')
    opt.add_option('--libdir', type='string', help='Library directory [Default: <prefix>/lib]')
    opt.add_option('--libdir32', type='string', help='32bit Library directory [Default: <prefix>/lib32]')
    opt.add_option('--pkgconfigdir', type='string', help='pkg-config file directory [Default: <libdir>/pkgconfig]')
    opt.add_option('--mandir', type='string', help='Manpage directory [Default: <prefix>/share/man/man1]')

    # options affecting binaries
    opt.add_option('--platform', type='string', default=sys.platform, help='Target platform for cross-compiling, e.g. cygwin or win32')
    opt.add_option('--mixed', action='store_true', default=False, help='Build with 32/64 bits mixed mode')
    opt.add_option('--debug', action='store_true', default=False, dest='debug', help='Build debuggable binaries')
    opt.add_option('--static', action='store_true', default=False, dest='static', help='Build static binaries (Windows only)')

    # options affecting general jack functionality
    opt.add_option('--classic', action='store_true', default=False, help='Force enable standard JACK (jackd) even if D-Bus JACK (jackdbus) is enabled too')
    opt.add_option('--dbus', action='store_true', default=False, help='Enable D-Bus JACK (jackdbus)')
    opt.add_option('--autostart', type='string', default='default', help='Autostart method. Possible values: "default", "classic", "dbus", "none"')
    opt.add_option('--profile', action='store_true', default=False, help='Build with engine profiling')
    opt.add_option('--clients', default=256, type='int', dest='clients', help='Maximum number of JACK clients')
    opt.add_option('--ports-per-application', default=2048, type='int', dest='application_ports', help='Maximum number of ports per application')
    opt.add_option('--systemd-unit', action='store_true', default=False, help='Install systemd units.')

    opt.set_auto_options_define('HAVE_%s')
    opt.set_auto_options_style('yesno_and_hack')

    # options with third party dependencies
    doxygen = opt.add_auto_option(
            'doxygen',
            help='Build doxygen documentation',
            conf_dest='BUILD_DOXYGEN_DOCS',
            default=False)
    doxygen.find_program('doxygen')
    alsa = opt.add_auto_option(
            'alsa',
            help='Enable ALSA driver',
            conf_dest='BUILD_DRIVER_ALSA')
    alsa.check_cfg(
            package='alsa >= 1.0.18',
            args='--cflags --libs')
    firewire = opt.add_auto_option(
            'firewire',
            help='Enable FireWire driver (FFADO)',
            conf_dest='BUILD_DRIVER_FFADO')
    firewire.check_cfg(
            package='libffado >= 1.999.17',
            args='--cflags --libs')
    iio = opt.add_auto_option(
            'iio',
            help='Enable IIO driver',
            conf_dest='BUILD_DRIVER_IIO')
    iio.check_cfg(
            package='gtkIOStream >= 1.4.0',
            args='--cflags --libs')
    iio.check_cfg(
            package='eigen3 >= 3.1.2',
            args='--cflags --libs')
    portaudio = opt.add_auto_option(
            'portaudio',
            help='Enable Portaudio driver',
            conf_dest='BUILD_DRIVER_PORTAUDIO')
    portaudio.check(header_name='windows.h') # only build portaudio on windows
    portaudio.check_cfg(
            package='portaudio-2.0 >= 19',
            uselib_store='PORTAUDIO',
            args='--cflags --libs')
    winmme = opt.add_auto_option(
            'winmme',
            help='Enable WinMME driver',
            conf_dest='BUILD_DRIVER_WINMME')
    winmme.check(
            header_name=['windows.h', 'mmsystem.h'],
            msg='Checking for header mmsystem.h')

    celt = opt.add_auto_option(
            'celt',
            help='Build with CELT')
    celt.add_function(check_for_celt)

    # Suffix _PKG to not collide with HAVE_OPUS defined by the option.
    opus = opt.add_auto_option(
            'opus',
            help='Build Opus netjack2')
    opus.check(header_name='opus/opus_custom.h')
    opus.check_cfg(
            package='opus >= 0.9.0',
            args='--cflags --libs',
            define_name='HAVE_OPUS_PKG')

    samplerate = opt.add_auto_option(
            'samplerate',
            help='Build with libsamplerate')
    samplerate.check_cfg(
            package='samplerate',
            args='--cflags --libs')
    sndfile = opt.add_auto_option(
            'sndfile',
            help='Build with libsndfile')
    sndfile.check_cfg(
            package='sndfile',
            args='--cflags --libs')
    readline = opt.add_auto_option(
            'readline',
            help='Build with readline')
    readline.check(lib='readline')
    readline.check(
            header_name=['stdio.h', 'readline/readline.h'],
            msg='Checking for header readline/readline.h')
    sd = opt.add_auto_option(
            'systemd',
            help='Use systemd notify')
    sd.check(header_name='systemd/sd-daemon.h')
    sd.check(lib='systemd')
    db = opt.add_auto_option(
            'db',
            help='Use Berkeley DB (metadata)')
    db.check(header_name='db.h')
    db.check(lib='db')
    zalsa = opt.add_auto_option(
            'zalsa',
            help='Build internal zita-a2j/j2a client')
    zalsa.check(lib='zita-alsa-pcmi')
    zalsa.check(lib='zita-resampler')

    # dbus options
    opt.recurse('dbus')

    # this must be called before the configure phase
    opt.apply_auto_options_hack()

def detect_platform(conf):
    # GNU/kFreeBSD and GNU/Hurd are treated as Linux
    platforms = [
        # ('KEY, 'Human readable name', ['strings', 'to', 'check', 'for'])
        ('IS_LINUX',   'Linux',   ['gnu0', 'gnukfreebsd', 'linux', 'posix']),
        ('IS_MACOSX',  'MacOS X', ['darwin']),
        ('IS_SUN',     'SunOS',   ['sunos']),
        ('IS_WINDOWS', 'Windows', ['cygwin', 'msys', 'win32'])
    ]

    for key,name,strings in platforms:
        conf.env[key] = False

    conf.start_msg('Checking platform')
    platform = Options.options.platform
    for key,name,strings in platforms:
        for s in strings:
            if platform.startswith(s):
                conf.env[key] = True
                conf.end_msg(name, color='CYAN')
                break


def configure(conf):
    conf.load('compiler_cxx')
    conf.load('compiler_c')

    detect_platform(conf)

    if conf.env['IS_WINDOWS']:
        conf.env.append_unique('CCDEFINES', '_POSIX')
        conf.env.append_unique('CXXDEFINES', '_POSIX')
        if Options.options.platform == 'msys':
            conf.env.append_value('INCLUDES', ['/mingw64/include'])
            conf.check(
                header_name='asio.h',
                includes='/opt/asiosdk/common',
                msg='Checking for ASIO SDK',
                define_name='HAVE_ASIO',
                mandatory=False)

    conf.env.append_unique('CFLAGS', '-Wall')
    conf.env.append_unique('CXXFLAGS', ['-Wall', '-Wno-invalid-offsetof'])
    conf.env.append_unique('CXXFLAGS', '-std=gnu++11')

    if not conf.env['IS_MACOSX']:
        conf.env.append_unique('LDFLAGS', '-Wl,--no-undefined')
    else:
        conf.check(lib='aften', uselib='AFTEN', define_name='AFTEN')
        conf.check_cxx(
            fragment=''
                + '#include <aften/aften.h>\n'
                + 'int\n'
                + 'main(void)\n'
                + '{\n'
                + 'AftenContext fAftenContext;\n'
                + 'aften_set_defaults(&fAftenContext);\n'
                + 'unsigned char *fb;\n'
                + 'float *buf=new float[10];\n'
                + 'int res = aften_encode_frame(&fAftenContext, fb, buf, 1);\n'
                + '}\n',
            lib='aften',
            msg='Checking for aften_encode_frame()',
            define_name='HAVE_AFTEN_NEW_API',
            mandatory=False)

        # TODO
        conf.env.append_unique('CXXFLAGS', '-Wno-deprecated-register')

    conf.load('autooptions')

    conf.recurse('compat')

    # Check for functions.
    conf.check(
            fragment=''
                + '#define _GNU_SOURCE\n'
                + '#include <poll.h>\n'
                + '#include <signal.h>\n'
                + '#include <stddef.h>\n'
                + 'int\n'
                + 'main(void)\n'
                + '{\n'
                + '   ppoll(NULL, 0, NULL, NULL);\n'
                + '}\n',
            msg='Checking for ppoll',
            define_name='HAVE_PPOLL',
            mandatory=False)

    # Check for backtrace support
    conf.check(
        header_name='execinfo.h',
        define_name='HAVE_EXECINFO_H',
        mandatory=False)

    conf.recurse('common')
    if Options.options.dbus:
        conf.recurse('dbus')
        if conf.env['BUILD_JACKDBUS'] != True:
            conf.fatal('jackdbus was explicitly requested but cannot be built')
    if conf.env['IS_LINUX']:
        if Options.options.systemd_unit:
            conf.recurse('systemd')
        else:
            conf.env['SYSTEMD_USER_UNIT_DIR'] = None


    conf.recurse('example-clients')
    conf.recurse('tools')

    # test for the availability of ucontext, and how it should be used
    for t in ['gp_regs', 'uc_regs', 'mc_gregs', 'gregs']:
        fragment = '#include <ucontext.h>\n'
        fragment += 'int main() { ucontext_t *ucontext; return (int) ucontext->uc_mcontext.%s[0]; }' % t
        confvar = 'HAVE_UCONTEXT_%s' % t.upper()
        conf.check_cc(fragment=fragment, define_name=confvar, mandatory=False,
                      msg='Checking for ucontext->uc_mcontext.%s' % t)
        if conf.is_defined(confvar):
            conf.define('HAVE_UCONTEXT', 1)

    fragment = '#include <ucontext.h>\n'
    fragment += 'int main() { return NGREG; }'
    conf.check_cc(fragment=fragment, define_name='HAVE_NGREG', mandatory=False,
                  msg='Checking for NGREG')

    conf.env['LIB_PTHREAD'] = ['pthread']
    conf.env['LIB_DL'] = ['dl']
    conf.env['LIB_RT'] = ['rt']
    conf.env['LIB_M'] = ['m']
    conf.env['LIB_STDC++'] = ['stdc++']
    conf.env['JACK_API_VERSION'] = JACK_API_VERSION
    conf.env['JACK_VERSION'] = VERSION

    conf.env['BUILD_WITH_PROFILE'] = Options.options.profile
    conf.env['BUILD_WITH_32_64'] = Options.options.mixed
    conf.env['BUILD_CLASSIC'] = Options.options.classic
    conf.env['BUILD_DEBUG'] = Options.options.debug
    conf.env['BUILD_STATIC'] = Options.options.static

    if conf.env['BUILD_JACKDBUS']:
        conf.env['BUILD_JACKD'] = conf.env['BUILD_CLASSIC']
    else:
        conf.env['BUILD_JACKD'] = True

    conf.env['BINDIR'] = conf.env['PREFIX'] + '/bin'

    if Options.options.htmldir:
        conf.env['HTMLDIR'] = Options.options.htmldir
    else:
        # set to None here so that the doxygen code can find out the highest
        # directory to remove upon install
        conf.env['HTMLDIR'] = None

    if Options.options.libdir:
        conf.env['LIBDIR'] = Options.options.libdir
    else:
        conf.env['LIBDIR'] = conf.env['PREFIX'] + '/lib'

    if Options.options.pkgconfigdir:
        conf.env['PKGCONFDIR'] = Options.options.pkgconfigdir
    else:
        conf.env['PKGCONFDIR'] = conf.env['LIBDIR'] + '/pkgconfig'

    if Options.options.mandir:
        conf.env['MANDIR'] = Options.options.mandir
    else:
        conf.env['MANDIR'] = conf.env['PREFIX'] + '/share/man/man1'

    if conf.env['BUILD_DEBUG']:
        conf.env.append_unique('CXXFLAGS', '-g')
        conf.env.append_unique('CFLAGS', '-g')
        conf.env.append_unique('LINKFLAGS', '-g')

    if not Options.options.autostart in ['default', 'classic', 'dbus', 'none']:
        conf.fatal('Invalid autostart value "' + Options.options.autostart + '"')

    if Options.options.autostart == 'default':
        if conf.env['BUILD_JACKD']:
            conf.env['AUTOSTART_METHOD'] = 'classic'
        else:
            conf.env['AUTOSTART_METHOD'] = 'dbus'
    else:
        conf.env['AUTOSTART_METHOD'] = Options.options.autostart

    if conf.env['AUTOSTART_METHOD'] == 'dbus' and not conf.env['BUILD_JACKDBUS']:
        conf.fatal('D-Bus autostart mode was specified but jackdbus will not be built')
    if conf.env['AUTOSTART_METHOD'] == 'classic' and not conf.env['BUILD_JACKD']:
        conf.fatal('Classic autostart mode was specified but jackd will not be built')

    if conf.env['AUTOSTART_METHOD'] == 'dbus':
        conf.define('USE_LIBDBUS_AUTOLAUNCH', 1)
    elif conf.env['AUTOSTART_METHOD'] == 'classic':
        conf.define('USE_CLASSIC_AUTOLAUNCH', 1)

    conf.define('CLIENT_NUM', Options.options.clients)
    conf.define('PORT_NUM_FOR_CLIENT', Options.options.application_ports)

    if conf.env['IS_WINDOWS']:
        # we define this in the environment to maintain compatibility with
        # existing install paths that use ADDON_DIR rather than have to
        # have special cases for windows each time.
        conf.env['ADDON_DIR'] = conf.env['LIBDIR'] + '/jack'
        if Options.options.platform == 'msys':
            conf.define('ADDON_DIR', 'jack')
            conf.define('__STDC_FORMAT_MACROS', 1) # for PRIu64
        else:
            # don't define ADDON_DIR in config.h, use the default 'jack' defined in
            # windows/JackPlatformPlug_os.h
            pass
    else:
        conf.env['ADDON_DIR'] = os.path.normpath(os.path.join(conf.env['LIBDIR'], 'jack'))
        conf.define('ADDON_DIR', conf.env['ADDON_DIR'])
        conf.define('JACK_LOCATION', os.path.normpath(os.path.join(conf.env['PREFIX'], 'bin')))

    if not conf.env['IS_WINDOWS']:
        conf.define('USE_POSIX_SHM', 1)
    conf.define('JACKMP', 1)
    if conf.env['BUILD_JACKDBUS']:
        conf.define('JACK_DBUS', 1)
    if conf.env['BUILD_WITH_PROFILE']:
        conf.define('JACK_MONITOR', 1)
    conf.write_config_header('config.h', remove=False)

    svnrev = None
    try:
        f = open('svnversion.h')
        data = f.read()
        m = re.match(r'^#define SVN_VERSION "([^"]*)"$', data)
        if m != None:
            svnrev = m.group(1)
        f.close()
    except IOError:
        pass

    if Options.options.mixed:
        conf.setenv(lib32, env=conf.env.derive())
        conf.env.append_unique('CFLAGS', '-m32')
        conf.env.append_unique('CXXFLAGS', '-m32')
        conf.env.append_unique('CXXFLAGS', '-DBUILD_WITH_32_64')
        conf.env.append_unique('LINKFLAGS', '-m32')
        if Options.options.libdir32:
            conf.env['LIBDIR'] = Options.options.libdir32
        else:
            conf.env['LIBDIR'] = conf.env['PREFIX'] + '/lib32'

        if conf.env['IS_WINDOWS'] and conf.env['BUILD_STATIC']:
            def replaceFor32bit(env):
                for e in env: yield e.replace('x86_64', 'i686', 1)
            for env in ('AR', 'CC', 'CXX', 'LINK_CC', 'LINK_CXX'):
                conf.all_envs[lib32][env] = list(replaceFor32bit(conf.all_envs[lib32][env]))
            conf.all_envs[lib32]['LIB_REGEX'] = ['tre32']

        # libdb does not work in mixed mode
        conf.all_envs[lib32]['HAVE_DB'] = 0
        conf.all_envs[lib32]['HAVE_DB_H'] = 0
        conf.all_envs[lib32]['LIB_DB'] = []
        # no need for opus in 32bit mixed mode clients
        conf.all_envs[lib32]['LIB_OPUS'] = []
        # someone tell me where this file gets written please..
        conf.write_config_header('config.h')

    print()
    print('==================')
    version_msg = 'JACK ' + VERSION
    if svnrev:
        version_msg += ' exported from r' + svnrev
    else:
        version_msg += ' svn revision will checked and eventually updated during build'
    print(version_msg)

    conf.msg('Maximum JACK clients', Options.options.clients, color='NORMAL')
    conf.msg('Maximum ports per application', Options.options.application_ports, color='NORMAL')

    conf.msg('Install prefix', conf.env['PREFIX'], color='CYAN')
    conf.msg('Library directory', conf.all_envs['']['LIBDIR'], color='CYAN')
    if conf.env['BUILD_WITH_32_64']:
        conf.msg('32-bit library directory', conf.all_envs[lib32]['LIBDIR'], color='CYAN')
    conf.msg('Drivers directory', conf.env['ADDON_DIR'], color='CYAN')
    display_feature(conf, 'Build debuggable binaries', conf.env['BUILD_DEBUG'])

    tool_flags = [
        ('C compiler flags',   ['CFLAGS', 'CPPFLAGS']),
        ('C++ compiler flags', ['CXXFLAGS', 'CPPFLAGS']),
        ('Linker flags',       ['LINKFLAGS', 'LDFLAGS'])
    ]
    for name,vars in tool_flags:
        flags = []
        for var in vars:
            flags += conf.all_envs[''][var]
        conf.msg(name, repr(flags), color='NORMAL')

    if conf.env['BUILD_WITH_32_64']:
        conf.msg('32-bit C compiler flags', repr(conf.all_envs[lib32]['CFLAGS']))
        conf.msg('32-bit C++ compiler flags', repr(conf.all_envs[lib32]['CXXFLAGS']))
        conf.msg('32-bit linker flags', repr(conf.all_envs[lib32]['LINKFLAGS']))
    display_feature(conf, 'Build with engine profiling', conf.env['BUILD_WITH_PROFILE'])
    display_feature(conf, 'Build with 32/64 bits mixed mode', conf.env['BUILD_WITH_32_64'])

    display_feature(conf, 'Build standard JACK (jackd)', conf.env['BUILD_JACKD'])
    display_feature(conf, 'Build D-Bus JACK (jackdbus)', conf.env['BUILD_JACKDBUS'])
    conf.msg('Autostart method', conf.env['AUTOSTART_METHOD'])

    if conf.env['BUILD_JACKDBUS'] and conf.env['BUILD_JACKD']:
        print(Logs.colors.RED + 'WARNING !! mixing both jackd and jackdbus may cause issues:' + Logs.colors.NORMAL)
        print(Logs.colors.RED + 'WARNING !! jackdbus does not use .jackdrc nor qjackctl settings' + Logs.colors.NORMAL)

    conf.summarize_auto_options()

    if conf.env['BUILD_JACKDBUS']:
        conf.msg('D-Bus service install directory', conf.env['DBUS_SERVICES_DIR'], color='CYAN')

        if conf.env['DBUS_SERVICES_DIR'] != conf.env['DBUS_SERVICES_DIR_REAL']:
            print()
            print(Logs.colors.RED + 'WARNING: D-Bus session services directory as reported by pkg-config is')
            print(Logs.colors.RED + 'WARNING:', end=' ')
            print(Logs.colors.CYAN + conf.env['DBUS_SERVICES_DIR_REAL'])
            print(Logs.colors.RED + 'WARNING: but service file will be installed in')
            print(Logs.colors.RED + 'WARNING:', end=' ')
            print(Logs.colors.CYAN + conf.env['DBUS_SERVICES_DIR'])
            print(Logs.colors.RED + 'WARNING: You may need to adjust your D-Bus configuration after installing jackdbus')
            print('WARNING: You can override dbus service install directory')
            print('WARNING: with --enable-pkg-config-dbus-service-dir option to this script')
            print(Logs.colors.NORMAL, end=' ')
    print()

def init(ctx):
    for y in (BuildContext, CleanContext, InstallContext, UninstallContext):
        name = y.__name__.replace('Context','').lower()
        class tmp(y):
            cmd = name + '_' + lib32
            variant = lib32

def obj_add_includes(bld, obj):
    if bld.env['BUILD_JACKDBUS']:
        obj.includes += ['dbus']

    if bld.env['IS_LINUX']:
        obj.includes += ['linux', 'posix']

    if bld.env['IS_MACOSX']:
        obj.includes += ['macosx', 'posix']

    if bld.env['IS_SUN']:
        obj.includes += ['posix', 'solaris']

    if bld.env['IS_WINDOWS']:
        obj.includes += ['windows']

# FIXME: Is SERVER_SIDE needed?
def build_jackd(bld):
    jackd = bld(
        features = ['cxx', 'cxxprogram'],
        defines = ['HAVE_CONFIG_H','SERVER_SIDE'],
        includes = ['.', 'common', 'common/jack'],
        target = 'jackd',
        source = ['common/Jackdmp.cpp'],
        use = ['serverlib', 'SYSTEMD']
    )

    if bld.env['BUILD_JACKDBUS']:
        jackd.source += ['dbus/audio_reserve.c', 'dbus/reserve.c']
        jackd.use += ['DBUS-1']

    if bld.env['IS_LINUX']:
        jackd.use += ['DL', 'M', 'PTHREAD', 'RT', 'STDC++']

    if bld.env['IS_MACOSX']:
        jackd.use += ['DL', 'PTHREAD']
        jackd.framework = ['CoreFoundation']

    if bld.env['IS_SUN']:
        jackd.use += ['DL', 'PTHREAD']

    obj_add_includes(bld, jackd)

    return jackd

# FIXME: Is SERVER_SIDE needed?
def create_driver_obj(bld, **kw):
    if 'use' in kw:
        kw['use'] += ['serverlib']
    else:
        kw['use'] = ['serverlib']

    driver = bld(
        features = ['c', 'cxx', 'cshlib', 'cxxshlib'],
        defines = ['HAVE_CONFIG_H', 'SERVER_SIDE'],
        includes = ['.', 'common', 'common/jack'],
        install_path = '${ADDON_DIR}/',
        **kw)

    if bld.env['IS_WINDOWS']:
        driver.env['cxxshlib_PATTERN'] = 'jack_%s.dll'
    else:
        driver.env['cxxshlib_PATTERN'] = 'jack_%s.so'

    obj_add_includes(bld, driver)

    return driver

def build_drivers(bld):
    # Non-hardware driver sources. Lexically sorted.
    dummy_src = [
        'common/JackDummyDriver.cpp'
    ]

    loopback_src = [
        'common/JackLoopbackDriver.cpp'
    ]

    net_src = [
        'common/JackNetDriver.cpp'
    ]

    netone_src = [
        'common/JackNetOneDriver.cpp',
        'common/netjack.c',
        'common/netjack_packet.c'
    ]

    proxy_src = [
        'common/JackProxyDriver.cpp'
    ]

    # Hardware driver sources. Lexically sorted.
    alsa_src = [
        'common/memops.c',
        'linux/alsa/JackAlsaDriver.cpp',
        'linux/alsa/alsa_rawmidi.c',
        'linux/alsa/alsa_seqmidi.c',
        'linux/alsa/alsa_midi_jackmp.cpp',
        'linux/alsa/generic_hw.c',
        'linux/alsa/hdsp.c',
        'linux/alsa/alsa_driver.c',
        'linux/alsa/hammerfall.c',
        'linux/alsa/ice1712.c'
    ]

    alsarawmidi_src = [
        'linux/alsarawmidi/JackALSARawMidiDriver.cpp',
        'linux/alsarawmidi/JackALSARawMidiInputPort.cpp',
        'linux/alsarawmidi/JackALSARawMidiOutputPort.cpp',
        'linux/alsarawmidi/JackALSARawMidiPort.cpp',
        'linux/alsarawmidi/JackALSARawMidiReceiveQueue.cpp',
        'linux/alsarawmidi/JackALSARawMidiSendQueue.cpp',
        'linux/alsarawmidi/JackALSARawMidiUtil.cpp'
    ]

    boomer_src = [
        'common/memops.c',
        'solaris/oss/JackBoomerDriver.cpp'
    ]

    coreaudio_src = [
        'macosx/coreaudio/JackCoreAudioDriver.mm',
        'common/JackAC3Encoder.cpp'
    ]

    coremidi_src = [
        'macosx/coremidi/JackCoreMidiInputPort.mm',
        'macosx/coremidi/JackCoreMidiOutputPort.mm',
        'macosx/coremidi/JackCoreMidiPhysicalInputPort.mm',
        'macosx/coremidi/JackCoreMidiPhysicalOutputPort.mm',
        'macosx/coremidi/JackCoreMidiVirtualInputPort.mm',
        'macosx/coremidi/JackCoreMidiVirtualOutputPort.mm',
        'macosx/coremidi/JackCoreMidiPort.mm',
        'macosx/coremidi/JackCoreMidiUtil.mm',
        'macosx/coremidi/JackCoreMidiDriver.mm'
    ]

    ffado_src = [
        'linux/firewire/JackFFADODriver.cpp',
        'linux/firewire/JackFFADOMidiInputPort.cpp',
        'linux/firewire/JackFFADOMidiOutputPort.cpp',
        'linux/firewire/JackFFADOMidiReceiveQueue.cpp',
        'linux/firewire/JackFFADOMidiSendQueue.cpp'
    ]

    iio_driver_src = [
        'linux/iio/JackIIODriver.cpp'
    ]

    oss_src = [
        'common/memops.c',
        'solaris/oss/JackOSSDriver.cpp'
    ]

    portaudio_src = [
        'windows/portaudio/JackPortAudioDevices.cpp',
        'windows/portaudio/JackPortAudioDriver.cpp',
    ]

    winmme_src = [
        'windows/winmme/JackWinMMEDriver.cpp',
        'windows/winmme/JackWinMMEInputPort.cpp',
        'windows/winmme/JackWinMMEOutputPort.cpp',
        'windows/winmme/JackWinMMEPort.cpp',
    ]

    # Create non-hardware driver objects. Lexically sorted.
    create_driver_obj(
        bld,
        target = 'dummy',
        source = dummy_src)

    create_driver_obj(
        bld,
        target = 'loopback',
        source = loopback_src)

    create_driver_obj(
        bld,
        target = 'net',
        source = net_src)

    create_driver_obj(
        bld,
        target = 'netone',
        source = netone_src,
        use = ['SAMPLERATE', 'CELT'])

    create_driver_obj(
        bld,
        target = 'proxy',
        source = proxy_src)

    # Create hardware driver objects. Lexically sorted after the conditional,
    # e.g. BUILD_DRIVER_ALSA.
    if bld.env['BUILD_DRIVER_ALSA']:
        create_driver_obj(
            bld,
            target = 'alsa',
            source = alsa_src,
            use = ['ALSA'])
        create_driver_obj(
            bld,
            target = 'alsarawmidi',
            source = alsarawmidi_src,
            use = ['ALSA'])

    if bld.env['BUILD_DRIVER_FFADO']:
        create_driver_obj(
            bld,
            target = 'firewire',
            source = ffado_src,
            use = ['LIBFFADO'])

    if bld.env['BUILD_DRIVER_IIO']:
        create_driver_obj(
            bld,
            target = 'iio',
            source = iio_src,
            use = ['GTKIOSTREAM', 'EIGEN3'])

    if bld.env['BUILD_DRIVER_PORTAUDIO']:
        create_driver_obj(
            bld,
            target = 'portaudio',
            source = portaudio_src,
            use = ['PORTAUDIO'])

    if bld.env['BUILD_DRIVER_WINMME']:
        create_driver_obj(
            bld,
            target = 'winmme',
            source = winmme_src,
            use = ['WINMME'])

    if bld.env['IS_MACOSX']:
        create_driver_obj(
            bld,
            target = 'coreaudio',
            source = coreaudio_src,
            use = ['AFTEN'],
            framework = ['AudioUnit', 'CoreAudio', 'CoreServices'])

        create_driver_obj(
            bld,
            target = 'coremidi',
            source = coremidi_src,
            use = ['serverlib'], # FIXME: Is this needed?
            framework = ['AudioUnit', 'CoreMIDI', 'CoreServices', 'Foundation'])

    if bld.env['IS_SUN']:
        create_driver_obj(
            bld,
            target = 'boomer',
            source = boomer_src)
        create_driver_obj(
            bld,
            target = 'oss',
            source = oss_src)

def build(bld):
    if not bld.variant and bld.env['BUILD_WITH_32_64']:
        Options.commands.append(bld.cmd + '_' + lib32)

    # process subfolders from here
    bld.recurse('common')

    if bld.variant:
        # only the wscript in common/ knows how to handle variants
        return

    bld.recurse('compat')

    if not os.access('svnversion.h', os.R_OK):
        def post_run(self):
            sg = Utils.h_file(self.outputs[0].abspath(self.env))
            #print sg.encode('hex')
            Build.bld.node_sigs[self.env.variant()][self.outputs[0].id] = sg

        script = bld.path.find_resource('svnversion_regenerate.sh')
        script = script.abspath()

        bld(
                rule = '%s ${TGT}' % script,
                name = 'svnversion',
                runnable_status = Task.RUN_ME,
                before = 'c cxx',
                color = 'BLUE',
                post_run = post_run,
                source = ['svnversion_regenerate.sh'],
                target = [bld.path.find_or_declare('svnversion.h')]
        )

    if bld.env['BUILD_JACKD']:
        build_jackd(bld)

    build_drivers(bld)

    bld.recurse('example-clients')
    bld.recurse('tools')

    if bld.env['IS_LINUX']:
        bld.recurse('man')
        bld.recurse('systemd')
    if not bld.env['IS_WINDOWS']:
        bld.recurse('tests')
    if bld.env['BUILD_JACKDBUS']:
        bld.recurse('dbus')

    if bld.env['BUILD_DOXYGEN_DOCS']:
        html_build_dir = bld.path.find_or_declare('html').abspath()

        bld(
            features = 'subst',
            source = 'doxyfile.in',
            target = 'doxyfile',
            HTML_BUILD_DIR = html_build_dir,
            SRCDIR = bld.srcnode.abspath(),
            VERSION = VERSION
        )

        # There are two reasons for logging to doxygen.log and using it as
        # target in the build rule (rather than html_build_dir):
        # (1) reduce the noise when running the build
        # (2) waf has a regular file to check for a timestamp. If the directory
        #     is used instead waf will rebuild the doxygen target (even upon
        #     install).
        def doxygen(task):
            doxyfile = task.inputs[0].abspath()
            logfile = task.outputs[0].abspath()
            cmd = '%s %s &> %s' % (task.env['DOXYGEN'][0], doxyfile, logfile)
            return task.exec_command(cmd)

        bld(
            rule = doxygen,
            source = 'doxyfile',
            target = 'doxygen.log'
        )

        # Determine where to install HTML documentation. Since share_dir is the
        # highest directory the uninstall routine should remove, there is no
        # better candidate for share_dir, but the requested HTML directory if
        # --htmldir is given.
        if bld.env['HTMLDIR']:
            html_install_dir = bld.options.destdir + bld.env['HTMLDIR']
            share_dir = html_install_dir
        else:
            share_dir = bld.options.destdir + bld.env['PREFIX'] + '/share/jack-audio-connection-kit'
            html_install_dir = share_dir + '/reference/html/'

        if bld.cmd == 'install':
            if os.path.isdir(html_install_dir):
                Logs.pprint('CYAN', 'Removing old doxygen documentation installation...')
                shutil.rmtree(html_install_dir)
                Logs.pprint('CYAN', 'Removing old doxygen documentation installation done.')
            Logs.pprint('CYAN', 'Installing doxygen documentation...')
            shutil.copytree(html_build_dir, html_install_dir)
            Logs.pprint('CYAN', 'Installing doxygen documentation done.')
        elif bld.cmd =='uninstall':
            Logs.pprint('CYAN', 'Uninstalling doxygen documentation...')
            if os.path.isdir(share_dir):
                shutil.rmtree(share_dir)
            Logs.pprint('CYAN', 'Uninstalling doxygen documentation done.')
        elif bld.cmd =='clean':
            if os.access(html_build_dir, os.R_OK):
                Logs.pprint('CYAN', 'Removing doxygen generated documentation...')
                shutil.rmtree(html_build_dir)
                Logs.pprint('CYAN', 'Removing doxygen generated documentation done.')

def dist(ctx):
    # This code blindly assumes it is working in the toplevel source directory.
    if not os.path.exists('svnversion.h'):
        os.system('./svnversion_regenerate.sh svnversion.h')

from waflib import TaskGen
@TaskGen.extension('.mm')
def mm_hook(self, node):
    """Alias .mm files to be compiled the same as .cpp files, gcc will do the right thing."""
    return self.create_compiled_task('cxx', node)
