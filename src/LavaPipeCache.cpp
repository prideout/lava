// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/LavaPipeCache.h>
#include <par/LavaLog.h>

#include <unordered_map>

#include "LavaInternal.h"

using namespace std;

namespace par {

namespace {

struct CacheKey {
    LavaPipeCache::RasterState raster;
    LavaPipeCache::VertexState vertex;
    VkShaderModule vshader;
    VkShaderModule fshader;
    VkRenderPass renderPass;
};

struct CacheVal {
    VkPipeline handle;
    uint64_t timestamp;
    // move-only (disallow copy) to allow keeping a pointer to a value in the map.
    CacheVal(CacheVal const&) = delete;
    CacheVal& operator=(CacheVal const&) = delete;
    CacheVal(CacheVal &&) = default;
    CacheVal& operator=(CacheVal &&) = default;
};

struct IsEqual {
    using RasterState = LavaPipeCache::RasterState;
    using VertexState = LavaPipeCache::VertexState;
    bool operator()(const VkVertexInputBindingDescription& a,
            const VkVertexInputBindingDescription& b) const {
        return a.binding == b.binding && a.stride == b.stride && a.inputRate == b.inputRate;
    }
    bool operator()(const VkVertexInputAttributeDescription& a,
            const VkVertexInputAttributeDescription& b) const {
        return a.location == b.location && a.binding == b.binding && a.format == b.format &&
                a.offset == b.offset;
    }
    bool operator()(const CacheKey& a, const CacheKey& b) const {
        return (*this)(a.raster, b.raster) && (*this)(a.vertex, b.vertex) &&
                a.vshader == b.vshader && a.fshader == b.fshader && a.renderPass == b.renderPass;
    }
    bool operator()(const RasterState& a, const RasterState& b) const {
        return 0 == memcmp((const void*) &a, (const void*) &b, sizeof(b));
    }
    bool operator()(const VertexState& a, const VertexState& b) const {
        if (a.topology != b.topology) {
            return false;
        }
        if (a.attributes.size() != b.attributes.size()) {
            return false;
        }
        for (size_t i = 0; i < a.attributes.size(); ++i) {
            if (!(*this)(a.attributes[i], b.attributes[i])) {
                return false;
            }
        }
        if (a.buffers.size() != b.buffers.size()) {
            return false;
        }
        for (size_t i = 0; i < a.buffers.size(); ++i) {
            if (!(*this)(a.buffers[i], b.buffers[i])) {
                return false;
            }
        }
        return true;
    }
};

using Cache = unordered_map<CacheKey, CacheVal, MurmurHashFn<CacheKey>, IsEqual>;

namespace DirtyFlag {
    static constexpr uint8_t RASTER = 1 << 0; 
    static constexpr uint8_t VERTEX = 1 << 1;
    static constexpr uint8_t SHADER = 1 << 2;
    static constexpr uint8_t PASS   = 1 << 3;
}

struct LavaPipeCacheImpl : LavaPipeCache {
    ~LavaPipeCacheImpl() noexcept;
    CacheVal* currentPipeline = nullptr;
    VkDevice device;
    Cache cache;
    CacheKey currentState;
    uint8_t dirtyFlags = 0xf;
    VkPipelineLayout pipelineLayout;
};

LAVA_DEFINE_UPCAST(LavaPipeCache)

LavaPipeCache::RasterState createDefaultRasterState() {
    VkPipelineRasterizationStateCreateInfo rasterization {};
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.cullMode = VK_CULL_MODE_NONE;
    rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization.depthClampEnable = VK_FALSE;
    rasterization.rasterizerDiscardEnable = VK_FALSE;
    rasterization.depthBiasEnable = VK_FALSE;
    rasterization.lineWidth = 1.0f;
    VkPipelineColorBlendAttachmentState blending = {};
    blending.colorWriteMask = 0xf;
    blending.blendEnable = VK_FALSE;
    VkPipelineDepthStencilStateCreateInfo depthStencil {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.back.failOp = VK_STENCIL_OP_KEEP;
    depthStencil.back.passOp = VK_STENCIL_OP_KEEP;
    depthStencil.back.compareOp = VK_COMPARE_OP_ALWAYS;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = depthStencil.back;
    VkPipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.pSampleMask = nullptr;
    return {
        rasterization,
        blending,
        depthStencil,
        multisampling,
    };
}

} // anonymous namespace

LavaPipeCache* LavaPipeCache::create(Config config) noexcept {
    assert(config.device);
    auto impl = new LavaPipeCacheImpl;
    impl->device = config.device;
    impl->currentState = {
        .raster = createDefaultRasterState(),
        .vertex = config.vertex,
        .vshader = config.vshader,
        .fshader = config.fshader,
        .renderPass = config.renderPass
    };
    auto& layouts = config.descriptorLayouts;
    VkPipelineLayoutCreateInfo info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = (uint32_t) layouts.size(),
        .pSetLayouts = layouts.empty() ? nullptr : layouts.data()
    };
    vkCreatePipelineLayout(impl->device, &info, VKALLOC, &impl->pipelineLayout);
    return impl;
}

void LavaPipeCache::operator delete(void* ptr) {
    auto impl = (LavaPipeCacheImpl*) ptr;
    ::delete impl;
}

LavaPipeCacheImpl::~LavaPipeCacheImpl() noexcept {
    for (auto& pair : cache) {
        vkDestroyPipeline(device, pair.second.handle, VKALLOC);
    }
    vkDestroyPipelineLayout(device, pipelineLayout, VKALLOC);
}

VkPipelineLayout LavaPipeCache::getLayout() const noexcept {
    return upcast(this)->pipelineLayout;
}

VkPipeline LavaPipeCache::getPipeline() noexcept {
    LavaPipeCacheImpl* impl = upcast(this);
    VkPipeline pipeline;
    impl->getPipeline(&pipeline);
    return pipeline;
}

bool LavaPipeCache::getPipeline(VkPipeline* pipeline) noexcept {
    auto impl = upcast(this);
    if (not impl->dirtyFlags) {
        *pipeline = impl->currentPipeline->handle;
        impl->currentPipeline->timestamp = getCurrentTime();
        return false;
    }
    impl->dirtyFlags = 0;
    auto iter = impl->cache.find(impl->currentState);
    if (iter != impl->cache.end()) {
        impl->currentPipeline = &(iter->second);
        *pipeline = impl->currentPipeline->handle;
        impl->currentPipeline->timestamp = getCurrentTime();
        return true;
    }

    auto& key = impl->currentState;
    VkPipelineVertexInputStateCreateInfo vertexInputState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = (uint32_t) key.vertex.buffers.size(),
        .pVertexBindingDescriptions = key.vertex.buffers.data(),
        .vertexAttributeDescriptionCount = (uint32_t) key.vertex.attributes.size(),
        .pVertexAttributeDescriptions = key.vertex.attributes.data()
    };
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = key.vertex.topology
    };

    VkPipelineViewportStateCreateInfo viewportState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };
    VkDynamicState dynamicStateEnables[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamicState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pDynamicStates = dynamicStateEnables,
        .dynamicStateCount = 2
    };

    VkPipelineColorBlendStateCreateInfo blending {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = key.fshader ? 1u : 0u,
        .pAttachments = key.fshader ? &key.raster.blending : nullptr,
    };

    VkPipelineShaderStageCreateInfo vshader {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = key.vshader,
        .pName = "main"
    };
    VkPipelineShaderStageCreateInfo fshader {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = key.fshader,
        .pName = "main"
    };
    VkPipelineShaderStageCreateInfo shaders[] = { vshader, fshader };

    VkGraphicsPipelineCreateInfo pipelineCreateInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .layout = impl->pipelineLayout,
        .renderPass = key.renderPass,
        .stageCount = key.fshader ? 2u : 1u,
        .pStages = shaders,
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pRasterizationState = &key.raster.rasterization,
        .pMultisampleState = &key.raster.multisampling,
        .pViewportState = &viewportState,
        .pDepthStencilState = &key.raster.depthStencil,
        .pDynamicState = &dynamicState,
        .pColorBlendState = &blending
    };

    VkPipeline pipe;
    vkCreateGraphicsPipelines(impl->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, VKALLOC, &pipe);
    *pipeline = pipe;

    iter = impl->cache.emplace(make_pair(impl->currentState, CacheVal {
        .timestamp = getCurrentTime(),
        .handle = pipe
    })).first;
    impl->currentPipeline = &(iter->second);
    return true;
}

