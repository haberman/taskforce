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

