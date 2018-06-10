// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLog.h>

using namespace par;

static bool sFirst = true;

LavaLog par::llog;

LavaLog::LavaLog() {
    if (sFirst) {
        mLogger = spdlog::stdout_color_mt("console");
        #ifndef NDEBUG
        mLogger->set_level(spdlog::level::debug);
        #endif
        mLogger->set_pattern("%T %t %^%v%$");
        sFirst = false;
    } else {
        mLogger = spdlog::get("console");
    }
}
