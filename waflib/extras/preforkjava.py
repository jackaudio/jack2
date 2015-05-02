#! /usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2015 (ita)

# TODO: have the child process terminate if the parent is killed abruptly

import os, re, socket, threading, sys, subprocess, time, atexit, traceback, random
try:
	import SocketServer
except ImportError:
	import socketserver as SocketServer
try:
	from queue import Queue
except ImportError:
	from Queue import Queue

import json as pickle

SHARED_KEY = None
HEADER_SIZE = 64

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

	def make_server(bld, idx):
		top = getattr(bld, 'preforkjava_top', os.path.dirname(os.path.abspath('__file__')))
		cp = getattr(bld, 'preforkjava_cp', os.path.join(top, 'minimal-json-0.9.3-SNAPSHOT.jar') + os.pathsep + top)

		for x in cp.split(os.pathsep):
			if x and not os.path.exists(x):
				Logs.warn('Invalid classpath: %r' % cp)
				Logs.warn('Set for example bld.preforkjava_cp to /path/to/minimal-json:/path/to/Prefork.class/')

		cwd = getattr(bld, 'preforkjava_cwd', top)
		port = getattr(bld, 'preforkjava_port', 51200)
		cmd = getattr(bld, 'preforkjava_cmd', 'java -cp %s%s Prefork %d' % (cp, os.pathsep, port))
		proc = subprocess.Popen(cmd.split(), shell=False, cwd=cwd)
		proc.port = port
		return proc

	def make_conn(bld, srv):
		#port = PORT + idx
		port = srv.port
		conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
		conn.connect(('127.0.0.1', port))
		return conn

	SERVERS = []
	CONNS = []
	def close_all():
		global SERVERS
		while SERVERS:
			srv = SERVERS.pop()
			pid = srv.pid
			try:
				srv.kill()
			except Exception as e:
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

		data = pickle.dumps(kw)
		params = [REQ, str(len(data))]
		header = make_header(params, self.SHARED_KEY)

		conn = CONNS[idx]

		if sys.hexversion > 0x3000000:
			data = data.encode('iso8859-1')
		put_data(conn, header + data)

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
			(out, err, exc) = pickle.loads(data)
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

	def init_key(ctx):
		try:
			key = ctx.SHARED_KEY = os.environ['SHARED_KEY']
		except KeyError:
			key = "".join([chr(random.SystemRandom().randint(40, 126)) for x in range(20)])
			os.environ['SHARED_KEY'] = ctx.SHARED_KEY = key
		os.environ['PREFORKPID'] = str(os.getpid())
		return key

	def init_servers(ctx, maxval):
		while len(SERVERS) < 1:
			i = len(SERVERS)
			srv = make_server(ctx, i)
			SERVERS.append(srv)
		while len(CONNS) < maxval:
			i = len(CONNS)
			srv = SERVERS[0]
			conn = None
			for x in range(30):
				try:
					conn = make_conn(ctx, srv)
					break
				except socket.error:
					time.sleep(0.01)
			if not conn:
				raise ValueError('Could not start the server!')
			CONNS.append(conn)

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
		opt.add_option('--pin-process', action='store_true', dest='smp', default=False)
		init_key(opt)
		init_servers(opt, 40)

	def build(bld):
		if bld.cmd == 'clean':
			return

		init_key(bld)
		init_servers(bld, bld.jobs)
		init_smp(bld)

		bld.__class__.exec_command_old = bld.__class__.exec_command
		bld.__class__.exec_command = exec_command

