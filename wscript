#! /usr/bin/env python
# encoding: utf-8
from __future__ import print_function

import os
import subprocess
import shutil
import re
import sys

from waflib import Logs, Options, Task, Utils
from waflib.Build import BuildContext, CleanContext, InstallContext, UninstallContext

VERSION='1.9.12'
APPNAME='jack'
JACK_API_VERSION = '0.1.0'

# these variables are mandatory ('/' are converted automatically)
top = '.'
out = 'build'

# lib32 variant name used when building in mixed mode
lib32 = 'lib32'

auto_options = []

def display_feature(conf, msg, build):
    if build:
        conf.msg(msg, 'yes', color='GREEN')
    else:
        conf.msg(msg, 'no', color='YELLOW')

# This function prints an error without stopping waf. The reason waf should not
# be stopped is to be able to list all missing dependencies in one chunk.
def print_error(msg):
    print(Logs.colors.RED + msg + Logs.colors.NORMAL)

class AutoOption:
    """
    This class is the foundation for the auto options. It adds an option
    --foo=no|yes to the list of options and deals with all logic and checks for
    these options.

    Each option can have different dependencies that will be checked. If all
    dependencies are available and the user has not done any request the option
    will be enabled. If the user has requested to enable the option the class
    ensures that all dependencies are available and prints an error message
    otherwise. If the user disables the option, i.e. --foo=no, no checks are
    made.

    For each option it is possible to add packages that are required for the
    option using the add_package function. For dependency programs add_program
    should be used. For libraries (without pkg-config support) the add_library
    function should be used. For headers the add_header function exists. If
    there is another type of requirement or dependency the check hook (an
    external function called when configuring) can be used.

    When all checks have been made and the class has made a decision the result
    is saved in conf.env['NAME'] where 'NAME' by default is the uppercase of the
    name argument to __init__, but it can be changed with the conf_dest argument
    to __init__.

    The class will define a preprocessor symbol with the result. The default
    name is HAVE_NAME, but it can be changed using the define argument to
    __init__.
    """

    def __init__(self, opt, name, help, conf_dest=None, define=None):
        # check hook to call upon configuration
        self.check_hook = None
        self.check_hook_error = None
        self.check_hook_found = True

        # required libraries
        self.libs = [] # elements on the form [lib,uselib_store]
        self.libs_not_found = [] # elements on the form lib

        # required headers
        self.headers = []
        self.headers_not_found = []

        # required packages (checked with pkg-config)
        self.packages = [] # elements on the form [package,uselib_store,atleast_version]
        self.packages_not_found = [] # elements on the form [package,atleast_version]

        # required programs
        self.programs = [] # elements on the form [program,var]
        self.programs_not_found = [] # elements on the form program

        # the result of the configuration (should the option be enabled or not?)
        self.result = False

        self.help = help
        self.option = '--' + name
        self.dest = 'auto_option_' + name
        if conf_dest:
            self.conf_dest = conf_dest
        else:
            self.conf_dest = name.upper()
        if not define:
            self.define = 'HAVE_' + name.upper()
        else:
            self.define = define
        opt.add_option(self.option, type='string', default='auto', dest=self.dest, help=self.help+' (enabled by default if possible)', metavar='no|yes')

    def add_library(self, library, uselib_store=None):
        """
        Add a required library that should be checked during configuration. The
        library will be checked using the conf.check function. If the
        uselib_store arugment is not given it defaults to LIBRARY (the uppercase
        of the library argument). The uselib_store argument will be passed to
        check which means LIB_LIBRARY, CFLAGS_LIBRARY and DEFINES_LIBRARY, etc.
        will be defined if the option is enabled.
        """
        if not uselib_store:
            uselib_store = library.upper().replace('-', '_')
        self.libs.append([library, uselib_store])

    def add_header(self, header):
        """
        Add a required header that should be checked during configuration. The
        header will be checked using the conf.check function which means
        HAVE_HEADER_H will be defined if found.
        """
        self.headers.append(header)

    def add_package(self, package, uselib_store=None, atleast_version=None):
        """
        Add a required package that should be checked using pkg-config during
        configuration. The package will be checked using the conf.check_cfg
        function and the uselib_store and atleast_version will be passed to
        check_cfg. If uselib_store is None it defaults to PACKAGE (uppercase of
        the package argument) with hyphens and dots replaced with underscores.
        If atleast_version is None it defaults to '0'.
        """
        if not uselib_store:
            uselib_store = package.upper().replace('-', '_').replace('.', '_')
        if not atleast_version:
            atleast_version = '0'
        self.packages.append([package, uselib_store, atleast_version])

    def add_program(self, program, var=None):
        """
        Add a required program that should be checked during configuration. If
        var is not given it defaults to PROGRAM (the uppercase of the program
        argument). If the option is enabled the program is saved as a list (?!)
        in conf.env['PROGRAM'].
        """
        if not var:
            var = program.upper().replace('-', '_')
        self.programs.append([program, var])

    def set_check_hook(self, check_hook, check_hook_error):
        """
        Set the check hook and the corresponding error printing function to the
        configure step. The check_hook argument is a function that should return
        True if the extra prerequisites were found and False if not. The
        check_hook_error argument is an error printing function that should
        print an error message telling the user that --foo was explicitly
        requested but cannot be built since the extra prerequisites were not
        found. Both function should take a single argument that is the waf
        configuration context.
        """
        self.check_hook = check_hook
        self.check_hook_error = check_hook_error

    def _check(self, conf):
        """
        This is an internal function that runs all necessary configure checks.
        It checks all dependencies (even if some dependency was not found) so
        that the user can install all missing dependencies in one go, instead
        of playing the infamous hit-configure-hit-configure game.

        This function returns True if all dependencies were found and False if
        not.
        """
        all_found = True

        # Use-variables that should be used when checking libraries, headers and
        # programs. The list will be populated when looking for packages.
        use = []

        # check for packages
        for package,uselib_store,atleast_version in self.packages:
            try:
                conf.check_cfg(package=package, uselib_store=uselib_store, atleast_version=atleast_version, args='--cflags --libs')
                use.append(uselib_store)
            except conf.errors.ConfigurationError:
                all_found = False
                self.packages_not_found.append([package,atleast_version])

        # check for libraries
        for lib,uselib_store in self.libs:
            try:
                conf.check(lib=lib, uselib_store=uselib_store, use=use)
            except conf.errors.ConfigurationError:
                all_found = False
                self.libs_not_found.append(lib)

        # check for headers
        for header in self.headers:
            try:
                conf.check(header_name=header, use=use)
            except conf.errors.ConfigurationError:
                all_found = False
                self.headers_not_found.append(header)

        # check for programs
        for program,var in self.programs:
            try:
                conf.find_program(program, var=var, use=use)
            except conf.errors.ConfigurationError:
                all_found = False
                self.programs_not_found.append(program)

        # call hook (if specified)
        if self.check_hook:
            self.check_hook_found = self.check_hook(conf)
            if not self.check_hook_found:
                all_found = False

        return all_found

    def _configure_error(self, conf):
        """
        This is an internal function that prints errors for each missing
        dependency. The error messages tell the user that this option required
        some dependency, but it cannot be found.
        """

        for lib in self.libs_not_found:
            print_error('%s requires the %s library, but it cannot be found.' % (self.option, lib))

        for header in self.headers_not_found:
            print_error('%s requires the %s header, but it cannot be found.' % (self.option, header))

        for package,atleast_version in self.packages_not_found:
            string = package
            if atleast_version:
                string += ' >= ' + atleast_version
            print_error('%s requires the package %s, but it cannot be found.' % (self.option, string))

        for program in self.programs_not_found:
            print_error('%s requires the %s program, but it cannot be found.' % (self.option, program))

        if not self.check_hook_found:
            self.check_hook_error(conf)

    def configure(self, conf):
        """
        This function configures the option examining the argument given too
        --foo (where foo is this option). This function sets self.result to the
        result of the configuration; True if the option should be enabled or
        False if not. If not all dependencies were found self.result will shall
        be False. conf.env['NAME'] will be set to the same value aswell as a
        preprocessor symbol will be defined according to the result.

        If --foo[=yes] was given, but some dependency was not found an error
        message is printed (foreach missing dependency).

        This function returns True on success and False on error.
        """
        argument = getattr(Options.options, self.dest)
        if argument == 'no':
            self.result = False
            retvalue = True
        elif argument == 'yes':
            if self._check(conf):
                self.result = True
                retvalue = True
            else:
                self.result = False
                retvalue = False
                self._configure_error(conf)
        elif argument == 'auto':
            self.result = self._check(conf)
            retvalue = True
        else:
            print_error('Invalid argument "' + argument + '" to ' + self.option)
            self.result = False
            retvalue = False

        conf.env[self.conf_dest] = self.result
        if self.result:
            conf.define(self.define, 1)
        else:
            conf.define(self.define, 0)
        return retvalue

    def display_message(self, conf):
        """
        This function displays a result message with the help text and the
        result of the configuration.
        """
        display_feature(conf, self.help, self.result)

