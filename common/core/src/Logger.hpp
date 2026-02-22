#ifndef FALCOR_LOGGER_HPP
#define FALCOR_LOGGER_HPP

#include "quill/Quill.h"
#include "quill/Logger.h"

#include <string>
#include <iostream>
#include <utility>
#include <string_view>
#include <mutex>

namespace Falcor {

// ============================================================
// Inicialização segura do Quill (roda só uma vez)
// ============================================================
inline void ensure_quill_started()
{
    static std::once_flag flag;
    std::call_once(flag, []() {
        quill::start();
    });
}

// ============================================================
// Base Logger
// ============================================================
class Logger {
public:
    explicit Logger(quill::Logger* p_logger)
        : m_logger(p_logger)
    {
        ensure_quill_started();
    }

    virtual ~Logger() = default;

    template <typename... Args>
    void log_info(std::string_view format_str, Args&&... args)
    {
        std::string raw_msg =
            fmtquill::v10::format(
                fmtquill::v10::runtime(format_str),
                std::forward<Args>(args)...
            );

        std::cout << decorate_info() << raw_msg << std::endl;

        if (m_logger) {
            QUILL_LOG_INFO(m_logger, "{}", raw_msg);
        }

        on_log_info(raw_msg);
    }

    template <typename... Args>
    void log_error(std::string_view format_str, Args&&... args)
    {
        std::string raw_msg =
            fmtquill::v10::format(
                fmtquill::v10::runtime(format_str),
                std::forward<Args>(args)...
            );

        std::cerr << decorate_error() << raw_msg << std::endl;

        if (m_logger) {
            QUILL_LOG_ERROR(m_logger, "{}", raw_msg);
        }

        on_log_error(raw_msg);
    }

protected:
    virtual std::string decorate_info() = 0;
    virtual std::string decorate_error() = 0;

    virtual void on_log_info(const std::string&) {}
    virtual void on_log_error(const std::string&) {}

private:
    quill::Logger* m_logger;
};

// ============================================================
// GENERAL LOGGER
// ============================================================
class GeneralLogger : public Logger {
public:
    explicit GeneralLogger(quill::Logger* logger)
        : Logger(logger) {}

protected:
    std::string decorate_info() override {
        return "\033[1;32m[General]\033[0m ";
    }

    std::string decorate_error() override {
        return "\033[1;31m[General]\033[0m ";
    }
};

// ============================================================
// SYSTEM LOGGER
// ============================================================
class SystemLogger : public Logger {
public:
    explicit SystemLogger(quill::Logger* logger)
        : Logger(logger) {}

protected:
    std::string decorate_info() override {
        return "\033[1;34m[System]\033[0m ";
    }

    std::string decorate_error() override {
        return "\033[1;35m[System Critical]\033[0m ";
    }
};

} // namespace Falcor

#endif // FALCOR_LOGGER_HPP