
#ifndef __TASKFORCE_H__
#define __TASKFORCE_H__

#include <memory>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>

namespace taskforce {

class Job;
class StrArr;
class Task;
class TaskForceProto;

class FileInfo {
  // Filename.  Filenames are relative to the base of the build tree unless
  // they begin with "/".
  std::string filename() const { return filename_; }

  // Pointer to the task that produces this file.
  const Task* task() const { return task_; }

  // Whether this is a source file.  A source file is an input to some task
  // but is not an output of any other task.  It must exist before the build
  // begins.
  bool is_source() const { return is_source_; }

  // Whether the file exists (logically speaking; this class knows nothing
  // about the actual filesystem).  For source files, this is true if
  // TaskForce::SetSourceFingerprint() has been called for this file.  For
  // other files (artifacts), returns true if a job producing the file has
  // completed successfully and the source files have not changed in the
  // meantime.
  bool exists() const { return exists_; }

  // The fingerprint for this file.
  // Requires that: exists() == true.
  std::string fingerprint() const { return fingerprint_; }

  // Pointer to the job that produced this file.
  // Requires that: exists() && !is_source()
  const Job* job() const { return job_; }

 private:
  friend class TaskForce;
  FileInfo();

  std::string filename_;
  const Task* task_;
  const Job* job_;
  std::string fingerprint_;
  bool is_source_;
  bool exists_;
};

// The TaskForce object maintains the graph of all tasks, the expected state
// of the filesystem, and a runqueue of jobs.  It does not perform any I/O
// or spawn any processes itself.
class TaskForce {
 public:
  // Adds the given tasks to the TaskForce.
  // TODO: figure out semantics of adding tasks to an existing graph.
  // TODO: should the graph-like object be a separate object?
  //
  // If there was an error in the input tasks, returns false and writes an
  // error message.
  bool AddTasks(std::vector<std::unique_ptr<Task>> tasks, std::string* error);

  // Sets the targets of the current build.  These are the files we will
  // generate jobs to build.  Returns false if any of the given targets
  // are not actually files produced by this TaskForce.
  //
  // If source_filenames is non-NULL, it will be filled with all filenames
  // that are sources of these targets.
  bool SetTargets(std::vector<std::string> target_filenames);

  // Notifies the TaskForce of the fingerprint of the given source filename.
  // This will be a no-op if the given filename is not an input of any task.
  // Returns "false" if this filename is an output of some task (and therefore
  // cannot be considered a target).
  //
  // TODO: provide a way to get filenames and running jobs that are obsoleted
  // by this.
  bool SetSourceFingerprint(std::string filename, std::string fingerprint);

  // TODO: iterators for files, tasks, jobs.

  // Gets the overall status of the build.
  enum BuildStatus {
    // All requested targets have been successfully built.  This is also the
    // case if no targets have been set with SetTargets().
    STATUS_FINISHED = 0,

    // The build is unsatisfiable because necessary source files do not exist.
    // There may be jobs that can be run, but the requested targets cannot be
    // satisfied.
    STATUS_UNSATISFIABLE = 1,

    // There are some outstanding jobs on the run queue or running that need
    // to finish before the build is complete.
    STATUS_BUILDING = 1,

    // At least one job has failed to run, but other jobs are still running or
    // can be run.
    STATUS_ERROR = 2,

    // At least one job failed to run and no further jobs can be run.
    STATUS_FAILURE = 3,
  };

  BuildStatus GetBuildStatus() const;

  // Returns the number of runnable tasks.
  int NumRunnable() const;

  // Gets the next eligible task and removes it from the run queue.
  // The client should call UpdateJob for this job id until the job is in
  // state ERROR or SUCCESS.
  // Requires that NumRunnable() > 0.
  const Job* GetFromRunQueue();

  // Updates a job.  This job id should already exist in this TaskForce in
  // the RUNNABLE or RUNNING state.  If this is an illegal state change for
  // this job, or this job id is unknown, returns false.
  bool UpdateJob(const Job* job);


 private:
  // Proto representing this TaskForce.
  std::unique_ptr<TaskForceProto> proto_;

  // The base environment that is applied to every task's run.
  std::unique_ptr<StrArr> base_env_;

  // Maps task_id -> Task*.
  std::unordered_map<std::string, const Task*> tasks_;

  // Maps each filename to the task that produces it.
  std::map<std::string, FileInfo> files_;

  // An ordering of the tasks that will always build dependencies before they
  // are used.
  std::vector<const Task*> topo_order_;

  //std::queue<taskforce::Job*> runqueue_;
  //std::set<taskforce::Job*> running_;
  //std::map<std::string, taskforce::Job*> jobs_;
};

class TaskRunner {
 public:
  // Starts running the given task.  The Task must outlive the JobRunner.
  void SpawnTask(const Task* task);

  // Returns the number of jobs that are currently running.
  int NumActive() const;

  // Blocks until a running job has finished, and then returns the job status.
  // Requires that NumActive() > 0.
  // If the given timeout is greater than zero and this many milliseconds elapse
  // before any job has finished, NULL is returned.
  std::unique_ptr<TaskResults> WaitForResults(int timeout_ms);

 private:
};

class 

}

#endif
