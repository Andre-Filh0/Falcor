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

namespace Falcor {

/**
 * @brief Inicialização do backend.
 */
inline void ensure_quill_started() {
    static std::once_flag flag;
    std::call_once(flag, []() {
        quill::start();
    });
}

/**
 * @brief Cores ANSI para o terminal.
 */
namespace Color {
    inline constexpr std::string_view Reset     = "\033[0m";
    inline constexpr std::string_view Bold      = "\033[1m";
    inline constexpr std::string_view Debug     = "\033[38;5;226m";  // Amarelo Vivo
    inline constexpr std::string_view Info      = "\033[38;5;75m";   
    inline constexpr std::string_view Warn      = "\033[38;5;215m";  
    inline constexpr std::string_view Error     = "\033[38;5;196m";  
    inline constexpr std::string_view Critical  = "\033[38;5;197m";  // Vermelho Claro/Brilhante
    inline constexpr std::string_view Gray      = "\033[38;5;243m";  
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
    explicit LoggerBase(quill::Logger* p_logger) : m_logger(p_logger) {
        ensure_quill_started();
    }

    virtual ~LoggerBase() = default;

    template <typename... Args>
    void log_debug(LogFormat format, Args&&... args) {
        if (m_logger) {
            std::string msg = fmtquill::v10::format(fmtquill::v10::runtime(format.fmt), std::forward<Args>(args)...);
            QUILL_LOG_DEBUG(m_logger, "{} {} {}|{} {}{}{}", 
                decorate_debug(), format_loc(format.loc), Color::Divider, Color::Debug, Color::Bold, msg, Color::Reset);
        }
    }

    template <typename... Args>
    void log_info(LogFormat format, Args&&... args) {
        if (m_logger) {
            std::string msg = fmtquill::v10::format(fmtquill::v10::runtime(format.fmt), std::forward<Args>(args)...);
            QUILL_LOG_INFO(m_logger, "{} {} {}|{} {}", 
                decorate_info(), format_loc(format.loc), Color::Divider, Color::Reset, msg);
        }
    }

    template <typename... Args>
    void log_error(LogFormat format, Args&&... args) {
        if (m_logger) {
            std::string msg = fmtquill::v10::format(fmtquill::v10::runtime(format.fmt), std::forward<Args>(args)...);
            QUILL_LOG_ERROR(m_logger, "{} {} {}|{} {}{}{}", 
                decorate_error(), format_loc(format.loc), Color::Divider, Color::Error, Color::Bold, msg, Color::Reset);
        }
    }

    template <typename... Args>
    void log_critical(LogFormat format, Args&&... args) {
        if (m_logger) {
            std::string msg = fmtquill::v10::format(fmtquill::v10::runtime(format.fmt), std::forward<Args>(args)...);
            QUILL_LOG_CRITICAL(m_logger, "{} {} {}|{} {}{}{}", 
                decorate_critical(), format_loc(format.loc), Color::Divider, Color::Critical, Color::Bold, msg, Color::Reset);
        }
    }

protected:
    virtual std::string decorate_debug() = 0;
    virtual std::string decorate_info() = 0;
    virtual std::string decorate_error() = 0;
    virtual std::string decorate_critical() = 0;

    inline std::string format_loc(const std::source_location& loc) const {
        std::string_view file = loc.file_name();
        auto last_slash = file.find_last_of("\\/");
        if (last_slash != std::string_view::npos) file.remove_prefix(last_slash + 1);
        
        return fmtquill::v10::format("{}{:<12}:{:>3}{}", Color::Gray, file, loc.line(), Color::Reset);
    }

private:
    quill::Logger* m_logger;
};

class GeneralLogger : public LoggerBase {
public: 
    using LoggerBase::LoggerBase;
protected:
    std::string decorate_debug() override { 
        return fmtquill::v10::format("{}[ DEBUG ]{}", Color::Debug, Color::Reset); 
    }
    std::string decorate_info() override { 
        return fmtquill::v10::format("{}[ INFO  ]{}", Color::Info, Color::Reset); 
    }
    // Mantive o nome interno como ERROR para bater com a gravidade do Quill, mas você pode mudar o texto para WARN se preferir
    std::string decorate_error() override { 
        return fmtquill::v10::format("{}[ ERROR ]{}", Color::Error, Color::Reset); 
    }
    std::string decorate_critical() override { 
        return fmtquill::v10::format("{}[ CRIT  ]{}", Color::Critical, Color::Reset); 
    }
};

inline quill::Logger* create_clean_logger(std::string const& name)
{
    ensure_quill_started();
    auto handler = quill::stdout_handler();
    handler->set_pattern(
        "%(ascii_time) %(message)",
        "%H:%M:%S",
        quill::Timezone::LocalTime
    );
    return quill::create_logger(name, std::move(handler));
}

inline Falcor::GeneralLogger Logger(create_clean_logger("GENERAL"));

} // namespace Falcor

#endif