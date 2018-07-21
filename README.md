<img src="https://github.com/prideout/lava/raw/master/extras/assets/klein31416.gif" height="128px">

[![Build Status](https://travis-ci.org/prideout/lava.svg?branch=master)](https://travis-ci.org/prideout/lava)

*At this point Lava is just an experimental playground, I do not recommend using it in a production
setting.*

**Lava** is a toy C++ library composed of classes that make it easy to create and manage Vulkan
objects.  Each Lava class is defined by a single header with no dependencies on anything other than
`vulkan.h` and the STL.

For more information, see the [documentation](http://github.prideout.net/lava/).

## Scope

Lava does not include a materials system, a scene graph, an asset loader, or any
platform-specific functionality like windowing and events.

Lava is implemented with a subset of C++14 that forbids RTTI, exceptions, and the use of
`<iostream>`. The public API uses a very narrow subset of C++ whereby classes contain nothing but
methods.

The core library has no dependencies on any third-party libraries other than the single-file
[vk_mem_alloc.h](src/vk_mem_alloc.h) library and [spdlog](https://github.com/gabime/spdlog), which
are included in the repo for convenience.

## Supported platforms

Lava supports macOS via MoltenVK, as well as Linux and Android. It should be easy to extend to other
platforms in the future.
