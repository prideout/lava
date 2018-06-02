
**Lava** is a small set of Vulkan utilities that never adds to the command buffer. For example,
the application must invoke `vkCmdDraw` on its own, but it can use lava to create the
`VkDevice` and `VkQueue`. Lava consists of the following components:

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
- [**LavaDescCache**](include/par/LavaDescCache.h)
  creates a descriptor set layout and manages a set of corollary descriptor sets.
- **LavaTexture**

Each Lava class is completely independent of every other class, so clients can choose a subset
of functionality as needed.
 
## Scope

By design, lava does not include a materials system, a scene graph, an asset loader, or any
platform-specific functionality like windowing and events.

lava is written in a subset of C++14 that forbids RTTI, exceptions, and the use
of `<iostream>`.

The public API is an even narrower subset of C++ whereby classes contain nothing but public methods.

The core library has no dependencies on any third-party libraries other than the single-file
[vk_mem_alloc.h](src/vk_mem_alloc.h) library and [spdlog](https://github.com/gabime/spdlog), which
are included in the repo for convenience.

## Supported platforms

At the time of this writing, we're testing against Linux and MoltenVK on MacOS.
