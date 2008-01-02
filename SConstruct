#
# Copyright (C) 2007 Arnold Krille
# Copyright (C) 2007 Pieter Palmers
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

build_dir = ARGUMENTS.get('BUILDDIR', "")
if build_dir:
    build_base=build_dir+'/'
    if not os.path.isdir( build_base ):
        os.makedirs( build_base )
    print "Building into: " + build_base
else:
    build_base=''

if not os.path.isdir( "cache" ):
    os.makedirs( "cache" )

opts = Options( "cache/"+build_base+"options.cache" )

# make this into a command line option and/or a detected value
build_for_linux = True

#
# If this is just to display a help-text for the variable used via ARGUMENTS, then its wrong...
opts.Add( "BUILDDIR", "Path to place the built files in", "")

opts.AddOptions(
#    BoolOption( "DEBUG", """\
#Toggle debug-build. DEBUG means \"-g -Wall\" and more, otherwise we will use
#  \"-O2\" to optimise.""", True ),
    PathOption( "PREFIX", "The prefix where jackdmp will be installed to.", "/usr/local", PathOption.PathAccept ),
    PathOption( "BINDIR", "Overwrite the directory where apps are installed to.", "$PREFIX/bin", PathOption.PathAccept ),
    PathOption( "LIBDIR", "Overwrite the directory where libs are installed to.", "$PREFIX/lib", PathOption.PathAccept ),
    PathOption( "INCLUDEDIR", "Overwrite the directory where headers are installed to.", "$PREFIX/include", PathOption.PathAccept ),
    PathOption( "SHAREDIR", "Overwrite the directory where misc shared files are installed to.", "$PREFIX/share/libffado", PathOption.PathAccept ),
    BoolOption( "ENABLE_ALSA", "Enable/Disable the ALSA backend.", True ),
    BoolOption( "ENABLE_FREEBOB", "Enable/Disable the FreeBoB backend.", True ),
    BoolOption( "ENABLE_FIREWIRE", "Enable/Disable the FireWire backend.", True ),
    BoolOption( "DEBUG", """Do a debug build.""", True ),
    BoolOption( "BUILD_TESTS", """Build tests where applicable.""", True ),
    BoolOption( "BUILD_EXAMPLES", """Build the example clients in their directory.""", True ),
    BoolOption( "INSTALL_EXAMPLES", """Install the example clients in the BINDIR directory.""", True ),
    BoolOption( "BUILD_DOXYGEN_DOCS", """Build doxygen documentation.""", True ),
    )

## Load the builders in config
buildenv={}
if os.environ.has_key('PATH'):
    buildenv['PATH']=os.environ['PATH']
else:
    buildenv['PATH']=''

if os.environ.has_key('PKG_CONFIG_PATH'):
    buildenv['PKG_CONFIG_PATH']=os.environ['PKG_CONFIG_PATH']
else:
    buildenv['PKG_CONFIG_PATH']=''

if os.environ.has_key('LD_LIBRARY_PATH'):
    buildenv['LD_LIBRARY_PATH']=os.environ['LD_LIBRARY_PATH']
else:
    buildenv['LD_LIBRARY_PATH']=''


env = Environment( tools=['default','scanreplace','pkgconfig', 'doxygen'], toolpath=['admin'],
                   ENV=buildenv, PLATFORM = platform, options=opts )

Help( """
For building jackdmp you can set different options as listed below. You have to
specify them only once, scons will save the last value you used and re-use
that.
To really undo your settings and return to the factory defaults, remove the
"cache"-folder and the file ".sconsign.dblite" from this directory.
For example with: "rm -Rf .sconsign.dblite cache"

""" )
Help( opts.GenerateHelpText( env ) )

# make sure the necessary dirs exist
if not os.path.isdir( "cache/" + build_base ):
    os.makedirs( "cache/" + build_base )
if not os.path.isdir( 'cache/objects' ):
    os.makedirs( 'cache/objects' )

if build_base:
    env['build_base']="#/"+build_base
else:
    env['build_base']="#/"


CacheDir( 'cache/objects' )
opts.Save( 'cache/' + build_base + "options.cache", env )

tests = {}
tests.update( env['PKGCONFIG_TESTS'] )

if not env.GetOption('clean'):
    conf = Configure( env,
        custom_tests = tests,
        conf_dir = "cache/" + build_base,
        log_file = "cache/" + build_base + 'config.log' )

    #
    # Check if the environment can actually compile c-files by checking for a
    # header shipped with gcc.
    #
    if not conf.CheckHeader( "stdio.h" ):
        print "It seems as if stdio.h is missing. This probably means that your build environment is broken, please make sure you have a working c-compiler and libstdc installed and usable."
        Exit( 1 )

    #
    # The following checks are for headers and libs and packages we need.
    #
    allpresent = 1;

    if build_for_linux:
        allpresent &= conf.CheckForPKGConfig();

