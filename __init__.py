"""Core TaskForce classes.

The following classes define the core TaskForce data model.  They are used by
all builds and are independent of any build/configuration strategy.
Specifically these classes make no assumptions about:

 * how tasks are created/defined
 * how inputs/output files are stored
 * how to compute the hash of an input file
 * how to receive notifications that a file is changed
 * how commands are invoked and how their results are collected
 * where the output directory lives in relation to the source directory
"""

class FilePath:
  """Object representing a file path.

  This is a filename relative to the base of a tree.  It can be relative to
  the base of the source tree, the build tree, or the output tree.
  """

  def __init__(self, path)
    """Initializes the object with the given path.

    Paths can be in one of the following forms:
      * foo/bar/baz (relative to the source tree)
      * #/foo/bar/baz (relative to the build tree)
      * @/foo/bar/baz (relative to the output tree)

    Paths may not contain "../" (which is disallowed) or "./" (which is
    unnecessary and complicates things).
    """

class Task:
  """Object that represents a single task that produces output file(s).

  A Task object is immutable once it is created, with the one exception of
  the "state" member (and a "runner" that can be associated with it while
  it is in the "running" state)."""

  def __init__(self, targets, sources, cmd, args, env, **kwargs):
    """Constructs a task object.

    Arguments:
      * targets: a FilePath or list of FilePaths defining the set of output
        files that the command will produce.

      * sources: a FilePath or list of FilePaths declaring dependencies.

      * cmd: the path to the runnable program that will be executed.

      * args: a list of strings that will be passed to cmd as arguments.
        These will be passed directly to spawnve and will not be subject
        to any shell interpretation.

    Keyword args:
      * stdin: a string specifying the stdin to supply to the process
      * (add capture_stderr and capture_stdout as options if the efficiency of
        always capturing them is an issue)

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

  def set_state(self, old_state, new_state, runner = None):
    """Updates the task to a new state.

    Raises an exception if the task is not currently in "old_state" (this
    indicates a race).  This is is a lightweight check, but is susceptible to
    the ABA problem; for truly robust synchronization the client should ensure
    that any attempts to modify a particular task's state are synchronized.
    One simple approach is to ensure that only one component is eligible to
    modify tasks in a given state.  For example, you could say that only a
    single Scheduler instance is allowed to modify tasks that are in the
    RUNNABLE state."""

  @property
  def target_id(self, srcfile_hashes, target_path):
    """Returns a hash identifying the output of this build for the given target.

    The hash provides the following guarantee: if two targets have the same id,
    they are guaranteed to produce the same output for the given target.
    Other target ids may produce the same output also, but we cannot know that
    without actually running the command.  If two targets have the same id, we
    can know they have the same output *without* running the command.

    This guarantee is only as good as the task definition.  If the task fails
    to specify some of its effective inputs, then the hash's guarantee may be
    false."""


class TaskResult:
  """Keeps information about the result of a task run.

  The information contains:
    * the process return status
    * whether this status indicates success or failure
    * for each target, the target_id and (for successful runs) its file hash.
    * strings of stdout/stderr
    * a copy of the task as it existed when the command finished
    * metrics about its running time and resource utilization."""

  @property
  def stdout(self):
    pass


class TaskGraph:
  """Object that represents the graph of live Task objects.

  A TaskGraph contains a set of tasks and allows clients to subscribe to
  state changes of them."""

  def __init__(self):
    self.tasks = {}

  def add(self, task):
    """Adds a task to the TaskForce.

    Constraints (we can validate these):
      * the task may not belong to any other TaskGraph.
      * a target may not be built by more than one task.
      * a given filename may not exist in more than one tree (src, build, out)
      * there may not be cycles in the graph.

    """

    for target in task.targets:
      if target in self.tasks: raise "Multiple tasks build target %s" % (target)
      self.tasks[target] = task

  def remove(self, task):
    pass
