
#ifndef __TASKFORCE_H__
#define __TASKFORCE_H__

#include <map>
#include <queue>
#include <set>
#include <string>

namespace taskforce {
class Task;
class Job;
}

class TaskForce {
 public:
  // Initializes the graph of tasks.  Ownership of the tasks is taken.
  //
  // If there was an error in the input tasks, returns false and writes an
  // error message.
  bool Initialize(const std::vector<taskforce::Task*>& tasks,
                  std::string* error);

  // Attempts to build everything, returning true if this succeeded.
  bool Build();

 private:
  bool SpawnJob(taskforce::Job* job);

  struct FileInfo {
    std::string sha1;
    bool is_source;  // False if generated by a task.
  };

  std::map<std::string, taskforce::Task*> tasks_;
  std::map<std::string, FileInfo> files_;
  std::queue<taskforce::Job*> runqueue_;
  std::set<taskforce::Job*> running_;
  std::map<std::string, taskforce::Job*> jobs_;
};

#endif