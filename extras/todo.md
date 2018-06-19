# Android

    TriangleRecordedApp::draw
    Gradle Wrapper (Test with linux)
    AmberApplication, AmberMain
        Refactor all non-android demos (keep their file names though)
    Android: swipe to navigate demos, with a hardcoded "entry" app?
    HelloTexture

# LavaContext

    expose a pointer to VkRenderPassBeginInfo for convenience.
    recording API should take a callback instead of begin / end

# LavaCpuBuffer

    remove setData in favor of persistent mapping

# 08_klein_bottle

    Introduce LavaGeometry
        only supports two "attribs" because it assumes non-position data is interleaved

# gli and ktx pipeline

    cdwfs/img2ktx

# .clang-format

    .clang-format
        AlignAfterOpenBracket: BAS_DontAlign

# lavaray

    Lava as a submodule
    RadeonRays as a submodule, in OpenCL mode for now.
    Emulate their cornell box example (non shadowed)
    Emulate their shadowed cornell box example (including OpenGL kernel)

# LavaTexture

    Add miplevel support, test with stb_image_resize

# Texture space caching

    https://software.intel.com/sites/default/files/managed/b4/a0/author_preprint_texture-space-caching-and-reconstruction-for-ray-tracing.pdf

# Demo ideas

    streamlines     https://www.kovach.me/posts/2018-04-30-blotch.html
    space_colony    https://twitter.com/BendotK/status/1005209740874584064
                    http://algorithmicbotany.org/papers/colonization.egwnp2007.large.pdf
                    http://algorithmicbotany.org/papers/venation.sig2005.pdf
    demos/circle    draws filled-in circle with random sampling
    demos/gamma     https://www.shadertoy.com/view/llBGz1
    demos/smallpt   https://www.codeplay.com/portal/sycl-ing-the-smallpt-raytracer
    demos/sdfshapes https://www.shadertoy.com/view/4dfXDn
    demos/skinning  Cesium Skinning and glTF and rapidjson
    https://github.com/prideout/recipes
    http://xeogl.org/examples/#materials_metallic_fireHydrant
    https://github.com/syoyo/tinygltf

# Path tracer notes

    https://developer.apple.com/documentation/metalperformanceshaders/metal_for_accelerating_ray_tracing
    https://github.com/prideout/aobaker
        https://github.com/RobotLocomotion/homebrew-director/blob/master/Formula/embree.rb
        https://embree.github.io/data/embree-siggraph-2017-final.pdf
    https://github.com/aras-p/ToyPathTracer
    https://github.com/lighttransport/nanort
    http://aantron.github.io/better-enums/ (overkill)
    https://www.cgbookcase.com/downloads/
    http://graphics.stanford.edu/~henrik/images/cbox.html (best cornell box)
    https://github.com/SaschaWillems/Vulkan/blob/master/examples/raytracing/raytracing.cpp
    https://github.com/GoGreenOrDieTryin/Vulkan-GPU-Ray-Tracer
    http://www.kevinbeason.com/smallpt/
    https://github.com/munificent/smallpt/blob/master/smallpt.cpp
    http://www.hxa.name/minilight/
