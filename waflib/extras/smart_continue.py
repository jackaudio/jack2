#! /usr/bin/env python
# Thomas Nagy, 2011

# Try to cancel the tasks that cannot run with the option -k when an error occurs:
# 1 direct file dependencies
# 2 tasks listed in the before/after/ext_in/ext_out attributes

from waflib import Task, Runner

Task.CANCELED = 4

def cancel_next(self, tsk):
	if not isinstance(tsk, Task.TaskBase):
		return
	if tsk.hasrun >= Task.SKIPPED:
		# normal execution, no need to do anything here
		return

	try:
		canceled_tasks, canceled_nodes = self.canceled_tasks, self.canceled_nodes
	except AttributeError:
		canceled_tasks = self.canceled_tasks = set([])
		canceled_nodes = self.canceled_nodes = set([])

	try:
		canceled_nodes.update(tsk.outputs)
	except AttributeError:
		pass

	try:
		canceled_tasks.add(tsk)
	except AttributeError:
		pass

def get_out(self):
	tsk = self.out.get()
	if not self.stop:
		self.add_more_tasks(tsk)
	self.count -= 1
	self.dirty = True
	self.cancel_next(tsk) # new code

def error_handler(self, tsk):
	if not self.bld.keep:
		self.stop = True
	self.error.append(tsk)
	self.cancel_next(tsk) # new code

Runner.Parallel.cancel_next = cancel_next
Runner.Parallel.get_out = get_out
Runner.Parallel.error_handler = error_handler

def get_next_task(self):
	tsk = self.get_next_task_smart_continue()
	if not tsk:
		return tsk

	try:
		canceled_tasks, canceled_nodes = self.canceled_tasks, self.canceled_nodes
	except AttributeError:
		pass
	else:
		# look in the tasks that this one is waiting on
		# if one of them was canceled, cancel this one too
		for x in tsk.run_after:
			if x in canceled_tasks:
				tsk.hasrun = Task.CANCELED
				self.cancel_next(tsk)
				break
		else:
			# so far so good, now consider the nodes
			for x in getattr(tsk, 'inputs', []) + getattr(tsk, 'deps', []):
				if x in canceled_nodes:
					tsk.hasrun = Task.CANCELED
					self.cancel_next(tsk)
					break
	return tsk

Runner.Parallel.get_next_task_smart_continue = Runner.Parallel.get_next_task
Runner.Parallel.get_next_task = get_next_task

