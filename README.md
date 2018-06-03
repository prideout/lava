
**Lava** is a small set of C++ utilities for Vulkan that never add to the command buffer. 
Each Lava class is completely independent of every other class, so clients can choose a subset
of functionality as needed.

For more information, see the [documentation](http://github.prideout.net/lava/index.html).

## Scope

Lava does not include a materials system, a scene graph, an asset loader, or any
platform-specific functionality like windowing and events.

We use a subset of C++14 that forbids RTTI, exceptions, and the use of `<iostream>`.

The public API is very narrow subset of C++ whereby classes contain nothing but public methods.

The core library has no dependencies on any third-party libraries other than the single-file
[vk_mem_alloc.h](src/vk_mem_alloc.h) library and [spdlog](https://github.com/gabime/spdlog), which
are included in the repo for convenience.

## Supported platforms

Lava supports Linux, as well as MacOS via MoltenVK, and it should be easy to add additional
platforms in the future.