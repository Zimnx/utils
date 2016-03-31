#pragma once

/*
 * Copyright (C) 2016 Maciej Zimnoch
 */

#include <Barrier.hpp>

template<typename ResultType>
class ResultBarrier : public Barrier {

  public:
    explicit ResultBarrier(uint32_t count = 1)
      : Barrier(count),
        m_success(false)
    {}

    void setResult(const ResultType& result, bool success = true) {
      m_result = result;
      m_success = success;
      done();
    }

    bool getResult(ResultType& result) {
      if (m_success) {
        result = m_result;
      }

      return m_success;
    }

  private:

    bool m_success;
    ResultType m_result;
};
