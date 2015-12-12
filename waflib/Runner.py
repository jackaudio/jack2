#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2005-2010 (ita)

"""
Runner.py: Task scheduling and execution

"""

import random, atexit
try:
	from queue import Queue
except ImportError:
	from Queue import Queue
from waflib import Utils, Task, Errors, Logs

GAP = 10
"""
Wait for free tasks if there are at least ``GAP * njobs`` in queue
"""

class TaskConsumer(Utils.threading.Thread):
	"""
	Task consumers belong to a pool of workers

	They wait for tasks in the queue and then use ``task.process(...)``
	"""
	def __init__(self):
		Utils.threading.Thread.__init__(self)
		self.ready = Queue()
		"""
		Obtain :py:class:`waflib.Task.TaskBase` instances from this queue.
		"""
		self.setDaemon(1)
		self.start()

	def run(self):
		"""
		Loop over the tasks to execute
		"""
		try:
			self.loop()
		except Exception:
			pass

	def loop(self):
		"""
		Obtain tasks from :py:attr:`waflib.Runner.TaskConsumer.ready` and call
		:py:meth:`waflib.Task.TaskBase.process`. If the object is a function, execute it.
		"""
		while 1:
			tsk = self.ready.get()
			if not isinstance(tsk, Task.TaskBase):
				tsk(self)
			else:
				tsk.process()

pool = Queue()
"""
Pool of task consumer objects
"""

def get_pool():
	"""
	Obtain a task consumer from :py:attr:`waflib.Runner.pool`.
	Do not forget to put it back by using :py:func:`waflib.Runner.put_pool`
	and reset properly (original waiting queue).

	:rtype: :py:class:`waflib.Runner.TaskConsumer`
	"""
	try:
		return pool.get(False)
	except Exception:
		return TaskConsumer()

def put_pool(x):
	"""
	Return a task consumer to the thread pool :py:attr:`waflib.Runner.pool`

	:param x: task consumer object
	:type x: :py:class:`waflib.Runner.TaskConsumer`
	"""
	pool.put(x)

def _free_resources():
	global pool
	lst = []
	while pool.qsize():
		lst.append(pool.get())
	for x in lst:
		x.ready.put(None)
	for x in lst:
		x.join()
	pool = None
atexit.register(_free_resources)

