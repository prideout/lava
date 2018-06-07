// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace par {

// Manages a set of pipeline objects that all conform to a specific pipeline layout.
//
// Creates a single VkPipelineLayout upon construction and stores it as immutable state.
// Accepts state changes via setRasterState, setVertexState, setShaderModule, and setRenderPass.
// Creates or fetches a pipeline when getPipeline() is called.
// Can optionally free least-recently-used pipelines once per frame (releaseUnused).
//
class LavaPipeCache {
public:
    struct RasterState {
        VkPipelineRasterizationStateCreateInfo rasterization;
        VkPipelineColorBlendAttachmentState blending;
        VkPipelineDepthStencilStateCreateInfo depthStencil;
        VkPipelineMultisampleStateCreateInfo multisampling;
    };
    struct VertexState {
        VkPrimitiveTopology topology;
        std::vector<VkVertexInputAttributeDescription> attributes;
        std::vector<VkVertexInputBindingDescription> buffers;
    };
    struct Config {
        VkDevice device;
        VertexState vertex;
        std::vector<VkDescriptorSetLayout> descriptorLayouts;
        VkShaderModule vshader;
        VkShaderModule fshader;
        VkRenderPass renderPass;
    };
    static LavaPipeCache* create(Config config) noexcept;
    ~LavaPipeCache() noexcept;

    // Fetches the pipeline layout that was created at construction.
    VkPipelineLayout getLayout() const noexcept;

    // Fetches or creates a VkPipeline object corresponding to the layout that was established
    // during construction, as well as the current state (see setRasterState et al).
    VkPipeline getPipeline() noexcept;

    // Returns true if the client should call vkCmdBindPipeline.
    bool getPipeline(VkPipeline* pipeline) noexcept;

    const RasterState& getDefaultRasterState() const noexcept;
    void setRasterState(const RasterState& rasterState) noexcept;
    void setVertexState(const VertexState& varray) noexcept;
    void setShaderModule(VkShaderStageFlagBits stage, VkShaderModule module) noexcept;
    void setRenderPass(VkRenderPass renderPass) noexcept;

    // Evicts pipeline objects that were last used more than N milliseconds ago.
    void releaseUnused(uint64_t milliseconds) noexcept;
protected:
    LavaPipeCache() noexcept = default;
    // par::noncopyable
    LavaPipeCache(LavaPipeCache const&) = delete;
    LavaPipeCache& operator=(LavaPipeCache const&) = delete;
};

}
