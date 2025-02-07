#pragma once

#ifndef CPUE_NO_LOGGING

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace CPUE {

class Log {
public:
    static void init();

    static std::shared_ptr<spdlog::logger>& get_logger() { return s_logger; }

private:
    static std::shared_ptr<spdlog::logger> s_logger;
};

}

#ifdef CPUE_ENABLE_TRACE
#define CPUE_TRACE(...) ::CPUE::Log::get_logger()->trace(__VA_ARGS__)
#else
#define CPUE_TRACE(...)
#endif
#define CPUE_INFO(...)     ::CPUE::Log::get_logger()->info(__VA_ARGS__)
#define CPUE_WARN(...)     ::CPUE::Log::get_logger()->warn(__VA_ARGS__)
#define CPUE_ERROR(...)    ::CPUE::Log::get_logger()->error(__VA_ARGS__)
#define CPUE_CRITICAL(...) ::CPUE::Log::get_logger()->critical(__VA_ARGS__)

#else

#define CPUE_TRACE(...)
#define CPUE_INFO(...)
#define CPUE_WARN(...)
#define CPUE_ERROR(...)
#define CPUE_CRITICAL(...)

#endif