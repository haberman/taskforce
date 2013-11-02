
#include "taskforce.h"

#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include "taskforce.pb.h"
#include "sha1.h"

//using namespace std;
using google::protobuf::RepeatedPtrField;

namespace taskforce {

namespace {

const int kNumMicrosPerSecond = 1000000;

int64_t NowUsec() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * kNumMicrosPerSecond) + tv.tv_usec;
}

template <class T, class Key>
bool TryInsert(T* container, Key&& val,
               typename T::iterator* iter) {
  auto success = container->insert(std::forward<Key>(val));
  if (success.second) {
    *iter = success.first;
    return true;
  } else {
    return false;
  }
}

template <class T>
const typename T::mapped_type& FindOrDie(const T& container,
                                         const typename T::key_type& key) {
  typename T::const_iterator iter = container.find(key);
  assert(iter != container.end());
  return iter->second;
}

class TopoSort {
 public:
  TopoSort() {
  }

  bool Sort(const RepeatedPtrField<Task>& tasks,
            const std::unordered_map<std::string, const Task*>* task_producing,
            std::vector<const Task*>* sorted, std::string* error) {
    error_ = error;
    sorted->clear();
    sorted_ = sorted;
    unseen_.clear();
    visiting_.clear();
    stack_.clear();
    task_producing_ = task_producing;

    for (const auto& task : tasks) {
      unseen_.insert(&task);
    }

    for (TaskSet::iterator it = unseen_.begin(); it != unseen_.end(); ) {
      assert(stack_.size() == 0);
      stack_.push_back(*it);
      if (!Visit(*it)) {
        return false;
      }
      stack_.pop_back();
      unseen_.erase(it++);
    }

    return true;
  }

  bool Visit(const Task* task) {
    TaskSet::iterator iter;

    if (!TryInsert(&visiting_, task, &iter)) {
      *error_ = "Dependency cycle: [" + task->label();
      while (stack_.back() != task) {
        *error_ += ", " + stack_.back()->label();
        stack_.pop_back();
      }
      *error_ += "]";
      return false;
    }

    stack_.push_back(task);
    if (stack_[0] != task) {
      unseen_.erase(task);
    }

    for (const auto& src : task->source()) {
      Visit(FindOrDie(*task_producing_, src));
    }

    stack_.pop_back();
    visiting_.erase(iter);
    sorted_->push_back(task);
    return true;
  }

 private:
  typedef std::unordered_set<const Task*> TaskSet;
  TaskSet unseen_;
  TaskSet visiting_;

  // Solely for the purposes of error reporting, so we can print the cycle.
  std::vector<const Task*> stack_;

  std::vector<const Task*>* sorted_;
  std::string* error_;

  const std::unordered_map<std::string, const Task*>* task_producing_;
};

}  // anoymous namespace

// For constructing arrays of strings in the slightly peculiar format
// required by execve().
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
    // unique_ptr frees the array of pointers but not the pointed-to strings.
    for (int i = 0; i < size_; i++) {
      free(array_[i]);
    }
  }

  void AppendFrom(const StrArr& other) {
    size_t new_size = size_ + other.size_;
    std::unique_ptr<char*[]> new_array(new char*[new_size + 1]);
    std::copy(&array_[0], &array_[size_], &new_array[0]);
    array_.swap(new_array);

    for (int i = 0; i < other.size_; i++) {
      array_[size_ + i] = strdup(other.array_[i]);
    }
    std::swap(size_, new_size);
    array_[size_] = NULL;
  }

  char **get() { return array_.get(); }

 private:
  size_t size_;
  // Can't use vector<char*> because execve() takes ptr to non-const array.
  std::unique_ptr<char*[]> array_;
};

bool TaskForce::Initialize(std::unique_ptr<TaskForceProto> taskforce,
                           std::string* error) {
  proto_ = std::move(taskforce);
  base_env_.reset(new StrArr(proto_->base_env()));

  // Initialize tasks_: tasks indexed by their SHA1-derived ID.
  for (const auto& task : proto_->task()) {
    std::string serialized;
    task.SerializeToString(&serialized);
    const Task*& entry = tasks_[GetSHA1(serialized)];
    if (entry) {
      tasks_.clear();
      *error = "Duplicate task: " + serialized;
    }
    entry = &task;

    // Initialize task_producing_.
    //
    // TODO: normalize the pathnames somehow?
    for (const auto& target : task.target()) {
      task_producing_[target] = &task;
    }
  }

  // Initialize topo_order_: a topographical order of Tasks that respects their
  // declared dependencies.
  //
  // There are various things we could do to be more clever about the ordering.
  if (!TopoSort().Sort(proto_->task(), &task_producing_, &topo_order_, error)) {
    return false;
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
    const Task* task = tasks_[job->task_id()];
    assert(task);

    StrArr arg(task->arg());
    StrArr env(task->env());
    env.AppendFrom(*base_env_.get());
    execve(task->arg(0).c_str(), arg.get(), env.get());

    // If we got here it's failure.
    std::cerr << "Error in exec(): " << strerror(errno);
    exit(1);
  }
}

bool TaskForce::Build() {
  return true;
}

}  // namespace taskforce
