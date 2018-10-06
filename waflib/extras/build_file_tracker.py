#! /usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2015

"""
Force files to depend on the timestamps of those located in the build directory. You may
want to use this to force partial rebuilds, see playground/track_output_files/ for a working example.

Note that there is a variety of ways to implement this, one may want use timestamps on source files too for example,
or one may want to hash the files in the source directory only under certain conditions (md5_tstamp tool)
or to hash the file in the build directory with its timestamp (similar to 'update_outputs')
"""

import os
from waflib import Node, Utils

def get_bld_sig(self):
	try:
		return self.cache_sig
	except AttributeError:
		pass

	if not self.is_bld() or self.ctx.bldnode is self.ctx.srcnode:
		self.sig = Utils.h_file(self.abspath())
		self.cache_sig = ret = self.sig
	else:
		# add the
		self.cache_sig = ret = self.sig + str(os.stat(self.abspath()).st_mtime)
	return ret

Node.Node.get_bld_sig = get_bld_sig

