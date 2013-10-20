
#include "taskforce.h"

#include <errno.h>
#include <unistd.h>
#include <memory>
#include <iostream>
#include <sys/time.h>
#include "taskforce.pb.h"
#include "sha1.h"

using namespace taskforce;
//using namespace std;
using google::protobuf::RepeatedPtrField;

const int kNumMicrosPerSecond = 1000000;

static int64_t NowUsec() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * kNumMicrosPerSecond) + tv.tv_usec;
}

class StrArr {
 public:
  StrArr(const RepeatedPtrField<std::string>& field)
      : size_(field.size()),
        array_(new char*[size_ + 1]) {
    array_[size_] = NULL;
    for (int i = 0; i < size_; i++) {
      array_[i] = strdup(field.Get(i).c_str());
    }
  }

  ~StrArr() {
    for (int i = 0; i < size_; i++) {
      free(array_[i]);
    }
  }

  char **get() { return array_.get(); }

 private:
  size_t size_;
  std::unique_ptr<char*[]> array_;
};

bool TaskForce::Initialize(const std::vector<Task*>& tasks,
                           std::string* error) {
  for (const auto& task : tasks) {
    task->clear_id();
    std::string serialized;
    task->SerializeToString(&serialized);
    task->set_id(GetSHA1(serialized));

    Task*& entry = tasks_[task->id()];
    if (entry) {
      tasks_.clear();
      *error = "Duplicate task: " + serialized;
    }
    entry = task;
  }
  return true;
}

bool TaskForce::SpawnJob(Job* job) {
  job->set_started_usec(NowUsec());
  pid_t pid = fork();
  if (pid == -1) {
    job->set_stderr(strerror(errno));
    return false;
  } else if (pid) {
    // Parent.
    job->set_pid(pid);
    return true;
  } else {
    // Child.  Setup argv and environment.
    Task* task = tasks_[job->task_id()];
    assert(task);

    StrArr arg(task->arg());
    StrArr env(task->env());
    execve(task->arg(0).c_str(), arg.get(), env.get());

    // If we got here it's failure.
    std::cerr << "Error in exec(): " << strerror(errno);
    exit(1);
  }
}

bool TaskForce::Build() {
  return true;
}
