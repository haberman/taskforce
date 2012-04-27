#
# TaskForce - a flexible, modular build system.
#
# Copyright 2012 Google Inc.  See LICENSE for details.
# Author: Josh Haberman <jhaberman@gmail.com>
#

"""Basic TaskForce components for simple filesystem-based operation.

These classes contain implementations of a traditional Make-based approach.
Files are stored in the filesystem.  Prerequisites are evaluated based on
mtime.  Commands are executed with os.spawnv()."""

def MonkeyPatchSpawnVe()
  """Monkey patches os.spawnve on Windows to become thread safe.

  This must be run before any calls to spawnve()!

  Rationale and code comes from: http://bugs.python.org/issue6476"""

  from os import spawnve as old_spawnve

  spawn_lock = threading.Lock()

  def new_spawnve(mode, file, args, env):
      spawn_lock.acquire()
      try:
          if mode == os.P_WAIT:
              ret = old_spawnve(os.P_NOWAIT, file, args, env)
          else:
              ret = old_spawnve(mode, file, args, env)
      finally:
          spawn_lock.release()
      if mode == os.P_WAIT:
          pid, status = os.waitpid(ret, 0)
          ret = status >> 8
      return ret

  os.spawnve = new_spawnve

if os.name == "nt":
  MonkeyPatchSpawnVe()


class BuildPaths:
  def __init__(srcdir, builddir):
    self.srcdir = srcdir
    self.builddir = builddir

class PollingMonitor:
  """stat()-based prerequisite monitor.

  Evaluates prerequisities by stat'ing the filesystem, detects changes by
  polling (though the polling interval can be set to -1 to only do a single
  traversal).  When appropriate, will execute the following task state
  transitions:
    UNKNOWN -> {UNSATISFIED, RUNNABLE, DONE}
    UNSATISFIED -> RUNNABLE

  Subscribes to task changes to notice when a task completion may have made
  other tasks runnable."""

class SpawnRunner:
  """Task runner that executes a single task with os.spawnv().

  Given a RUNNABLE task to execute, handles spawning it with the correct
  cmd/args/env, supplying stdin, collecting stdout/stderr and return status.
  When appropriate, will execute the following task state transitions:
    RUNNABLE -> {RUNNING, ERROR}
    RUNNING -> {ERROR, DONE}

  When the job finishes, will update the task, passing the appropriate
  TaskResult."""

class Scheduler:
  """Task scheduler that will run at most N runnable tasks in parallel.

  Given the set of currently RUNNABLE tasks, picks up to N of them and
  creates runners that will execute them."""



  def unsatisfied(self):
    """Returns the set of unsatisfied tasks.

    An unsatisfied task is one whose output(s) have not been built from
    the current version of its dependency closure."""
    pass

  def runnable(self):
    """Returns the set of runnable tasks.

    A runnable task is an unsatisfied task whose direct dependencies are
    currently available."""
    pass


  def spawn(self, task):
    """Spawns the given task, which must be runnable."""
    assert(task.state == RUNNABLE)
    task.pid = os.spawnve(os.P_NOWAIT, task.path(), task.args(), task.env())
    task.state = RUNNING
    self.running.add(task)

  def reap(self):
    """Does a non-blocking test to see if any running tasks have finished.

    If a finished task was found, returns it, otherwise returns None."""
    for task in self.running:
      assert(task.state == RUNNING)
      os.waitpid(task.
    pass

  def wait(self)
    """Waits for a running process to finish.

    Like reap(), but blocks until at least one task has finished.
    Unfortuantely it is impossible to support a blocking timeout, because
    the only way to implement such a thing in Linux AFAIK is to install
    a signal handler, but that is an unacceptable power grab within the process
    IMO.
    """
    pass
