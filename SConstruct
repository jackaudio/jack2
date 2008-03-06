#
# Copyright (C) 2007 Arnold Krille
# Copyright (C) 2007 Pieter Palmers
# Copyright (C) 2008 Marc-Olivier Barre
#
# This file originates from FFADO (www.ffado.org)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import os
from string import Template

platform = ARGUMENTS.get('OS', str(Platform()))

build_dir = ARGUMENTS.get('BUILDDIR', '')
if build_dir:
    build_base = build_dir + '/'
    if not os.path.isdir(build_base):
        os.makedirs(build_base)
    print 'Building into: ' + build_base
else:
    build_base=''

if not os.path.isdir('cache'):
    os.makedirs('cache')

opts = Options('cache/'+build_base+'options.cache')

#
# Command-line options section
#

# If this is just to display a help-text for the variable used via ARGUMENTS, then its wrong...
opts.Add( 'BUILDDIR', 'Path to place the built files in', '')
opts.AddOptions(
    PathOption('PREFIX', 'The prefix where jackdmp will be installed to', '/usr/local', PathOption.PathAccept),
    PathOption('BINDIR', 'Overwrite the directory where apps are installed to', '$PREFIX/bin', PathOption.PathAccept),
    PathOption('LIBDIR', 'Overwrite the directory where libs are installed to', '$PREFIX/lib', PathOption.PathAccept),
    PathOption('INCLUDEDIR', 'Overwrite the directory where headers are installed to', '$PREFIX/include', PathOption.PathAccept),
    # TODO: The next one is stupid, should be autodetected
    BoolOption('BUILD_FOR_LINUX', 'Enable/Disable depending on your system', True),
    BoolOption('ENABLE_ALSA', 'Enable/Disable the ALSA backend', True),
    BoolOption('ENABLE_FREEBOB', 'Enable/Disable the FreeBoB backend', True),
    BoolOption('ENABLE_FIREWIRE', 'Enable/Disable the FireWire backend', True),
    BoolOption('DEBUG', 'Do a debug build', False),
    BoolOption('BUILD_TESTS', 'Build tests where applicable', True),
    BoolOption('BUILD_EXAMPLES', 'Build the example clients in their directory', True),
    BoolOption('INSTALL_EXAMPLES', 'Install the example clients in the BINDIR directory', True),
    BoolOption('BUILD_DOXYGEN_DOCS', 'Build doxygen documentation', False),
    )

#
# Configuration section
#

# Load the builders in config
buildenv = os.environ
if os.environ.has_key('PATH'):
    buildenv['PATH'] = os.environ['PATH']
else:
    buildenv['PATH'] = ''

if os.environ.has_key('PKG_CONFIG_PATH'):
    buildenv['PKG_CONFIG_PATH'] = os.environ['PKG_CONFIG_PATH']
else:
    buildenv['PKG_CONFIG_PATH'] = ''

if os.environ.has_key('LD_LIBRARY_PATH'):
    buildenv['LD_LIBRARY_PATH'] = os.environ['LD_LIBRARY_PATH']
else:
    buildenv['LD_LIBRARY_PATH'] = ''

env = Environment(tools=['default', 'scanreplace', 'pkgconfig', 'doxygen'], toolpath = ['admin'], ENV=buildenv, PLATFORM = platform, options = opts)

Help('To build jackdmp you can set different options as listed below. You have to specify them only once, scons will save the latest values you set and re-use then. To really undo your settings and return to the factory defaults, remove the .sconsign.dblite and options.cache files from your BUILDDIR directory.')
Help(opts.GenerateHelpText(env))

# make sure the necessary dirs exist
if not os.path.isdir('cache/' + build_base):
    os.makedirs('cache/' + build_base)

if build_base:
    env['build_base']='#/'+build_base
else:
    env['build_base']='#/'

opts.Save('cache/' + build_base + 'options.cache', env)

tests = {}
tests.update(env['PKGCONFIG_TESTS'])