# example on how to check for additional libs
#    pkgs = {
#        'alsa' : '1.0.0',
#        }
#    for pkg in pkgs:
#        name2 = pkg.replace("+","").replace(".","").replace("-","").upper()
#        env['%s_FLAGS' % name2] = conf.GetPKGFlags( pkg, pkgs[pkg] )
#        if env['%s_FLAGS'%name2] == 0:
#            allpresent &= 0

    if not allpresent:
        print """
(At least) One of the dependencies is missing. I can't go on without it, please
install the needed packages (remember to also install the *-devel packages)
"""
        Exit( 1 )

    # jack doesn't have to be present, but it would be nice to know if it is
    env['JACK_FLAGS'] = conf.GetPKGFlags( 'jack', '0.100.0' )
    if env['JACK_FLAGS']:
        env['JACK_PREFIX'] = conf.GetPKGPrefix( 'jack' )
        env['JACK_EXEC_PREFIX'] = conf.GetPKGExecPrefix( 'jack' )
        env['JACK_LIBDIR'] = conf.GetPKGLibdir( 'jack' )
        env['JACK_INCLUDEDIR'] = conf.GetPKGIncludedir( 'jack' )

    #
    # Optional checks follow:
    #
    if build_for_linux and env['ENABLE_ALSA']:
        env['ALSA_FLAGS'] = conf.GetPKGFlags( 'alsa', '1.0.0' )
        if env['ALSA_FLAGS'] == 0:
            print " Disabling 'alsa' backend since no useful ALSA installation found."
            env['ENABLE_ALSA'] = False

    if build_for_linux and env['ENABLE_FREEBOB']:
        env['FREEBOB_FLAGS'] = conf.GetPKGFlags( 'libfreebob', '1.0.0' )
        if env['FREEBOB_FLAGS'] == 0:
            print " Disabling 'freebob' backend since no useful FreeBoB installation found."
            env['ENABLE_FREEBOB'] = False

    if build_for_linux and env['ENABLE_FIREWIRE']:
        env['FFADO_FLAGS'] = conf.GetPKGFlags( 'libffado', '1.999.7' )
        if env['FFADO_FLAGS'] == 0:
            print " Disabling 'firewire' backend since no useful FFADO installation found."
            env['ENABLE_FIREWIRE'] = False

    env = conf.Finish()

if env['DEBUG']:
    print "Doing a DEBUG build"
    # -Werror could be added to, which would force the devs to really remove all the warnings :-)
    env.AppendUnique( CCFLAGS=["-DDEBUG","-Wall","-g"] )
else:
    env.AppendUnique( CCFLAGS=["-O3","-DNDEBUG"] )

env.AppendUnique( CCFLAGS=["-fPIC", "-DSOCKET_RPC_FIFO_SEMA", "-D__SMP__"] )
env.AppendUnique( CFLAGS=["-fPIC", "-DUSE_POSIX_SHM"] )

#
# XXX: Maybe we can even drop these lower-case variables and only use the uppercase ones?
#
env['prefix'] = Template( os.path.join( env['PREFIX'] ) ).safe_substitute( env )
env['bindir'] = Template( os.path.join( env['BINDIR'] ) ).safe_substitute( env )
env['libdir'] = Template( os.path.join( env['LIBDIR'] ) ).safe_substitute( env )
env['includedir'] = Template( os.path.join( env['INCLUDEDIR'] ) ).safe_substitute( env )
env['sharedir'] = Template( os.path.join( env['SHAREDIR'] ) ).safe_substitute( env )

env.Alias( "install", env['libdir'] )
env.Alias( "install", env['includedir'] )
env.Alias( "install", env['sharedir'] )
env.Alias( "install", env['bindir'] )

# for config.h.in
env['ADDON_DIR']='%s' % env['prefix']
env['LIB_DIR']='lib'
env['JACK_LOCATION']='%s' % env['bindir']

#
# To have the top_srcdir as the doxygen-script is used from auto*
#
env['top_srcdir'] = env.Dir( "." ).abspath

#subprojects = env.Split('common common/jack tests example-clients linux/alsa linux/freebob linux/firewire')

#for subproject in subprojects:
    #env.AppendUnique( CCFLAGS=["-I%s" % subproject] )
    #env.AppendUnique( CFLAGS=["-I%s" % subproject] )

env.ScanReplace( "config.h.in" )
# ensure that the config.h is always updated, since it
# sometimes fails to pick up the changes
# note: this still doesn't seem to cause dependent files to be rebuilt.
NoCache("config.h")
AlwaysBuild("config.h")

#
# Start building
#
if env['BUILD_DOXYGEN_DOCS']:
    env.Doxygen("doxyfile")

subdirs=['common']
if env['PLATFORM'] == 'posix':
    subdirs.append('linux')

if env['PLATFORM'] == 'macosx': # FIXME FOR SLETZ: check macosx/SConscript
    subdirs.append('macosx')

if env['PLATFORM'] == 'windows': # FIXME FOR SLETZ: create/check macosx/SConscript
    subdirs.append('windows')

if env['BUILD_EXAMPLES']:
    subdirs.append('example-clients')

if env['BUILD_TESTS']:
    subdirs.append('tests')

if build_base:
    env.SConscript( dirs=subdirs, exports="env", build_dir=build_base+subdir )
else:
    env.SConscript( dirs=subdirs, exports="env" )

