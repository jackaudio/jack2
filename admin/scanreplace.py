#!/usr/bin/python
#
# Copyright (C) 2007 Arnold Krille
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

#
# Taken from http://www.scons.org/wiki/ReplacementBuilder
#

from string import Template

def replace_action(target, source, env):
	open( str(target[0]), 'w' ).write( Template( open( str(source[0]), 'r' ).read() ).safe_substitute( env ) )
	return 0

def replace_string(target, source, env):
	return "building '%s' from '%s'" % ( str(target[0]), str(source[0]) )

def generate(env, **kw):
	action = env.Action( replace_action, replace_string )
	env['BUILDERS']['ScanReplace'] = env.Builder( action=action, src_suffix='.in', single_source=True )

def exists(env):
	return 1

#!/usr/bin/python
#
# Copyright (C) 2007 Arnold Krille
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

#
# Taken from http://www.scons.org/wiki/ReplacementBuilder
#

from string import Template

def replace_action(target, source, env):
	open( str(target[0]), 'w' ).write( Template( open( str(source[0]), 'r' ).read() ).safe_substitute( env ) )
	return 0

def replace_string(target, source, env):
	return "building '%s' from '%s'" % ( str(target[0]), str(source[0]) )

def generate(env, **kw):
	action = env.Action( replace_action, replace_string )
	env['BUILDERS']['ScanReplace'] = env.Builder( action=action, src_suffix='.in', single_source=True )

def exists(env):
	return 1