if not env.GetOption('clean'):
    conf = Configure(env,
        custom_tests = tests,
        conf_dir = 'cache/' + build_base,
        log_file = 'cache/' + build_base + 'config.log')

    # Check if the environment can actually compile c-files by checking for a
    # header shipped with gcc
    if not conf.CheckHeader( 'stdio.h' ):
        print 'Error: It seems as if stdio.h is missing. This probably means that your build environment is broken, please make sure you have a working c-compiler and libstdc installed and usable.'
        Exit(1)

    # The following checks are for headers, libs and packages we depend on
    allpresent = 1;

    if env['BUILD_FOR_LINUX']:
        allpresent &= conf.CheckForPKGConfig('0.20');

    if not allpresent:
        print "--> At least one of the dependencies is missing. I can't go on without it, please install the needed packages (remember to also install the *-devel packages)"
        Exit(1)

    # If jack has the same PREFIX as the one we plan to use, exit with an error message
    env['JACK_FLAGS'] = conf.GetPKGFlags('jack', '0.90')
    if env['JACK_FLAGS']:
        print "--> Found an existing JACK installation, let's be careful not to erase it"
        if conf.GetPKGPrefix( 'jack' ) == env['PREFIX']:
            print '--> JACK is installed in the same directory as our current PREFIX. Either remove JACK or change your installation PREFIX.'
            Exit(1)

    # Optional checks follow:
    if env['BUILD_FOR_LINUX'] and env['ENABLE_ALSA']:
        env['ALSA_FLAGS'] = conf.GetPKGFlags('alsa', '1.0.0')
        if env['ALSA_FLAGS'] == 0:
            print "--> Disabling 'alsa' backend since no useful ALSA installation found"
            env['ENABLE_ALSA'] = False

    if env['BUILD_FOR_LINUX'] and env['ENABLE_FREEBOB']:
        env['FREEBOB_FLAGS'] = conf.GetPKGFlags('libfreebob', '1.0.0')
        if env['FREEBOB_FLAGS'] == 0:
            print "--> Disabling 'freebob' backend since no useful FreeBoB installation found"
            env['ENABLE_FREEBOB'] = False

    if env['BUILD_FOR_LINUX'] and env['ENABLE_FIREWIRE']:
        env['FFADO_FLAGS'] = conf.GetPKGFlags('libffado', '1.999.14')
        if env['FFADO_FLAGS'] == 0:
            print "--> Disabling 'firewire' backend since no useful FFADO installation found"
            env['ENABLE_FIREWIRE'] = False

    env = conf.Finish()

if env['DEBUG']:
    print '--> Doing a DEBUG build'
    # TODO: -Werror could be added to, which would force the devs to really remove all the warnings :-)
    env.AppendUnique(CCFLAGS = ['-DDEBUG', '-Wall', '-g'])
else:
    env.AppendUnique(CCFLAGS = ['-O3','-DNDEBUG'])

env.AppendUnique(CCFLAGS = ['-fPIC', '-DSOCKET_RPC_FIFO_SEMA', '-D__SMP__'])
env.AppendUnique(CFLAGS = ['-fPIC', '-DUSE_POSIX_SHM'])

env.Alias('install', env['LIBDIR'])
env.Alias('install', env['INCLUDEDIR'])
env.Alias('install', env['BINDIR'])

# for config.h.in
# TODO: Is that necessary ?
env['ADDON_DIR']='%s' % env['PREFIX']
env['LIB_DIR']='lib'
env['JACK_LOCATION']='%s' % env['BINDIR']

# To have the top_srcdir as the doxygen-script is used from auto*
# TODO: Understand the previous comment
env['top_srcdir'] = env.Dir('.').abspath

env.ScanReplace( 'config.h.in' )
# TODO: find out what's that about. Is it useful ?
AlwaysBuild('config.h')

# Ensure we have a path to where the libraries are
env.AppendUnique(LIBPATH=['#/common'])

#
# Build section
#

if env['BUILD_DOXYGEN_DOCS']:
    env.Doxygen('doxyfile')

subdirs=['common']

# TODO: Really handle each platform automatically
if env['PLATFORM'] == 'posix':
    subdirs.append('linux')

# TODO FOR SLETZ: test macosx/SConscript
if env['PLATFORM'] == 'macosx':
    subdirs.append('macosx')

# TODO FOR SLETZ & MARC: create/check windows/SConscript
#if env['PLATFORM'] == 'windows':
#   subdirs.append('windows')

if env['BUILD_EXAMPLES']:
    subdirs.append('example-clients')

if env['BUILD_TESTS']:
    subdirs.append('tests')

if build_base:
    env.SConscript(dirs=subdirs, exports='env', build_dir=build_base+subdir)
else:
    env.SConscript(dirs=subdirs, exports='env')
