
**lava** is a small set of Vulkan utilities that never adds to the command buffer. For example,
the application must invoke `vkCmdDraw` on its own, but it can use lava to create the
`VkDevice` and `VkQueue`. The API consists of the following classes:

- [**LavaContext**](include/par/LavaContext.h)
  manages an instance, device, swap chain, and command queue.
- [**LavaLoader**](include/par/LavaLoader.h)
  loads all Vulkan entry points (include this instead of `vulkan.h`)
- [**LavaProgram**](include/par/LavaProgram.h)
  consumes GLSL or SPIRV and wraps a pair of `VkShaderModule` handles.
- [**LavaPipeCache**](include/par/LavaPipeCache.h)
  manages a set of pipelines for a given pipeline layout.
- [**LavaCpuBuffer**](include/par/LavaCpuBuffer.h)
  is a shared CPU-GPU buffer, useful for staging or uniform buffers.
- [**LavaGpuBuffer**](include/par/LavaGpuBuffer.h)
  is a fast device-only buffer used for vertex buffers and index buffers.
- **LavaBinder**
  creates a descriptor set layout amd manages a set of corollary descriptor sets.
- **LavaFramebuffer**
  is an abstraction of an off-screen rendering surface.

Textures, UniformBlocks?

Each Lava class is independent of every other Lava class. For example, `LavaBinder` takes
`VkShaderModule` rather than `LavaProgram`. This allows applications to select which subset of lava
functionality they wish to use.

In the name of simplicity, Lava is intentionally constrained and opinionated. For example,
**LavaBinder** always creates 4 descriptor sets.

## Philosophy and style

By design, lava does not include a materials system, or a scene graph, or an asset loader, or any
platform-specific functionality like windowing and events.

lava is written in a subset of C++14 that forbids RTTI, exceptions, nested namespaces, and the use
of `<iostream>`.

The public API is an even narrower subset of C++ whereby classes contain nothing but public methods.

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
public-facing Lava types live in the `par` namespace and there are no nested namespaces.

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
