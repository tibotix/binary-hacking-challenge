#ifndef CPUE_NO_LOGGING

#include "logging.h"
#include <vector>

__attribute__((constructor)) static void init_logger() {
    CPUE::Log::init();
}

namespace CPUE {

std::shared_ptr<spdlog::logger> Log::s_logger;

void Log::init() {
    if (s_logger)
        return;

    std::vector<spdlog::sink_ptr> log_sinks;
    log_sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    log_sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("cpue.log", true));

    log_sinks[0]->set_pattern("%^[%T] %n: %v%$");
    log_sinks[1]->set_pattern("[%T] [%l] %n: %v");

    s_logger = std::make_shared<spdlog::logger>("CPUE", begin(log_sinks), end(log_sinks));
    spdlog::register_logger(s_logger);
    s_logger->set_level(spdlog::level::trace);
    s_logger->flush_on(spdlog::level::trace);
}

}

#else

#endif