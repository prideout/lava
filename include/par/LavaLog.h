// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace par {

class LavaLog {
public:
    LavaLog();

    template<typename Arg1, typename... Args>
    void trace(const char *fmt, const Arg1 &, const Args &... args) const;

    template<typename Arg1, typename... Args>
    void debug(const char *fmt, const Arg1 &, const Args &... args) const;

    template<typename Arg1, typename... Args>
    void info(const char *fmt, const Arg1 &, const Args &... args) const;

    template<typename Arg1, typename... Args>
    void warn(const char *fmt, const Arg1 &, const Args &... args) const;

    template<typename Arg1, typename... Args>
    void error(const char *fmt, const Arg1 &, const Args &... args) const;

    template<typename Arg1, typename... Args>
    void critical(const char *fmt, const Arg1 &, const Args &... args) const;

    template<typename T>
    void trace(const T &msg) const;

    template<typename T>
    void debug(const T &msg) const;

    template<typename T>
    void info(const T &msg) const;

    template<typename T>
    void warn(const T &msg) const;

    template<typename T>
    void error(const T &msg) const;

    template<typename T>
    void critical(const T &msg) const;

private:
    std::shared_ptr<spdlog::logger> mLogger;
};

template<typename Arg1, typename... Args>
inline void LavaLog::trace(const char *fmt, const Arg1 &arg1, const Args &... args) const {
    mLogger->log(spdlog::level::trace, fmt, arg1, args...);
}

template<typename Arg1, typename... Args>
inline void LavaLog::debug(const char *fmt, const Arg1 &arg1, const Args &... args) const {
    mLogger->log(spdlog::level::debug, fmt, arg1, args...);
}

template<typename Arg1, typename... Args>
inline void LavaLog::info(const char *fmt, const Arg1 &arg1, const Args &... args) const {
    mLogger->log(spdlog::level::info, fmt, arg1, args...);
}

template<typename Arg1, typename... Args>
inline void LavaLog::warn(const char *fmt, const Arg1 &arg1, const Args &... args) const {
    mLogger->log(spdlog::level::warn, fmt, arg1, args...);
}

template<typename Arg1, typename... Args>
inline void LavaLog::error(const char *fmt, const Arg1 &arg1, const Args &... args) const {
    mLogger->log(spdlog::level::err, fmt, arg1, args...);
}

template<typename Arg1, typename... Args>
inline void LavaLog::critical(const char *fmt, const Arg1 &arg1, const Args &... args) const {
    mLogger->log(spdlog::level::critical, fmt, arg1, args...);
}

template<typename T>
inline void LavaLog::trace(const T &msg) const {
    mLogger->log(spdlog::level::trace, msg);
}

template<typename T>
inline void LavaLog::debug(const T &msg) const {
    mLogger->log(spdlog::level::debug, msg);
}

template<typename T>
inline void LavaLog::info(const T &msg) const {
    mLogger->log(spdlog::level::info, msg);
}

template<typename T>
inline void LavaLog::warn(const T &msg) const {
    mLogger->log(spdlog::level::warn, msg);
}

template<typename T>
inline void LavaLog::error(const T &msg) const {
    mLogger->log(spdlog::level::err, msg);
}

template<typename T>
inline void LavaLog::critical(const T &msg) const {
    mLogger->log(spdlog::level::critical, msg);
}

}
