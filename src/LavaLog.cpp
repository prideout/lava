// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLog.h>

using namespace par;

static bool sFirst = true;

LavaLog par::llog;

LavaLog::LavaLog() {
    if (sFirst) {
        #if defined(__ANDROID__)
        mLogger = spdlog::android_logger("console", "lava");
        mLogger->set_pattern("%^%v%$");
        #else
        mLogger = spdlog::stdout_color_mt("console");
        mLogger->set_pattern("%T %t %^%v%$");
        #endif
        #ifndef NDEBUG
        //mLogger->set_level(spdlog::level::debug);
        #endif
        sFirst = false;
    } else {
        mLogger = spdlog::get("console");
    }
}
