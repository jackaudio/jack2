#! /usr/bin/env python

"""
Illustrate how to override a class method to do something

In this case, print the commands being executed as strings
(the commands are usually lists, so this can be misleading)
"""

import sys
from waflib import Context, Utils, Logs

def exec_command(self, cmd, **kw):
	subprocess = Utils.subprocess
	kw['shell'] = isinstance(cmd, str)

	txt = cmd
	if isinstance(cmd, list):
		txt = ' '.join(cmd)

	print(txt)
	Logs.debug('runner_env: kw=%s' % kw)

	try:
		if self.logger:
			# warning: may deadlock with a lot of output (subprocess limitation)

			self.logger.info(cmd)

			kw['stdout'] = kw['stderr'] = subprocess.PIPE
			p = subprocess.Popen(cmd, **kw)
			(out, err) = p.communicate()
			if out:
				self.logger.debug('out: %s' % out.decode(sys.stdout.encoding or 'iso8859-1'))
			if err:
				self.logger.error('err: %s' % err.decode(sys.stdout.encoding or 'iso8859-1'))
			return p.returncode
		else:
			p = subprocess.Popen(cmd, **kw)
			return p.wait()
	except OSError:
		return -1

Context.Context.exec_command = exec_command


