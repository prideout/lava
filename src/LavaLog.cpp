// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLog.h>

using namespace par;

static bool sFirst = true;

LavaLog::LavaLog() {
    if (sFirst) {
        mLogger = spdlog::stdout_color_mt("console");
        mLogger->set_pattern("%T %t %^%v%$");
        sFirst = false;
    } else {
        mLogger = spdlog::get("console");
    }
}
