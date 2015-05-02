#! /usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2015 (ita)

"""
A version of prefork.py that uses unix sockets. The advantage is that it does not expose
connections to the outside. Yet performance it only works on unix-like systems
and performance can be slightly worse.

To use::

    def options(opt):
        # recommended, fork new processes before using more memory
        opt.load('preforkunix')

    def build(bld):
        bld.load('preforkunix')
        ...
        more code
"""

import os, re, socket, threading, sys, subprocess, atexit, traceback, signal, time
try:
	from queue import Queue
except ImportError:
	from Queue import Queue
try:
	import cPickle
except ImportError:
	import pickle as cPickle

HEADER_SIZE = 20

REQ = 'REQ'
RES = 'RES'
BYE = 'BYE'

def make_header(params, cookie=''):
	header = ','.join(params)
	header = header.ljust(HEADER_SIZE - len(cookie))
	assert(len(header) == HEADER_SIZE - len(cookie))
	header = header + cookie
	if sys.hexversion > 0x3000000:
		header = header.encode('iso8859-1')
	return header

re_valid_query = re.compile('^[a-zA-Z0-9_, ]+$')
if 1:
	def send_response(conn, ret, out, err, exc):
		if out or err or exc:
			data = (out, err, exc)
			data = cPickle.dumps(data, -1)
		else:
			data = ''

		params = [RES, str(ret), str(len(data))]

		# no need for the cookie in the response
		conn.send(make_header(params))
		if data:
			conn.send(data)

	def process_command(conn):
		query = conn.recv(HEADER_SIZE)
		if not query:
			return None
		#print(len(query))
		assert(len(query) == HEADER_SIZE)
		if sys.hexversion > 0x3000000:
			query = query.decode('iso8859-1')

		#print "%r" % query
		if not re_valid_query.match(query):
			send_response(conn, -1, '', '', 'Invalid query %r' % query)
			raise ValueError('Invalid query %r' % query)

		query = query.strip().split(',')

		if query[0] == REQ:
			run_command(conn, query[1:])
		elif query[0] == BYE:
			raise ValueError('Exit')
		else:
			raise ValueError('Invalid query %r' % query)
		return 'ok'

	def run_command(conn, query):

		size = int(query[0])
		data = conn.recv(size)
		assert(len(data) == size)
		kw = cPickle.loads(data)

		# run command
		ret = out = err = exc = None
		cmd = kw['cmd']
		del kw['cmd']
		#print(cmd)

		try:
			if kw['stdout'] or kw['stderr']:
				p = subprocess.Popen(cmd, **kw)
				(out, err) = p.communicate()
				ret = p.returncode
			else:
				ret = subprocess.Popen(cmd, **kw).wait()
		except KeyboardInterrupt:
			raise
		except Exception as e:
			ret = -1
			exc = str(e) + traceback.format_exc()

		send_response(conn, ret, out, err, exc)

