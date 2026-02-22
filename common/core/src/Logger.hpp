#pragma once

#ifndef FALCOR_LOGGER_HPP
#define FALCOR_LOGGER_HPP

#include "quill/Quill.h"
#include "quill/Logger.h"

#include <string>
#include <iostream>
#include <utility>
#include <string_view>
#include <mutex>
#include <chrono>
#include <ctime>
#include <source_location>

namespace Falcor {

inline void ensure_quill_started() {
    static std::once_flag flag;
    std::call_once(flag, []() { quill::start(); });
}

namespace Color {
    inline constexpr std::string_view Reset      = "\033[0m";
    inline constexpr std::string_view Dim        = "\033[90m";   
    inline constexpr std::string_view Bold       = "\033[1m";
    inline constexpr std::string_view Blue       = "\033[34m";   
    inline constexpr std::string_view Cyan       = "\033[36m";   
    inline constexpr std::string_view Red        = "\033[91m";   
    inline constexpr std::string_view Magenta    = "\033[35m";   
    inline constexpr std::string_view File       = "\033[38;5;242m"; 
}

/**
 * @brief Estrutura para capturar a string de formato E a localização.
 * Isso resolve o conflito de templates com argumentos variádicos.
 */
struct LogFormat {
    std::string_view fmt;
    std::source_location loc;

    // Construtor implícito para aceitar strings e capturar a origem
    LogFormat(const char* s, std::source_location l = std::source_location::current())
        : fmt(s), loc(l) {}
        
    LogFormat(std::string_view s, std::source_location l = std::source_location::current())
        : fmt(s), loc(l) {}
};

class Logger {
public:
    explicit Logger(quill::Logger* p_logger) : m_logger(p_logger) {
        ensure_quill_started();
    }

    virtual ~Logger() = default;

    template <typename... Args>
    void log_info(LogFormat format, Args&&... args) {
        std::string raw_msg = fmtquill::v10::format(fmtquill::v10::runtime(format.fmt), std::forward<Args>(args)...);
        
        std::cout << decorate_info() << format_loc(format.loc) << " " << raw_msg << Color::Reset << std::endl;

        if (m_logger) QUILL_LOG_INFO(m_logger, "[{}:{}] {}", format.loc.file_name(), format.loc.line(), raw_msg);
    }

    template <typename... Args>
    void log_error(LogFormat format, Args&&... args) {
        std::string raw_msg = fmtquill::v10::format(fmtquill::v10::runtime(format.fmt), std::forward<Args>(args)...);

        std::cerr << decorate_error() << format_loc(format.loc) << " " << Color::Bold << raw_msg << Color::Reset << std::endl;

        if (m_logger) QUILL_LOG_ERROR(m_logger, "[{}:{}] {}", format.loc.file_name(), format.loc.line(), raw_msg);
    }

protected:
    virtual std::string decorate_info() = 0;
    virtual std::string decorate_error() = 0;

    inline std::string format_loc(const std::source_location& loc) const {
        std::string_view file = loc.file_name();
        auto last_slash = file.find_last_of("\\/");
        if (last_slash != std::string_view::npos) file.remove_prefix(last_slash + 1);
        
        return fmtquill::v10::format(" {}{}:{}{}", Color::File, file, loc.line(), Color::Reset);
    }

    inline std::string get_timestamp() const {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_struct;
        localtime_r(&in_time_t, &tm_struct);

        return fmtquill::v10::format("{}[{:02d}/{:02d}/{} {:02d}:{:02d}:{:02d}]{}", 
            Color::Dim, tm_struct.tm_mday, tm_struct.tm_mon + 1, tm_struct.tm_year + 1900,
            tm_struct.tm_hour, tm_struct.tm_min, tm_struct.tm_sec, Color::Reset);
    }

private:
    quill::Logger* m_logger;
};

class GeneralLogger : public Logger {
public: using Logger::Logger;
protected:
    std::string decorate_info() override { 
        return fmtquill::v10::format("{} {}{}[INFO]{}", get_timestamp(), Color::Bold, Color::Cyan, Color::Reset); 
    }
    std::string decorate_error() override { 
        return fmtquill::v10::format("{} {}{}[WARN]{}", get_timestamp(), Color::Bold, Color::Red, Color::Reset); 
    }
};

class SystemLogger : public Logger {
public: using Logger::Logger;
protected:
    std::string decorate_info() override { 
        return fmtquill::v10::format("{} {}{}[SYS ]{}", get_timestamp(), Color::Bold, Color::Blue, Color::Reset); 
    }
    std::string decorate_error() override { 
        return fmtquill::v10::format("{} {}{}[CRIT]{}", get_timestamp(), Color::Bold, Color::Magenta, Color::Reset); 
    }
};

} // namespace Falcor

#endif