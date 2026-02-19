#pragma once

#include "quill/Quill.h"
#include "quill/Logger.h"
#include "quill/LogLevel.h"

#include <string_view>

enum  class LogTypes {
    SYSTEM_LOG,
    GENERAL_LOG
}

static inline const quill::Logger* quill::get_logger(LogTypes type) {
    if (type == LogTypes::SYSTEM_LOG) {
            return quill::Frontend::create_or_get_logger("System", 
                   quill::Frontend::create_sink<quill::ConsoleSink>());
        }
        return quill::Frontend::create_or_get_logger("General", 
               quill::Frontend::create_sink<quill::ConsoleSink>());
    }
}
enum class LogTypes {
    GNERAL_LOG,
    SYSTEM_LOG
};