const LavaPipeCache::RasterState& LavaPipeCache::getDefaultRasterState() const noexcept {
    static auto rasterState = createDefaultRasterState();
    return rasterState;
}

void LavaPipeCache::setRasterState(const RasterState& rasterState) noexcept {
    LavaPipeCacheImpl* impl = upcast(this);
    if (!IsEqual()(rasterState, impl->currentState.raster)) {
        impl->currentState.raster = rasterState;
        impl->dirtyFlags |= DirtyFlag::RASTER;
    }
}

void LavaPipeCache::setVertexState(const VertexState& vstate) noexcept {
    LavaPipeCacheImpl* impl = upcast(this);
    if (!IsEqual()(vstate, impl->currentState.vertex)) {
        impl->currentState.vertex = vstate;
        impl->dirtyFlags |= DirtyFlag::VERTEX;
    }
}

void LavaPipeCache::setShaderModule(VkShaderStageFlagBits stage, VkShaderModule module) noexcept {
    LavaPipeCacheImpl* impl = upcast(this);
    VkShaderModule* pmodule;
    if (stage == VK_SHADER_STAGE_VERTEX_BIT) {
        pmodule = &impl->currentState.vshader;
    } else if (stage == VK_SHADER_STAGE_FRAGMENT_BIT) {
        pmodule = &impl->currentState.fshader;
    } else {
        llog.fatal("Shader stage not supported.");
    }
    if (*pmodule != module) {
        *pmodule = module;
        impl->dirtyFlags |= DirtyFlag::SHADER;
    }
}

void LavaPipeCache::setRenderPass(VkRenderPass renderPass) noexcept {
    LavaPipeCacheImpl* impl = upcast(this);
    if (renderPass != impl->currentState.renderPass) {
        impl->currentState.renderPass = renderPass;
        impl->dirtyFlags |= DirtyFlag::PASS;
    }
}

void LavaPipeCache::releaseUnused(uint64_t milliseconds) noexcept {
    LavaPipeCacheImpl* impl = upcast(this);
    const uint64_t expiration = getCurrentTime() - milliseconds;
    auto& cache = impl->cache;
    for (decltype(impl->cache)::const_iterator iter = cache.begin(); iter != cache.end();) {
        if (iter->second.timestamp < expiration) {
            iter = cache.erase(iter);
        } else {
            ++iter;
        }
    }
}

} // par namespace