# This function adds an option to the list of auto options and returns the newly
# created option.
def add_auto_option(opt, name, help, conf_dest=None, define=None):
    option = AutoOption(opt, name, help, conf_dest=conf_dest, define=define)
    auto_options.append(option)
    return option

# This function applies a hack that for each auto option --foo=no|yes replaces
# any occurence --foo in argv with --foo=yes, in effect interpreting --foo as
# --foo=yes. The function has to be called before waf issues the option parser,
# i.e. before the configure phase.
def auto_options_argv_hack():
    for option in auto_options:
        for x in range(1, len(sys.argv)):
            if sys.argv[x] == option.option:
                sys.argv[x] += '=yes'

# This function configures all auto options. It stops waf and prints an error
# message if there were unsatisfied requirements.
def configure_auto_options(conf):
    ok = True
    for option in auto_options:
        if not option.configure(conf):
            ok = False
    if not ok:
        conf.fatal('There were unsatisfied requirements.')

# This function displays all options and the configuration results.
def display_auto_options_messages(conf):
    for option in auto_options:
        option.display_message(conf)

def check_for_celt(conf):
    found = False
    for version in ['11', '8', '7', '5']:
        define = 'HAVE_CELT_API_0_' + version
        if not found:
            try:
                conf.check_cfg(package='celt', atleast_version='0.' + version + '.0', args='--cflags --libs')
                found = True
                conf.define(define, 1)
                continue
            except conf.errors.ConfigurationError:
                pass
        conf.define(define, 0)
    return found

