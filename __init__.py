"""Core TaskForce classes.

The following classes define the core TaskForce data model.  They are used by
all builds and are independent of any build/configuration strategy.
Specifically these classes make no assumptions about:

 * how tasks are created/defined
 * how to evaluate whether something needs to be rebuilt
 * how to receive notifications that a file is changed
 * how commands are invoked and how their results are collected
 * how inputs/output files are stored
 * where the output directory lives in relation to the source directory

At this level of abstraction we are capable of building a dependency graph,
making sure it doesn't have any cycles, and doing sanity checks on the task
states and transitions (eg. a DONE task should not depend on a RUNNING or
UNSATISFIED task)."""

class Task:
  """Object that represents a single task that produces output file(s)."""

  def __init__(self, targets, sources, cmd, env, **kwargs):
    """Constructs a task object.

    Arguments:
      * targets: a string or list of strings defining the set of output files
        that the command will produce.

      * sources: a string or list of strings listing dependencies.  Filenames
        are relative to the root directory of the build unless they begin with
        '#/', in which case they are relative to the output directory.

        The list of sources must include the build tool itself if it is built
        from this source tree.

      * cmd: a string or list of strings defining how to produce "targets" from
        "sources".  If a string, the command will be likely be given to a shell
        and subject to shell interpretation (but warning, no environment
        variables will be set except what you explicitly set or propagate).  If
        an array, the arguments will be passed literally to execv() and will
        not be subject to interpretation.

    Keyword args:
      * stdin: a string specifying the stdin to supply to the process
      * guid: the guid for this task (if not specified one will be generated)

    """

    if isinstance(targets, basestring):
      targets = (targets)
    if isinstance(sources, basestring):
      sources = (sources)

    self.state = UNKNOWN
    self.targets = targets
    self.sources = sources
    self.cmd = cmd
    self.capture_stdout = False
    self.capture_stderr = False
    self.stdin = stdin
    self.env = {}

  @property
  def state(self):
    """The current state of the task.  The set of states are:

    UNKNOWN -- the current state is unknown (eg. the fs has not been scanned).
    UNSATISFIED -- task needs to run, but has unbuilt dependencies.
    RUNNABLE -- task needs to run, and all direct dependencies are ready.
    RUNNING -- task is currently running.
    DONE -- task is complete, outputs are up-to-date.
    ERROR -- there was an error running the task.

    A task is defined as "needing to run" when no version of the task's
    output(s) is currently available that was built with the current version of
    the inputs, command, and environment, and stdin (if any).
    """
    return self._state


class TaskResult:
  """Keeps information about the result of a task run.  This includes the
  process's success or failure, its return status, stdout, stderr, a copy of
  the task as it existed when the command finished, and metrics about its
  running time and resource utilization."""

  @property
  def stdout(self):
    pass


class TaskForce:
  """Object that defines the set of tasks and monitors state changes.

  A TaskForce contains a set of tasks.  It is required that all state changes
  to the task are made through this class; this allows for a centralized
  place where interested clients can subscribe to such state changes."""

  def __init__(self):
    self.tasks = {}

  def add(self, task):
    """Adds a task to the TaskForce.

    Constraints:
      * a target may not be built by more than one task.
      * there may not be cycles in the graph.
      * cmd must be reproduceable, may not read any source files it does not
        declare as sources, and must always produce the correct target(s).

    """

    for target in task.targets:
      if target in self.tasks: raise "Multiple tasks build target %s" % (target)
      self.tasks[target] = task

  def task(self, targets, sources, cmd):
    self.add(Task(targets, sources, cmd))

  def update(self, task, result = None):
    """Call to indicate that the task has changed.

    This must be called every time a task is updated in any way whatsoever.
    Will notify any interested subscribers about the change.  Transitions
    to DONE or ERROR must include a TaskResult as the "result" parameter."""


# All the rest of the classes in this file are 


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
