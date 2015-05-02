#!/usr/bin/python
# -*- coding: utf-8 -*-
# Tool to provide dedicated variables for cross-compilation

__author__ = __maintainer__ = "Jérôme Carretero <cJ-waf@zougloub.eu>"
__copyright__ = "Jérôme Carretero, 2014"

"""

This tool allows to use environment variables to define cross-compilation things,
mostly used when you use build variants.

Usage:

- In your build script::

    def configure(cfg):
      ...
      conf.load('c_cross_gnu')
      for variant in x_variants:
        conf.xcheck_host()
        conf.xcheck_host_var('POUET')
        ...

      ...

- Then::

    CHOST=arm-hardfloat-linux-gnueabi waf configure

    env arm-hardfloat-linux-gnueabi-CC="clang -..." waf configure

    CFLAGS=... CHOST=arm-hardfloat-linux-gnueabi HOST_CFLAGS=-g waf configure

    HOST_CC="clang -..." waf configure

"""

import os
from waflib import Utils, Configure

try:
	from shlex import quote
except ImportError:
	from pipes import quote

@Configure.conf
def xcheck_prog(conf, var, tool, cross=False):
	value = os.environ.get(var, '')
	value = Utils.to_list(value)

	if not value:
		return

	conf.env[var] = value
	if cross:
		pretty = 'cross-compilation %s' % var
	else:
		pretty = var
	conf.msg('Will use %s' % pretty,
	 " ".join(quote(x) for x in value))

@Configure.conf
def xcheck_envar(conf, name, wafname=None, cross=False):
	wafname = wafname or name
	value = os.environ.get(name, None)
	value = Utils.to_list(value)

	if not value:
		return

	conf.env[wafname] += value
	if cross:
		pretty = 'cross-compilation %s' % wafname
	else:
		pretty = wafname
	conf.msg('Will use %s' % pretty,
	 " ".join(quote(x) for x in value))

@Configure.conf
def xcheck_host_prog(conf, name, tool, wafname=None):
	wafname = wafname or name
	host = conf.env.CHOST
	specific = None
	if host:
		specific = os.environ.get('%s-%s' % (host[0], name), None)

	if specific:
		value = Utils.to_list(specific)
		conf.env[wafname] += value
		conf.msg('Will use cross-compilation %s' % name,
		 " ".join(quote(x) for x in value))
		return

	conf.xcheck_prog('HOST_%s' % name, tool, cross=True)

	if conf.env[wafname]:
		return

	value = None
	if host:
		value = '%s-%s' % (host[0], tool)

	if value:
		conf.env[wafname] = value
		conf.msg('Will use cross-compilation %s' % wafname, value)

@Configure.conf
def xcheck_host_envar(conf, name, wafname=None):
	wafname = wafname or name

	host = conf.env.CHOST
	specific = None
	if host:
		specific = os.environ.get('%s-%s' % (host[0], name), None)

	if specific:
		value = Utils.to_list(specific)
		conf.env[wafname] += value
		conf.msg('Will use cross-compilation %s' % name,
		 " ".join(quote(x) for x in value))
		return

	conf.xcheck_envar('HOST_%s' % name, wafname, cross=True)


@Configure.conf
def xcheck_host(conf):
	conf.xcheck_envar('CHOST', cross=True)
	conf.xcheck_host_prog('CC', 'gcc')
	conf.xcheck_host_prog('CXX', 'g++')
	conf.xcheck_host_prog('LINK_CC', 'gcc')
	conf.xcheck_host_prog('LINK_CXX', 'g++')
	conf.xcheck_host_prog('AR', 'ar')
	conf.xcheck_host_prog('AS', 'as')
	conf.xcheck_host_prog('LD', 'ld')
	conf.xcheck_host_envar('CFLAGS')
	conf.xcheck_host_envar('CXXFLAGS')
	conf.xcheck_host_envar('LDFLAGS', 'LINKFLAGS')
	conf.xcheck_host_envar('LIB')
	conf.xcheck_host_envar('PKG_CONFIG_PATH')
	# TODO find a better solution than this ugliness
	if conf.env.PKG_CONFIG_PATH:
		conf.find_program('pkg-config', var='PKGCONFIG')
		conf.env.PKGCONFIG = [
		 'env', 'PKG_CONFIG_PATH=%s' % (conf.env.PKG_CONFIG_PATH[0])
		] + conf.env.PKGCONFIG
