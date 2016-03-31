#pragma once

/*
 * Copyright (C) 2016 Maciej Zimnoch
 */

#include <cstdint>
#include <mutex>
#include <condition_variable>

class Barrier {

  public:
    explicit Barrier(uint32_t count = 1)
      : m_count(count),
        m_ready(false)
    {}

    void done() {
      std::unique_lock<std::mutex> lock(m_mutex);
      if (--m_count <= 0) {
        m_ready = true;
        lock.unlock();
        m_cond.notify_all();
      }
    }

    void wait() {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_cond.wait(lock, [&]{ return m_ready; });
    }

    void wait(uint32_t milliseconds) {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_cond.wait_for(lock, std::chrono::milliseconds(milliseconds), [&]{ return m_ready; });
    }

  private:
    uint32_t m_count;

    bool m_ready;
    std::condition_variable m_cond;
    std::mutex m_mutex;
};
