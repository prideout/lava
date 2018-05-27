// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <vk_mem_alloc.h>

#include <vector>

#define VKALLOC nullptr

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
}

namespace par {

VmaAllocator getVma(VkDevice device, VkPhysicalDevice gpu);
void createVma(VkDevice device, VkPhysicalDevice gpu);
void destroyVma(VkDevice device);

uint64_t getCurrentTime();
size_t murmurHash(uint32_t const* words, uint32_t nwords, uint32_t seed);

template<typename T>
struct MurmurHashFn {
    uint32_t operator()(const T& key) const {
        static_assert(0 == (sizeof(key) & 3), "Hashing requires a size that is a multiple of 4.");
        return murmurHash((uint32_t const *) &key, sizeof(key) / 4, 0u);
    }
};

// Wraps a std::vector and exposes the data pointer and size as public fields.
//
// This works nicely with Vulkan queries. For example:
//     LavaVector<VkExtensionProperties> props;
//     vkEnumerateInstanceExtensionProperties(nullptr, &props.size, nullptr);
//     vkEnumerateInstanceExtensionProperties(nullptr, &props.size, props.alloc());
template <typename T> struct LavaVector {
    uint32_t size = 0;
    T* data = nullptr;
    LavaVector() {}
    LavaVector(std::initializer_list<T> init) : mVector(init) { update(); }
    T* alloc() {
        mVector.resize(size);
        update();
        return data;
    }
    T& operator[](size_t t) {
        return mVector[t];
    }
    LavaVector<T>& operator=(const LavaVector<T>& that) {
        mVector = that.mVector;
        update();
        return *this;
    }
    template<class II> void assign(II first, II last) {
        mVector.assign(first, last);
        update();
    }
    void push_back(const T& t) {
        mVector.push_back(t);
        update();
    }
    void clear() {
        mVector.clear();
        update();
    }
    typename std::vector<T>::const_iterator begin() const { return mVector.begin(); }
    typename std::vector<T>::const_iterator end() const { return mVector.end(); }
private:
    void update() {
        data = mVector.data();
        size = static_cast<uint32_t>(mVector.size());
    }
    std::vector<T> mVector;
};

}
