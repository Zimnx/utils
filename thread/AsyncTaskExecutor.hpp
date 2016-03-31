#pragma once

/*
 * Copyright (C) 2016 Maciej Zimnoch
 */

#include <mutex>
#include <deque>
#include <thread>
#include <condition_variable>

namespace detail {

template<class Derived, class ContainerValueType>
class AsyncTaskExecutorBase {
  public:

    AsyncTaskExecutorBase()
      : m_ending{false}
    {
      m_thread = std::thread { &AsyncTaskExecutorBase::worker, this };
    }

    virtual ~AsyncTaskExecutorBase() {
      m_ending = true;
      m_cond.notify_one();
      m_thread.join();
    }

    void worker() {
      static_cast<Derived*>(this)->worker();
    }

    using Queue = std::deque<ContainerValueType>;

    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    bool m_ending;

    Queue m_queue;
};

template <typename Task, typename Callback, typename ResultType>
struct TaskCallStrategy {
    static void call(Task&& task, Callback&& callback) {
      callback(task());
    }
};

template <typename Task, typename Callback>
struct TaskCallStrategy<Task, Callback, void> {
    static void call(Task&& task, Callback&& callback) {
      task();
      callback();
    }
};

}

template<typename Task, typename Callback = std::function<void(typename Task::result_type)>>
class AsyncTaskExecutor : public detail::AsyncTaskExecutorBase<AsyncTaskExecutor<Task, Callback>, std::pair<Task, Callback>> {

    using ResultType = typename Task::result_type;
    using TaskWithCallback = std::pair<Task, Callback>;

  public:

    void scheduleTask(Task&& task, Callback&& callback) {
      std::unique_lock<std::mutex> lock(this->m_mutex);
      this->m_queue.emplace_back(task, callback);
      lock.unlock();
      this->m_cond.notify_one();
    }

    void worker() {
      for(;;) {
        std::unique_lock<std::mutex> lock(this->m_mutex);

        if (this->m_queue.empty()) {
          this->m_cond.wait(lock, [=]{ return this->m_ending || !this->m_queue.empty(); });
        }

        if (this->m_ending) {
          break;
        }

        const TaskWithCallback& taskWithCallback = this->m_queue.front();
        this->m_queue.pop_front();
        lock.unlock();

        Task task = taskWithCallback.first;
        Callback callback = taskWithCallback.second;

        detail::TaskCallStrategy<Task, Callback, ResultType>::call(std::move(task), std::move(callback));
      }
    }
};

using SimpleAsyncTaskExecutor = AsyncTaskExecutor<std::function<void()>, std::function<void()>>;
