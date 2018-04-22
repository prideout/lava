// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#define LAVA_IMPL_CLASS(CLASS) \
inline CLASS##Impl& upcast(CLASS& that) noexcept { \
    return static_cast<CLASS##Impl &>(that); \
} \
inline const CLASS##Impl& upcast(const CLASS& that) noexcept { \
    return static_cast<const CLASS##Impl &>(that); \
} \
inline CLASS##Impl* upcast(CLASS* that) noexcept { \
    return static_cast<CLASS##Impl *>(that); \
} \
inline CLASS##Impl const* upcast(CLASS const* that) noexcept { \
    return static_cast<CLASS##Impl const *>(that); \
} \
CLASS* CLASS::create() noexcept { \
    return new CLASS##Impl(); \
} \
void CLASS::destroy(CLASS** that) noexcept { \
    delete upcast(*that); \
    *that = nullptr; \
}
