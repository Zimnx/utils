#pragma once

/*
 * Copyright (C) 2016 Maciej Zimnoch
 */

#include <mutex>
#include <deque>
#include <thread>
#include <condition_variable>

namespace detail {

template <typename ResultType>
struct TaskCallStrategy {
  template<class Callback, class Task, class ArgsTuple, std::size_t... Is>
  static inline void call(const Callback& callback, const Task& task, const ArgsTuple& tuple, std::index_sequence<Is...> is) {
    ResultType result = task(std::get<Is>(tuple)...);
    if (callback) {
      callback(std::move(result));
    }
  }
};

template <>
struct TaskCallStrategy<void> {
  template<class Callback, class Task, class ArgsTuple, std::size_t... Is>
  static inline void call(const Callback& callback, const Task& task, const ArgsTuple& tuple, std::index_sequence<Is...> is) {
    task(std::get<Is>(tuple)...);
    if (callback) {
      callback();
    }
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
using CallbackType_T = typename CallbackType<T>::type;

} // namespace detail

template<class>
class AsyncTaskExecutor;

template<class R, class... Args >
class AsyncTaskExecutor<R(Args...)> {

    using ResultType = R;
    using Callback = detail::CallbackType_T<R>;
    using Task = std::function<R(Args...)>;

    struct CallParameters {
        CallParameters(Callback&& callback, Task&& task, std::tuple<Args...>&& args)
          : callback(std::forward<Callback>(callback))
          , task(std::forward<Task>(task))
          , args(std::forward<std::tuple<Args...>>(args))
        {}
        Callback callback;
        Task task;
        std::tuple<Args...> args;
    };

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

    void schedule(Callback&& callback, Task&& task, Args&&... args) {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_queue.emplace_back(std::forward<Callback>(callback),
                           std::forward<Task>(task),
                           std::forward_as_tuple(args...));
      lock.unlock();
      m_cond.notify_one();
    }

    void schedule(Task&& task, Args&&... args) {
      schedule(Callback(), std::forward<Task>(task), std::forward<Args>(args)...);
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

        CallParameters callParams = m_queue.front();
        m_queue.pop_front();
        lock.unlock();

        detail::TaskCallStrategy<ResultType>::call(callParams.callback,
                                                   callParams.task,
                                                   callParams.args,
                                                   std::make_index_sequence<sizeof...(Args)>());
      }
    }

    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    bool m_ending;

    std::deque<CallParameters> m_queue;
};
