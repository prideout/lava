# lava

This repository contains the source code and dependencies required to build the **lava** library and demo apps. At the time of this writing, only macOS is supported.

## Design philosophy

This is a lightweight Vulkan library that makes no **vkCmd\*** calls; that's your job. It also doesn't include a materials system, or a scene graph, or an asset loader, or any platform-specific stuff like windowing and events. However it does a lot of other things!

lava is written in a subset of C++17 that forbids RTTI, exceptions, nested namespaces, and the use of `<iostream>`.

The public API is an even narrower subset of C++ whereby classes can only contain public methods.

## Code style

The code is vertically compact, but no single line should be longer than 100 characters.

## How to build and run the demo apps

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