class Parallel(object):
	"""
	Schedule the tasks obtained from the build context for execution.
	"""
	def __init__(self, bld, j=2):
		"""
		The initialization requires a build context reference
		for computing the total number of jobs.
		"""

		self.numjobs = j
		"""
		Number of consumers in the pool
		"""

		self.bld = bld
		"""
		Instance of :py:class:`waflib.Build.BuildContext`
		"""

		self.outstanding = []
		"""List of :py:class:`waflib.Task.TaskBase` that may be ready to be executed"""

		self.frozen = []
		"""List of :py:class:`waflib.Task.TaskBase` that cannot be executed immediately"""

		self.out = Queue(0)
		"""List of :py:class:`waflib.Task.TaskBase` returned by the task consumers"""

		self.count = 0
		"""Amount of tasks that may be processed by :py:class:`waflib.Runner.TaskConsumer`"""

		self.processed = 1
		"""Amount of tasks processed"""

		self.stop = False
		"""Error flag to stop the build"""

		self.error = []
		"""Tasks that could not be executed"""

		self.biter = None
		"""Task iterator which must give groups of parallelizable tasks when calling ``next()``"""

		self.dirty = False
		"""Flag to indicate that tasks have been executed, and that the build cache must be saved (call :py:meth:`waflib.Build.BuildContext.store`)"""

	def get_next_task(self):
		"""
		Obtain the next task to execute.

		:rtype: :py:class:`waflib.Task.TaskBase`
		"""
		if not self.outstanding:
			return None
		return self.outstanding.pop(0)

	def postpone(self, tsk):
		"""
		A task cannot be executed at this point, put it in the list :py:attr:`waflib.Runner.Parallel.frozen`.

		:param tsk: task
		:type tsk: :py:class:`waflib.Task.TaskBase`
		"""
		if random.randint(0, 1):
			self.frozen.insert(0, tsk)
		else:
			self.frozen.append(tsk)

	def refill_task_list(self):
		"""
		Put the next group of tasks to execute in :py:attr:`waflib.Runner.Parallel.outstanding`.
		"""
		while self.count > self.numjobs * GAP:
			self.get_out()

		while not self.outstanding:
			if self.count:
				self.get_out()
			elif self.frozen:
				try:
					cond = self.deadlock == self.processed
				except AttributeError:
					pass
				else:
					if cond:
						msg = 'check the build order for the tasks'
						for tsk in self.frozen:
							if not tsk.run_after:
								msg = 'check the methods runnable_status'
								break
						lst = []
						for tsk in self.frozen:
							lst.append('%s\t-> %r' % (repr(tsk), [id(x) for x in tsk.run_after]))
						raise Errors.WafError('Deadlock detected: %s%s' % (msg, ''.join(lst)))
				self.deadlock = self.processed

			if self.frozen:
				self.outstanding += self.frozen
				self.frozen = []
			elif not self.count:
				self.outstanding.extend(next(self.biter))
				self.total = self.bld.total()
				break

	def add_more_tasks(self, tsk):
		"""
		Tasks may be added dynamically during the build by binding them to the task :py:attr:`waflib.Task.TaskBase.more_tasks`

		:param tsk: task
		:type tsk: :py:attr:`waflib.Task.TaskBase`
		"""
		if getattr(tsk, 'more_tasks', None):
			self.outstanding += tsk.more_tasks
			self.total += len(tsk.more_tasks)

	def get_out(self):
		"""
		Obtain one task returned from the task consumers, and update the task count. Add more tasks if necessary through
		:py:attr:`waflib.Runner.Parallel.add_more_tasks`.

		:rtype: :py:attr:`waflib.Task.TaskBase`
		"""
		tsk = self.out.get()
		if not self.stop:
			self.add_more_tasks(tsk)
		self.count -= 1
		self.dirty = True
		return tsk

	def add_task(self, tsk):
		"""
		Pass a task to a consumer.

		:param tsk: task
		:type tsk: :py:attr:`waflib.Task.TaskBase`
		"""
		try:
			self.pool
		except AttributeError:
			self.init_task_pool()
		self.ready.put(tsk)

	def init_task_pool(self):
		# lazy creation, and set a common pool for all task consumers
		pool = self.pool = [get_pool() for i in range(self.numjobs)]
		self.ready = Queue(0)
		def setq(consumer):
			consumer.ready = self.ready
		for x in pool:
			x.ready.put(setq)
		return pool

	def free_task_pool(self):
		# return the consumers, setting a different queue for each of them
		def setq(consumer):
			consumer.ready = Queue(0)
			self.out.put(self)
		try:
			pool = self.pool
		except AttributeError:
			pass
		else:
			for x in pool:
				self.ready.put(setq)
			for x in pool:
				self.get_out()
			for x in pool:
				put_pool(x)
			self.pool = []

	def skip(self, tsk):
		tsk.hasrun = Task.SKIPPED

	def error_handler(self, tsk):
		"""
		Called when a task cannot be executed. The flag :py:attr:`waflib.Runner.Parallel.stop` is set, unless
		the build is executed with::

			$ waf build -k

		:param tsk: task
		:type tsk: :py:attr:`waflib.Task.TaskBase`
		"""
		if hasattr(tsk, 'scan') and hasattr(tsk, 'uid'):
			# TODO waf 1.9 - this breaks encapsulation
			key = (tsk.uid(), 'imp')
			try:
				del self.bld.task_sigs[key]
			except KeyError:
				pass
		if not self.bld.keep:
			self.stop = True
		self.error.append(tsk)

	def task_status(self, tsk):
		try:
			return tsk.runnable_status()
		except Exception:
			self.processed += 1
			tsk.err_msg = Utils.ex_stack()
			if not self.stop and self.bld.keep:
				self.skip(tsk)
				if self.bld.keep == 1:
					# if -k stop at the first exception, if -kk try to go as far as possible
					if Logs.verbose > 1 or not self.error:
						self.error.append(tsk)
					self.stop = True
				else:
					if Logs.verbose > 1:
						self.error.append(tsk)
				return Task.EXCEPTION
			tsk.hasrun = Task.EXCEPTION

			self.error_handler(tsk)
			return Task.EXCEPTION

	def start(self):
		"""
		Give tasks to :py:class:`waflib.Runner.TaskConsumer` instances until the build finishes or the ``stop`` flag is set.
		If only one job is used, then execute the tasks one by one, without consumers.
		"""

		self.total = self.bld.total()

		while not self.stop:

			self.refill_task_list()

			# consider the next task
			tsk = self.get_next_task()
			if not tsk:
				if self.count:
					# tasks may add new ones after they are run
					continue
				else:
					# no tasks to run, no tasks running, time to exit
					break

			if tsk.hasrun:
				# if the task is marked as "run", just skip it
				self.processed += 1
				continue

			if self.stop: # stop immediately after a failure was detected
				break


			st = self.task_status(tsk)
			if st == Task.RUN_ME:
				tsk.position = (self.processed, self.total)
				self.count += 1
				tsk.master = self
				self.processed += 1

				if self.numjobs == 1:
					tsk.process()
				else:
					self.add_task(tsk)
			if st == Task.ASK_LATER:
				self.postpone(tsk)
			elif st == Task.SKIP_ME:
				self.processed += 1
				self.skip(tsk)
				self.add_more_tasks(tsk)

		# self.count represents the tasks that have been made available to the consumer threads
		# collect all the tasks after an error else the message may be incomplete
		while self.error and self.count:
			self.get_out()

		#print loop
		assert (self.count == 0 or self.stop)

		# free the task pool, if any
		self.free_task_pool()