def check_for_celt_error(conf):
    print_error('--celt requires the package celt, but it could not be found.')

# The readline/readline.h header does not work if stdio.h is not included
# before. Thus a fragment with both stdio.h and readline/readline.h need to be
# test-compiled to find out whether readline is available.
def check_for_readline(conf):
    # FIXME: This check can be incorporated into the AutoOptions class by
    #        passing header_name=['stdio.h', 'readline/readline.h'] to check.
    try:
        conf.check(fragment='''
                   #include <stdio.h>
                   #include <readline/readline.h>
                   int main(void) { return 0; }''',
                   execute=False,
                   msg='Checking for header readline/readline.h',
                   errmsg='not found')
        return True
    except conf.errors.ConfigurationError:
        return False

def check_for_readline_error(conf):
    print_error('--readline requires the readline/readline.h header, but it cannot be found.')

def check_for_mmsystem(conf):
    # FIXME: See comment in check_for_readline.
    try:
        conf.check(fragment='''
                   #include <windows.h>
                   #include <mmsystem.h>
                   int main(void) { return 0; }''',
                   execute=False,
                   msg='Checking for header mmsystem.h',
                   errmsg='not found')
        return True
    except conf.errors.ConfigurationError:
        return False

def check_for_mmsystem_error(conf):
    print_error('--winmme requires the mmsystem.h header, but it cannot be found.')

