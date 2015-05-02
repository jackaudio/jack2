#! /usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2015 (ita)

"""
Execute commands through pre-forked servers. This tool creates as many servers as build threads.
On a benchmark executed on Linux Kubuntu 14, 8 virtual cores and SSD drive::

    ./genbench.py /tmp/build 200 100 15 5
    waf clean build -j24
    # no prefork: 2m7.179s
    # prefork:    0m55.400s

To use::

    def options(opt):
        # optional, will spawn 40 servers early
        opt.load('prefork')

    def build(bld):
        bld.load('prefork')
        ...
        more code

The servers and the build process are using a shared nonce to prevent undesirable external connections.
"""

import os, re, socket, threading, sys, subprocess, time, atexit, traceback, random, signal
try:
	import SocketServer
except ImportError:
	import socketserver as SocketServer
try:
	from queue import Queue
except ImportError:
	from Queue import Queue
try:
	import cPickle
except ImportError:
	import pickle as cPickle

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

def safe_compare(x, y):
	sum = 0
	for (a, b) in zip(x, y):
		sum |= ord(a) ^ ord(b)
	return sum == 0

re_valid_query = re.compile('^[a-zA-Z0-9_, ]+$')
class req(SocketServer.StreamRequestHandler):
	def handle(self):
		try:
			while self.process_command():
				pass
		except KeyboardInterrupt:
			return
		except Exception as e:
			print(e)

	def send_response(self, ret, out, err, exc):
		if out or err or exc:
			data = (out, err, exc)
			data = cPickle.dumps(data, -1)
		else:
			data = ''

		params = [RES, str(ret), str(len(data))]

		# no need for the cookie in the response
		self.wfile.write(make_header(params))
		if data:
			self.wfile.write(data)
		self.wfile.flush()

	def process_command(self):
		query = self.rfile.read(HEADER_SIZE)
		if not query:
			return None
		#print(len(query))
		assert(len(query) == HEADER_SIZE)
		if sys.hexversion > 0x3000000:
			query = query.decode('iso8859-1')

		# magic cookie
		key = query[-20:]
		if not safe_compare(key, SHARED_KEY):
			print('%r %r' % (key, SHARED_KEY))
			self.send_response(-1, '', '', 'Invalid key given!')
			return 'meh'

		query = query[:-20]
		#print "%r" % query
		if not re_valid_query.match(query):
			self.send_response(-1, '', '', 'Invalid query %r' % query)
			raise ValueError('Invalid query %r' % query)

		query = query.strip().split(',')

		if query[0] == REQ:
			self.run_command(query[1:])
		elif query[0] == BYE:
			raise ValueError('Exit')
		else:
			raise ValueError('Invalid query %r' % query)
		return 'ok'

	def run_command(self, query):

		size = int(query[0])
		data = self.rfile.read(size)
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

		self.send_response(ret, out, err, exc)

def create_server(conn, cls):
	# child processes do not need the key, so we remove it from the OS environment
	global SHARED_KEY
	SHARED_KEY = os.environ['SHARED_KEY']
	os.environ['SHARED_KEY'] = ''

	ppid = int(os.environ['PREFORKPID'])
	def reap():
		if os.sep != '/':
			os.waitpid(ppid, 0)
		else:
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

	server = SocketServer.TCPServer(conn, req)
	print(server.server_address[1])
	sys.stdout.flush()
	#server.timeout = 6000 # seconds
	server.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
	try:
		server.serve_forever(poll_interval=0.001)
	except KeyboardInterrupt:
		pass

if __name__ == '__main__':
	conn = ("127.0.0.1", 0)
	#print("listening - %r %r\n" % conn)
	create_server(conn, req)
else:

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
		cmd = [sys.executable, os.path.abspath(__file__)]
		proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
		return proc

	def make_conn(bld, srv):
		port = srv.port
		conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
		conn.connect(('127.0.0.1', port))
		return conn


	SERVERS = []
	CONNS = []
	def close_all():
		global SERVERS, CONNS
		while CONNS:
			conn = CONNS.pop()
			try:
				conn.close()
			except:
				pass
		while SERVERS:
			srv = SERVERS.pop()
			try:
				srv.kill()
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
		header = make_header(params, self.SHARED_KEY)

		conn = CONNS[idx]

		put_data(conn, header + data)
		#put_data(conn, data)

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

	def init_key(ctx):
		try:
			key = ctx.SHARED_KEY = os.environ['SHARED_KEY']
		except KeyError:
			key = "".join([chr(random.SystemRandom().randint(40, 126)) for x in range(20)])
			os.environ['SHARED_KEY'] = ctx.SHARED_KEY = key

		os.environ['PREFORKPID'] = str(os.getpid())
		return key

	def init_servers(ctx, maxval):
		while len(SERVERS) < maxval:
			i = len(SERVERS)
			srv = make_server(ctx, i)
			SERVERS.append(srv)
		while len(CONNS) < maxval:
			i = len(CONNS)
			srv = SERVERS[i]

			# postpone the connection
			srv.port = int(srv.stdout.readline())

			conn = None
			for x in range(30):
				try:
					conn = make_conn(ctx, srv)
					break
				except socket.error:
					time.sleep(0.01)
			if not conn:
				raise ValueError('Could not start the server!')
			if srv.poll() is not None:
				Logs.warn('Looks like it it not our server process - concurrent builds are unsupported at this stage')
				raise ValueError('Could not start the server')
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
		init_key(opt)
		init_servers(opt, 40)
		opt.add_option('--pin-process', action='store_true', dest='smp', default=False)

	def build(bld):
		if bld.cmd == 'clean':
			return

		init_key(bld)
		init_servers(bld, bld.jobs)
		init_smp(bld)

		bld.__class__.exec_command_old = bld.__class__.exec_command
		bld.__class__.exec_command = exec_command

