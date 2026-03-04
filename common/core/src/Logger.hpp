#pragma once

#ifndef FALCOR_LOGGER_HPP
#define FALCOR_LOGGER_HPP

#include "quill/Quill.h"
#include "quill/Logger.h"
#include <string>
#include <utility>
#include <string_view>
#include <mutex>
#include <source_location>
#include <memory> 

inline void ensure_quill_started() {
    static std::once_flag flag;
    std::call_once(flag, []() {
        quill::start();
    });
}

namespace Color {
    inline constexpr std::string_view Reset     = "\033[0m";
    inline constexpr std::string_view Bold      = "\033[1m";
    inline constexpr std::string_view Debug     = "\033[38;5;226m"; 
    inline constexpr std::string_view Info      = "\033[38;5;75m";   
    inline constexpr std::string_view Warn      = "\033[38;5;215m";  
    inline constexpr std::string_view Error     = "\033[38;5;196m";  
    inline constexpr std::string_view Critical  = "\033[38;5;197m";  
    inline constexpr std::string_view Gray      = "\033[38;5;243m";  // Cinza claro
    inline constexpr std::string_view DarkGray  = "\033[38;5;237m";  // Cinza bem apagado
    inline constexpr std::string_view Divider   = "\033[38;5;238m";  
}

struct LogFormat {
    std::string_view fmt;
    std::source_location loc;

    LogFormat(const char* s, std::source_location l = std::source_location::current())
        : fmt(s), loc(l) {}
        
    LogFormat(std::string_view s, std::source_location l = std::source_location::current())
        : fmt(s), loc(l) {}
};

class LoggerBase {
public:
    explicit LoggerBase(quill::Logger* p_logger, std::string name) 
        : m_logger(p_logger), m_loggerName(std::move(name)) {
        ensure_quill_started();
    }

    virtual ~LoggerBase() = default;

    template <typename... Args>
    void log_debug(LogFormat format, Args&&... args) {
        if (m_logger) {
            std::string msg = fmtquill::v10::format(fmtquill::v10::runtime(format.fmt), std::forward<Args>(args)...);
            QUILL_LOG_DEBUG(m_logger, "{}{} {} {}|{} {}{}{}", 
                format_tag(), decorate_debug(), format_loc(format.loc), Color::Divider, Color::Debug, Color::Bold, msg, Color::Reset);
        }
    }

    template <typename... Args>
    void log_info(LogFormat format, Args&&... args) {
        if (m_logger) {
            std::string msg = fmtquill::v10::format(fmtquill::v10::runtime(format.fmt), std::forward<Args>(args)...);
            QUILL_LOG_INFO(m_logger, "{}{} {} {}|{} {}", 
                format_tag(), decorate_info(), format_loc(format.loc), Color::Divider, Color::Reset, msg);
        }
    }

    // ADICIONADO: Suporte para log_warning
    template <typename... Args>
    void log_warning(LogFormat format, Args&&... args) {
        if (m_logger) {
            std::string msg = fmtquill::v10::format(fmtquill::v10::runtime(format.fmt), std::forward<Args>(args)...);
            QUILL_LOG_WARNING(m_logger, "{}{} {} {}|{} {}{}{}", 
                format_tag(), decorate_warning(), format_loc(format.loc), Color::Divider, Color::Warn, Color::Bold, msg, Color::Reset);
        }
    }

    template <typename... Args>
    void log_error(LogFormat format, Args&&... args) {
        if (m_logger) {
            std::string msg = fmtquill::v10::format(fmtquill::v10::runtime(format.fmt), std::forward<Args>(args)...);
            QUILL_LOG_ERROR(m_logger, "{}{} {} {}|{} {}{}{}", 
                format_tag(), decorate_error(), format_loc(format.loc), Color::Divider, Color::Error, Color::Bold, msg, Color::Reset);
        }
    }

    template <typename... Args>
    void log_critical(LogFormat format, Args&&... args) {
        if (m_logger) {
            std::string msg = fmtquill::v10::format(fmtquill::v10::runtime(format.fmt), std::forward<Args>(args)...);
            QUILL_LOG_CRITICAL(m_logger, "{}{} {} {}|{} {}{}{}", 
                format_tag(), decorate_critical(), format_loc(format.loc), Color::Divider, Color::Critical, Color::Bold, msg, Color::Reset);
        }
    }

protected:
    virtual std::string decorate_debug() = 0;
    virtual std::string decorate_info() = 0;
    virtual std::string decorate_warning() = 0; // ADICIONADO
    virtual std::string decorate_error() = 0;
    virtual std::string decorate_critical() = 0;

    inline std::string format_tag() const {
        return fmtquill::v10::format("{}{:<14} {}", Color::Gray, m_loggerName, Color::Reset);
    }

    inline std::string format_loc(const std::source_location& loc) const {
        std::string_view file = loc.file_name();
        auto last_slash = file.find_last_of("\\/");
        if (last_slash != std::string_view::npos) file.remove_prefix(last_slash + 1);
        
        return fmtquill::v10::format("{}{:<12}:{:>3}{}", Color::Gray, file, loc.line(), Color::Reset);
    }

private:
    quill::Logger* m_logger;
    std::string m_loggerName;
};

class GeneralLogger : public LoggerBase {
public: 
    GeneralLogger(quill::Logger* p_logger, std::string name) : LoggerBase(p_logger, std::move(name)) {}
protected:
    std::string decorate_debug() override { return fmtquill::v10::format("{}[ DEBUG ]{}", Color::Debug, Color::Reset); }
    std::string decorate_info() override { return fmtquill::v10::format("{}[ INFO   ]{}", Color::Info, Color::Reset); }
    std::string decorate_warning() override { return fmtquill::v10::format("{}[ WARN   ]{}", Color::Warn, Color::Reset); }
    std::string decorate_error() override { return fmtquill::v10::format("{}[ ERROR ]{}", Color::Error, Color::Reset); }
    std::string decorate_critical() override { return fmtquill::v10::format("{}[ CRIT   ]{}", Color::Critical, Color::Reset); }
};

class SystemLogger : public LoggerBase {
public: 
    SystemLogger(quill::Logger* p_logger, std::string name) : LoggerBase(p_logger, std::move(name)) {}
protected:
    std::string decorate_debug() override { return fmtquill::v10::format("{}[ SYSDEBUG ]{}", Color::Debug, Color::Reset); }
    std::string decorate_info() override { return fmtquill::v10::format("{}[ SYSINFO ]{}", Color::Info, Color::Reset); }
    std::string decorate_warning() override { return fmtquill::v10::format("{}[ SYSWRN ]{}", Color::Warn, Color::Reset); }
    std::string decorate_error() override { return fmtquill::v10::format("{}[ SYSERR ]{}", Color::Error, Color::Reset); }
    std::string decorate_critical() override { return fmtquill::v10::format("{}[ SYSCRI ]{}", Color::Critical, Color::Reset); }
};

inline quill::Logger* create_clean_logger(std::string const& name)
{
    ensure_quill_started();
    auto handler = quill::stdout_handler();
    handler->set_pattern("%(ascii_time) %(message)", "%H:%M:%S", quill::Timezone::LocalTime);
    return quill::create_logger(name, std::move(handler));
}

namespace Falcor {

inline GeneralLogger Logger(create_clean_logger("GEN"), "GeneralLogger");
inline SystemLogger  SysLog(create_clean_logger("SYS"), "SystemLogger");

} // namespace Falcor

#endif