
**lava** is a small set of Vulkan utilities that never adds to the command buffer. For example,
the application must invoke `vkCmdDraw` on its own, but it can use lava to create the
`VkDevice` and `VkQueue`. The API consists of the following classes:

- [**LavaContext**](include/par/LavaContext.h)
  creates an instance, device, swap chain, and command queue.
- [**LavaLoader**](include/par/LavaLoader.h)
  loads all Vulkan entry points (include this instead of `vulkan.h`)
- [**LavaPipeCache**](include/par/LavaPipeCache.h)
  manages a set of pipelines for a given pipeline layout.
- [**LavaCpuBuffer**](include/par/LavaCpuBuffer.h)
  is a shared CPU-GPU buffer, useful for staging or uniform buffers.
- [**LavaGpuBuffer**](include/par/LavaGpuBuffer.h)
  is a fast device-only buffer used for vertex buffers and index buffers.
- **LavaBinder**
  creates a descriptor set layout amd manages a set of corollary descriptor sets.
- **LavaTexture**
- **LavaSamplerCache**
- **LavaUniformBuffer**

## Usage

Each Lava class is completely independent of other Lava classes. Each class lives in the `par`
namespace, and is instanced using a static `create` method.
 
Use `LavaContext` to create the standard litany of init-time Vulkan objects: an instance, a device,
a couple command buffers, etc.  For example:

```cpp
auto context = par::LavaContext::create({
    .depthBuffer = false,
    .validation = true,
    .createSurface = [window] (VkInstance instance) {
        VkSurfaceKHR surface;
        glfwCreateWindowSurface(instance, window, nullptr, &surface);
        return surface;
    }
});
const VkDevice device = context->getDevice();
const VkExtent2D extent = context->getSize();
// Do stuff here...
par::LavaContext::destroy(&context);
```

You can also use `LavaContext` as an aid for submitting command buffers and presenting the swap
chain:

```cpp
VkCommandBuffer cmdbuffer = context->beginFrame();
vkCmdBeginRenderPass(cmdbuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
vkCmdBindPipeline(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
vkCmdBindVertexBuffers(cmdbuffer, 0, 1, buffer, offsets);
vkCmdDraw(cmdbuffer, 3, 1, 0, 0);
vkCmdEndRenderPass(cmdbuffer);
context->endFrame();
```

Another Lava component is `LavaPipeCache`, which makes it easy to create pipeline objects on the
fly, as well as modifying rasterization state:

```cpp
auto pipelines = par::LavaPipeCache::create({
    .device = device,
    .vertex = {
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .attributes = { {
            .binding = 0u,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .location = 0u,
            .offset = 0u,
        } },
        .buffers = { {
            .binding = 0u,
            .stride = 12,
        } }
    },
    .descriptorLayouts = {},
    .vshader = vertexShaderModule,
    .fshader = fragmentShaderModule,
    .renderPass = renderPass
});
VkPipeline pipeline = pipelines->getPipeline();
// Do stuff here...
pipelines->setRenderPass(newRenderPass);
pipelines->setRasterState(newRasterState);
pipeline = pipelines->getPipeline();
// Do stuff here...
par::LavaPipeCache::destroy(&pipelines);
```

## Scope

By design, lava does not include a materials system, a scene graph, an asset loader, or any
platform-specific functionality like windowing and events.

lava is written in a subset of C++14 that forbids RTTI, exceptions, and the use
of `<iostream>`.

The public API is an even narrower subset of C++ whereby classes contain nothing but public methods.

The core library has dependencies on any third-party libraries other than the single-header
[vk_mem_alloc.h](src/vk_mem_alloc.h), which is directly included in the repo for convenience.

## Supported platforms

At the time of this writing, the only Vulkan implementation that we're testing against is MoltenVK,
but it should work on other platforms with just a bit of tweaking to the CMake file.

## How to build and run the demos

1. Clone this repo with `--recursive` to get the submodules.
1. Install the LunarG Vulkan SDK for macOS (see below).
1. Use brew to install cmake and ninja.
1. Invoke the following commands in your terminal.

```bash
cd <path to repo>
mkdir cmake-debug ; cd cmake-debug
rm -rf * ; cmake .. -G Ninja
ninja && ./vtriangle
```

You should now see a Cornell Box that looks like this:

[placeholder]

## LunarG SDK Instructions

* Download the tarball from their website.
* Copy its contents to `~/Vulkan`
* Add this to your `.profile`

```bash
export VULKAN_SDK=$HOME/Vulkan
export VK_LAYER_PATH=$VULKAN_SDK/macOS/etc/vulkan/explicit_layers.d
export VK_ICD_FILENAMES=$VULKAN_SDK/macOS/etc/vulkan/icd.d/MoltenVK_icd.json
export PATH="$VULKAN_SDK/macOS/bin:$PATH"
```

## Internal code style

The code is vertically compact, but no single line should be longer than 100 characters. All
public-facing Lava types live in the `par` namespace.

For `#include`, always use angle brackets unless including a private header that lives in the same
directory. Includes are arranged in blocks, where each block is an alphabetized list of headers. The
first block is composed of `par` headers, followed by a sorted list of blocks for each `extern/`
library, followed by a block of C++ STL headers, followed by the block of standard C headers,
followed by the block of private headers. For example:

```C++
#include <par/LavaContext.h>
#include <par/LavaLog.h>

#include <SPIRV/GlslangToSpv.h>

#include <string_view>
#include <vector>

#include "LavaInternal.h"
```

Methods and functions should have comments that are descriptive ("Opens the file") rather than
imperative ("Open the file").