if 1:

	from waflib import Logs, Utils, Runner, Errors, Options

	def init_task_pool(self):
		# lazy creation, and set a common pool for all task consumers
		pool = self.pool = []
		for i in range(self.numjobs):
			consumer = Runner.get_pool()
			pool.append(consumer)
			consumer.idx = i
		self.ready = Queue(0)
		def setq(consumer):
			consumer.ready = self.ready
			try:
				threading.current_thread().idx = consumer.idx
			except Exception as e:
				print(e)
		for x in pool:
			x.ready.put(setq)
		return pool
	Runner.Parallel.init_task_pool = init_task_pool

	def make_conn(bld):
		child_socket, parent_socket = socket.socketpair(socket.AF_UNIX)
		ppid = os.getpid()
		pid = os.fork()
		if pid == 0:
			parent_socket.close()

			# if the parent crashes, try to exit cleanly
			def reap():
				while 1:
					try:
						os.kill(ppid, 0)
					except OSError:
						break
					else:
						time.sleep(1)
				os.kill(os.getpid(), signal.SIGKILL)
			t = threading.Thread(target=reap)
			t.setDaemon(True)
			t.start()

			# write to child_socket only
			try:
				while process_command(child_socket):
					pass
			except KeyboardInterrupt:
				sys.exit(2)
		else:
			child_socket.close()
			return (pid, parent_socket)

	SERVERS = []
	CONNS = []
	def close_all():
		global SERVERS, CONS
		while CONNS:
			conn = CONNS.pop()
			try:
				conn.close()
			except:
				pass
		while SERVERS:
			pid = SERVERS.pop()
			try:
				os.kill(pid, 9)
			except:
				pass
	atexit.register(close_all)

	def put_data(conn, data):
		cnt = 0
		while cnt < len(data):
			sent = conn.send(data[cnt:])
			if sent == 0:
				raise RuntimeError('connection ended')
			cnt += sent

	def read_data(conn, siz):
		cnt = 0
		buf = []
		while cnt < siz:
			data = conn.recv(min(siz - cnt, 1024))
			if not data:
				raise RuntimeError('connection ended %r %r' % (cnt, siz))
			buf.append(data)
			cnt += len(data)
		if sys.hexversion > 0x3000000:
			ret = ''.encode('iso8859-1').join(buf)
		else:
			ret = ''.join(buf)
		return ret

	def exec_command(self, cmd, **kw):
		if 'stdout' in kw:
			if kw['stdout'] not in (None, subprocess.PIPE):
				return self.exec_command_old(cmd, **kw)
		elif 'stderr' in kw:
			if kw['stderr'] not in (None, subprocess.PIPE):
				return self.exec_command_old(cmd, **kw)

		kw['shell'] = isinstance(cmd, str)
		Logs.debug('runner: %r' % cmd)
		Logs.debug('runner_env: kw=%s' % kw)

		if self.logger:
			self.logger.info(cmd)

		if 'stdout' not in kw:
			kw['stdout'] = subprocess.PIPE
		if 'stderr' not in kw:
			kw['stderr'] = subprocess.PIPE

		if Logs.verbose and not kw['shell'] and not Utils.check_exe(cmd[0]):
			raise Errors.WafError("Program %s not found!" % cmd[0])

		idx = threading.current_thread().idx
		kw['cmd'] = cmd

		# serialization..
		#print("sub %r %r" % (idx, cmd))
		#print("write to %r %r" % (idx, cmd))

		data = cPickle.dumps(kw, -1)
		params = [REQ, str(len(data))]
		header = make_header(params)

		conn = CONNS[idx]

		put_data(conn, header + data)

		#print("running %r %r" % (idx, cmd))
		#print("read from %r %r" % (idx, cmd))

		data = read_data(conn, HEADER_SIZE)
		if sys.hexversion > 0x3000000:
			data = data.decode('iso8859-1')

		#print("received %r" % data)
		lst = data.split(',')
		ret = int(lst[1])
		dlen = int(lst[2])

		out = err = None
		if dlen:
			data = read_data(conn, dlen)
			(out, err, exc) = cPickle.loads(data)
			if exc:
				raise Errors.WafError('Execution failure: %s' % exc)

		if out:
			if not isinstance(out, str):
				out = out.decode(sys.stdout.encoding or 'iso8859-1')
			if self.logger:
				self.logger.debug('out: %s' % out)
			else:
				Logs.info(out, extra={'stream':sys.stdout, 'c1': ''})
		if err:
			if not isinstance(err, str):
				err = err.decode(sys.stdout.encoding or 'iso8859-1')
			if self.logger:
				self.logger.error('err: %s' % err)
			else:
				Logs.info(err, extra={'stream':sys.stderr, 'c1': ''})

		return ret

	def init_smp(self):
		if not getattr(Options.options, 'smp', getattr(self, 'smp', None)):
			return
		if Utils.unversioned_sys_platform() in ('freebsd',):
			pid = os.getpid()
			cmd = ['cpuset', '-l', '0', '-p', str(pid)]
		elif Utils.unversioned_sys_platform() in ('linux',):
			pid = os.getpid()
			cmd = ['taskset', '-pc', '0', str(pid)]
		if cmd:
			self.cmd_and_log(cmd, quiet=0)

	def options(opt):
		# memory consumption might be at the lowest point while processing options
		opt.add_option('--pin-process', action='store_true', dest='smp', default=False)
		if Utils.is_win32 or os.sep != '/':
			return
		while len(CONNS) < 30:
			(pid, conn) = make_conn(opt)
			SERVERS.append(pid)
			CONNS.append(conn)

	def build(bld):
		if Utils.is_win32 or os.sep != '/':
			return
		if bld.cmd == 'clean':
			return
		while len(CONNS) < bld.jobs:
			(pid, conn) = make_conn(bld)
			SERVERS.append(pid)
			CONNS.append(conn)
		init_smp(bld)
		bld.__class__.exec_command_old = bld.__class__.exec_command
		bld.__class__.exec_command = exec_command

