#pragma once

/*
 * Copyright (C) 2016 Maciej Zimnoch
 */

#include <chrono>

class Timer {

  public:
    Timer()
      : m_start(std::chrono::high_resolution_clock::now())
    {}

    template<typename T>
    T get() const {
      return std::chrono::duration_cast<T>(std::chrono::high_resolution_clock::now() - m_start);
    }

    friend std::ostream& operator<<(std::ostream& stream, const Timer& timer) {
      stream << timer.get<std::chrono::milliseconds>().count() << "ms";
      return stream;
    }

  private:
    std::chrono::high_resolution_clock::time_point m_start;

};
