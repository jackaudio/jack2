#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2005-2010 (ita)

"base for all c/c++ programs and libraries"

import os, sys, re
from waflib import Utils, Build
from waflib.Configure import conf

def get_extensions(lst):
	"""
	:param lst: files to process
	:list lst: list of string or :py:class:`waflib.Node.Node`
	:return: list of file extensions
	:rtype: list of string
	"""
	ret = []
	for x in Utils.to_list(lst):
		try:
			if not isinstance(x, str):
				x = x.name
			ret.append(x[x.rfind('.') + 1:])
		except Exception:
			pass
	return ret

def sniff_features(**kw):
	"""
	Look at the source files and return the features for a task generator (mainly cc and cxx)::

		snif_features(source=['foo.c', 'foo.cxx'], type='shlib')
		# returns  ['cxx', 'c', 'cxxshlib', 'cshlib']

	:param source: source files to process
	:type source: list of string or :py:class:`waflib.Node.Node`
	:param type: object type in *program*, *shlib* or *stlib*
	:type type: string
	:return: the list of features for a task generator processing the source files
	:rtype: list of string
	"""
	exts = get_extensions(kw['source'])
	type = kw['_type']
	feats = []

	# watch the order, cxx will have the precedence
	if 'cxx' in exts or 'cpp' in exts or 'c++' in exts or 'cc' in exts or 'C' in exts:
		feats.append('cxx')

	if 'c' in exts or 'vala' in exts:
		feats.append('c')

	if 'd' in exts:
		feats.append('d')

	if 'java' in exts:
		feats.append('java')

	if 'java' in exts:
		return 'java'

	if type in ('program', 'shlib', 'stlib'):
		for x in feats:
			if x in ('cxx', 'd', 'c'):
				feats.append(x + type)

	return feats

def set_features(kw, _type):
	kw['_type'] = _type
	kw['features'] = Utils.to_list(kw.get('features', [])) + Utils.to_list(sniff_features(**kw))

@conf
def program(bld, *k, **kw):
	"""
	Alias for creating programs by looking at the file extensions::

		def build(bld):
			bld.program(source='foo.c', target='app')
			# equivalent to:
			# bld(features='c cprogram', source='foo.c', target='app')

	"""
	set_features(kw, 'program')
	return bld(*k, **kw)

@conf
def shlib(bld, *k, **kw):
	"""
	Alias for creating shared libraries by looking at the file extensions::

		def build(bld):
			bld.shlib(source='foo.c', target='app')
			# equivalent to:
			# bld(features='c cshlib', source='foo.c', target='app')

	"""
	set_features(kw, 'shlib')
	return bld(*k, **kw)

@conf
def stlib(bld, *k, **kw):
	"""
	Alias for creating static libraries by looking at the file extensions::

		def build(bld):
			bld.stlib(source='foo.cpp', target='app')
			# equivalent to:
			# bld(features='cxx cxxstlib', source='foo.cpp', target='app')

	"""
	set_features(kw, 'stlib')
	return bld(*k, **kw)

@conf
def objects(bld, *k, **kw):
	"""
	Alias for creating object files by looking at the file extensions::

		def build(bld):
			bld.objects(source='foo.c', target='app')
			# equivalent to:
			# bld(features='c', source='foo.c', target='app')

	"""
	set_features(kw, 'objects')
	return bld(*k, **kw)

