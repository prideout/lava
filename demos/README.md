This folder contains a series of simple independent Lava demos that use
[glfw](https://github.com/glfw/glfw) for windowing and
[glslang](https://github.com/KhronosGroup/glslang) for real-time SPIRV generation.

Note that the core Lava library does not have dependencies on GLFW or glslang. **AmberProgram** and
**AmberCompiler** depend on glslang but they live outside the core Lava library.

- [01_shader_test](shader_test.cpp)
  is the simplest demo and does not draw any geometry.
  Demonstrates **LavaContext** and **AmberProgram**.
- [02_triangle_shared](triangle_shared.cpp)
  draws a triangle using a vertex buffer that resides in shared CPU-GPU memory.
  Demonstrates **LavaPipeCache** and **LavaCpuBuffer**.
- [03_triangle_staged](triangle_staged.cpp)
  draws a triangle a vertex buffer that is copied from a staging area.
  Demonstrates **LavaGpuBuffer** and the **work API** in LavaContext.
- [04_triangle_recorded](triangle_recorded.cpp)
  draws a triangle using a recorded command buffer.
  Demonstrates the **recording API** in LavaContext.
- [05_spinny_double](spinny_double.cpp)
  draws a spinning triangle with a mat4 uniform representing rotation.
  Demonstrates **LavaDescCache** and a double-buffered **LavaCpuBuffer** for uniforms.
- [06_spinny_staged](spinny_staged.cpp)
  draws a spinning triangle with a mat4 uniform representing rotation.
  Demonstrates **LavaGpuBuffer** for uniforms and **vkCmdCopyBuffer**.
