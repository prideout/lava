# lava

This is a lightweight C++ Vulkan library that never adds to the command buffer (e.g., it never
invokes `vkCmdDraw`). Writing to the command buffer is your job! The entire API consists of a
handful of simple classes:

- **LavaContext** manages the instance, device, swap chain, and command buffers.
- **LavaBinder** manages pipelines, descriptor sets, pipeline layouts, descriptor set layouts.
- **LavaCpuBuffer** is a shared CPU-GPU buffer, useful for staging or uniform blocks.
- **LavaGpuBuffer** is a fast device-only buffer used for vertex buffers and index buffers.
- **LavaFramebuffer** is an abstraction of an off-screen rendering surface.
- **LavaCompiler** helps you perform GLSL => SPIRV translation at run time.

Textures, UniformBlocks, and Programs?

## Philosophy and style

By design, lava does not include a materials system, or a scene graph, or an asset loader, or any
platform-specific functionality like windowing and events. However it does a lot of other things!

lava is written in a subset of C++17 that forbids RTTI, exceptions, nested namespaces, and the use
of `<iostream>`.

The public API is an even narrower subset of C++ whereby classes contain nothing but public methods.

The code is vertically compact, but no single line should be longer than 100 characters. All
public-facing Lava types live in the `par` namespace and there are no nested namespaces.

At the time of this writing, the only Vulkan implementation that we're testing against is MoltenVK,
but it should work on other platforms with just a bit of tweaking to the CMake file.

## How to build and run the demos

1. Clone this repo with `--recursive` to get the submodules.
1. Install the LunarG Vulkan SDK for macOS (see below).
1. Use brew to install clang, cmake, and ninja.
1. Invoke the following commands in your terminal.

```bash
cd <path to repo>
mkdir cmake-debug ; cd cmake-debug
rm -rf * ; cmake -DCMAKE_BUILD_TYPE=Debug .. -G Ninja
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
export PATH=$VULKAN_SDK/macOS/bin:$PATH
export VK_LAYER_PATH=$VULKAN_SDK/macOS/etc/vulkan/explicit_layers.d
export VK_ICD_FILENAMES=$VULKAN_SDK/macOS/etc/vulkan/icd.d/MoltenVK_icd.json
```
