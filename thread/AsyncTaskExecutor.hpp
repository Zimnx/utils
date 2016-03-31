#pragma once

/*
 * Copyright (C) 2016 Maciej Zimnoch
 */

#include <mutex>
#include <deque>
#include <thread>
#include <condition_variable>

namespace detail {

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
class AsyncTaskExecutor {

    using ResultType = typename Task::result_type;
    using TaskWithCallback = std::pair<Task, Callback>;

  public:

    AsyncTaskExecutor()
      : m_ending{false}
    {
      m_thread = std::thread { &AsyncTaskExecutor::worker, this };
    }

    virtual ~AsyncTaskExecutor() {
      m_ending = true;
      m_cond.notify_one();
      m_thread.join();
    }

    void scheduleTask(Task&& task, Callback&& callback) {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_queue.emplace_back(task, callback);
      lock.unlock();
      m_cond.notify_one();
    }

  private:

    void worker() {
      for(;;) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_queue.empty()) {
          m_cond.wait(lock, [=]{ return m_ending || !m_queue.empty(); });
        }

        if (m_ending) {
          break;
        }

        const TaskWithCallback& taskWithCallback = m_queue.front();
        m_queue.pop_front();
        lock.unlock();

        Task task = taskWithCallback.first;
        Callback callback = taskWithCallback.second;

        detail::TaskCallStrategy<Task, Callback, ResultType>::call(std::move(task), std::move(callback));
      }
    }

    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    bool m_ending;

    std::deque<TaskWithCallback> m_queue;
};

using SimpleAsyncTaskExecutor = AsyncTaskExecutor<std::function<void()>, std::function<void()>>;