def options(opt):
    # options provided by the modules
    opt.load('compiler_cxx')
    opt.load('compiler_c')

    opt.load('xcode')
    opt.load('xcode6')

    # install directories
    opt.add_option('--htmldir', type='string', default=None, help='HTML documentation directory [Default: <prefix>/share/jack-audio-connection-kit/reference/html/')
    opt.add_option('--libdir', type='string', help='Library directory [Default: <prefix>/lib]')
    opt.add_option('--libdir32', type='string', help='32bit Library directory [Default: <prefix>/lib32]')
    opt.add_option('--mandir', type='string', help='Manpage directory [Default: <prefix>/share/man/man1]')

    # options affecting binaries
    opt.add_option('--platform', type='string', default=sys.platform, help='Target platform for cross-compiling, e.g. cygwin or win32')
    opt.add_option('--mixed', action='store_true', default=False, help='Build with 32/64 bits mixed mode')
    opt.add_option('--debug', action='store_true', default=False, dest='debug', help='Build debuggable binaries')

    # options affecting general jack functionality
    opt.add_option('--classic', action='store_true', default=False, help='Force enable standard JACK (jackd) even if D-Bus JACK (jackdbus) is enabled too')
    opt.add_option('--dbus', action='store_true', default=False, help='Enable D-Bus JACK (jackdbus)')
    opt.add_option('--autostart', type='string', default='default', help='Autostart method. Possible values: "default", "classic", "dbus", "none"')
    opt.add_option('--profile', action='store_true', default=False, help='Build with engine profiling')
    opt.add_option('--clients', default=64, type='int', dest='clients', help='Maximum number of JACK clients')
    opt.add_option('--ports-per-application', default=768, type='int', dest='application_ports', help='Maximum number of ports per application')

    # options with third party dependencies
    doxygen = add_auto_option(opt, 'doxygen', help='Build doxygen documentation', conf_dest='BUILD_DOXYGEN_DOCS')
    doxygen.add_program('doxygen')
    alsa = add_auto_option(opt, 'alsa', help='Enable ALSA driver', conf_dest='BUILD_DRIVER_ALSA')
    alsa.add_package('alsa', atleast_version='1.0.18')
    firewire = add_auto_option(opt, 'firewire', help='Enable FireWire driver (FFADO)', conf_dest='BUILD_DRIVER_FFADO')
    firewire.add_package('libffado', atleast_version='1.999.17')
    freebob = add_auto_option(opt, 'freebob', help='Enable FreeBob driver')
    freebob.add_package('libfreebob', atleast_version='1.0.0')
    iio = add_auto_option(opt, 'iio', help='Enable IIO driver', conf_dest='BUILD_DRIVER_IIO')
    iio.add_package('gtkIOStream', atleast_version='1.4.0')
    iio.add_package('eigen3', atleast_version='3.1.2')
    portaudio = add_auto_option(opt, 'portaudio', help='Enable Portaudio driver', conf_dest='BUILD_DRIVER_PORTAUDIO')
    portaudio.add_header('windows.h') # only build portaudio on windows
    portaudio.add_package('portaudio-2.0', uselib_store='PORTAUDIO', atleast_version='19')
    winmme = add_auto_option(opt, 'winmme', help='Enable WinMME driver', conf_dest='BUILD_DRIVER_WINMME')
    winmme.set_check_hook(check_for_mmsystem, check_for_mmsystem_error)

    celt = add_auto_option(opt, 'celt', help='Build with CELT')
    celt.set_check_hook(check_for_celt, check_for_celt_error)
    opus = add_auto_option(opt, 'opus', help='Build Opus netjack2')
    opus.add_header('opus/opus_custom.h')
    opus.add_package('opus', atleast_version='0.9.0')
    samplerate = add_auto_option(opt, 'samplerate', help='Build with libsamplerate')
    samplerate.add_package('samplerate')
    sndfile = add_auto_option(opt, 'sndfile', help='Build with libsndfile')
    sndfile.add_package('sndfile')
    readline = add_auto_option(opt, 'readline', help='Build with readline')
    readline.add_library('readline')
    readline.set_check_hook(check_for_readline, check_for_readline_error)

    # dbus options
    opt.recurse('dbus')

    # this must be called before the configure phase
    auto_options_argv_hack()

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

    conf.env.append_unique('CXXFLAGS', '-Wall')
    conf.env.append_unique('CFLAGS', '-Wall')

    if conf.env['IS_MACOSX']:
        conf.check(lib='aften', uselib='AFTEN', define_name='AFTEN')

    # configure all auto options
    configure_auto_options(conf)

    # Check for functions.
    conf.check(
        function_name='ppoll',
        header_name=['poll.h', 'signal.h'],
        defines=['_GNU_SOURCE'],
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

    conf.recurse('example-clients')

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
        # we define this in the environment to maintain compatability with
        # existing install paths that use ADDON_DIR rather than have to
        # have special cases for windows each time.
        conf.env['ADDON_DIR'] = conf.env['BINDIR'] + '/jack'
        # don't define ADDON_DIR in config.h, use the default 'jack' defined in
        # windows/JackPlatformPlug_os.h
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
        conf.env.append_unique('CXXFLAGS', '-m32')
        conf.env.append_unique('CFLAGS', '-m32')
        conf.env.append_unique('LINKFLAGS', '-m32')
        if Options.options.libdir32:
            conf.env['LIBDIR'] = Options.options.libdir32
        else:
            conf.env['LIBDIR'] = conf.env['PREFIX'] + '/lib32'
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

    # display configuration result messages for auto options
    display_auto_options_messages(conf)

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
        use = ['serverlib']
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
    if bld.env['IS_MACOSX'] or bld.env['IS_WINDOWS']:
        # On MacOSX this is necessary.
        # I do not know if this is necessary on Windows.
        # Note added on 2015-12-13 by karllinden.
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

    freebob_src = [
        'linux/freebob/JackFreebobDriver.cpp'
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

    if bld.env['BUILD_DRIVER_FREEBOB']:
        create_driver_obj(
            bld,
            target = 'freebob',
            source = freebob_src,
            use = ['LIBFREEBOB'])

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
    if bld.env['IS_LINUX']:
        bld.recurse('man')
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
