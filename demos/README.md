This folder contains a series of simple independent Lava demos that use
[glfw](https://github.com/glfw/glfw) for windowing and
[glslang](https://github.com/KhronosGroup/glslang) for real-time SPIRV generation.

Note that the core Lava library does not have dependencies on GLFW or glslang. **AmberProgram** and
**AmberCompiler** depend on glslang but they live outside the core Lava library.

- [shader_test](shader_test.cpp)
  is the simplest demo and does not draw any geometry.
  Demonstrates **LavaContext** and **AmberProgram**.
- [triangle_shared](triangle_shared.cpp)
  draws a triangle using a vertex buffer that resides in shared CPU-GPU memory.
  Demonstrates **LavaPipeline** and **LavaCpuBuffer**.
- [triangle_staged](triangle_staged.cpp)
  draws a triangle a vertex buffer that is copied from a staging area.
  Demonstrates **LavaGpuBuffer** and the **work API** in LavaContext.
- [triangle_recorded](triangle_recorded.cpp)
  draws a triangle using a recorded command buffer.
  Demonstrates the **recording API** in LavaContext.
