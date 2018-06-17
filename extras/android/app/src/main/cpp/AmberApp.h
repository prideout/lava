#pragma once

struct AmberAppImpl;

struct AmberApp {
    AmberApp(void* app);
    ~AmberApp();
    bool isReady() const;
    void draw();
    AmberAppImpl& self;
};
