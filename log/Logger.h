/*
 * Copyrights (c) Maciej Zimnoch 2016
 */

#pragma once
#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>

class Logger {
  public:

    enum class LogLevel {
      Debug = 39,
      Info = 34,
      Error = 35
    };

    explicit Logger() {}
    explicit Logger(const std::string& logFile)
      : m_logFile(logFile)
    {}

    template <typename... Args>
    void log(LogLevel logLevel, std::string format, Args... args) const {
      auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

      std::stringstream ss;
      ss << std::put_time(std::localtime(&now), "%T");

      std::string logMessage = formatString("%s - " + format, ss.str().c_str(), args...);

      if (!m_logFile.empty()) {
        auto file = std::ofstream(m_logFile.c_str(), std::ios::out | std::ios::app);
        if (file.good()) {
          file << logMessage << std::endl;
        } else {
          std::cout << "Unable to open file = " << m_logFile << std::endl;
        }
      }

      std::cout << "\x1B[" << static_cast<uint32_t>(logLevel) << "m" << logMessage << "\x1B[0m" << std::endl;
    }

private:
    template<typename... Args>
    std::string formatString(const std::string& format, Args... args) const
    {
        size_t size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
        if (size > 8192) {
          size = 8192;
        }
        auto bufer = std::make_unique<char[]>(size);
        std::snprintf(bufer.get(), size, format.c_str(), args...);
        return std::string(bufer.get(), bufer.get() + size - 1);
    }

  private:
    std::string m_logFile;
};

#define log_debug(logger, message, ...) (logger.log(Logger::LogLevel::Debug, message ,##__VA_ARGS__))
#define log_info(logger, message, ...) (logger.log(Logger::LogLevel::Info, message, ##__VA_ARGS__))
#define log_error(logger, message, ...) (logger.log(Logger::LogLevel::Error, message, ##__VA_ARGS__))

