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
  static inline void call(const Task& task, const Callback& callback) {
    callback(task());
  }
};

template <typename Task, typename Callback>
struct TaskCallStrategy<Task, Callback, void> {
  static inline void call(const Task& task, const Callback& callback) {
    task();
    callback();
  }
};

template<typename T>
struct CallbackType {
  using type = std::function<void(T)>;
};

template<>
struct CallbackType<void> {
  using type = std::function<void()>;
};

template<typename T>
using CallbackType_t = typename CallbackType<T>::type;

} // namespace detail

template<class>
class AsyncTaskExecutor;

template<class R, class... Args >
class AsyncTaskExecutor<R(Args...)> {

    using ResultType = R;
    using Callback = detail::CallbackType_t<R>;
    using Task = std::function<R(Args...)>;
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

    void scheduleTask(Task task, Callback callback) {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_queue.emplace_back(std::move(task), std::move(callback));
      lock.unlock();
      m_cond.notify_one();
    }

  private:

    void worker() {
      for(;;) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (this->m_queue.empty()) {
          m_cond.wait(lock, [=]{ return m_ending || !m_queue.empty(); });
        }

        if (m_ending) {
          break;
        }

        TaskWithCallback taskWithCallback = m_queue.front();
        m_queue.pop_front();
        lock.unlock();

        const Task& task = taskWithCallback.first;
        const Callback& callback = taskWithCallback.second;

        detail::TaskCallStrategy<Task, Callback, ResultType>::call(task, callback);
      }
    }

    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    bool m_ending;

    std::deque<TaskWithCallback> m_queue;
};
