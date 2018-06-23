This folder contains a series of simple independent Lava demos that use
[GLFW](https://github.com/glfw/glfw) for windowing and
[glslang](https://github.com/KhronosGroup/glslang) for real-time SPIRV generation.

Note that the core Lava library does not have dependencies on GLFW or glslang. **AmberProgram** and
**AmberCompiler** depend on glslang but they live outside the core Lava library.

- [clear_screen](01_clear_screen.cpp)
  is the simplest demo and does not draw any geometry.
  Demonstrates **LavaContext** and **AmberProgram**.
- [triangle_shared](02_triangle_shared.cpp)
  draws a triangle using a vertex buffer that resides in shared CPU-GPU memory.
  Demonstrates **LavaPipeCache** and **LavaCpuBuffer**.
- [triangle_staged](03_triangle_staged.cpp)
  draws a triangle a vertex buffer that is copied from a staging area.
  Demonstrates **LavaGpuBuffer** and the **work API** in LavaContext.
- [triangle_recorded](04_triangle_recorded.cpp)
  draws a triangle using a recorded command buffer.
  Demonstrates the **recording API** in LavaContext.
- [spinny_double](05_spinny_double.cpp)
  draws a spinning triangle with a mat4 uniform representing rotation.
  Demonstrates **LavaDescCache** and a double-buffered **LavaCpuBuffer** for uniforms.
- [spinny_staged](06_spinny_staged.cpp)
  draws a spinning triangle with a mat4 uniform representing rotation.
  Demonstrates **LavaGpuBuffer** for uniforms and **vkCmdCopyBuffer**.
- [hello_texture](07_hello_texture.cpp)
  Demonstrates **LavaTexture**.
- [klein_bottle](08_klein_bottle.cpp)
  *This will demonstrate **LavaGeometry**, work in progress.*
- [particle_system](0a_particle_system.cpp)
  Fun with point sprites.
- [shadertoy](0b_shadertoy.cpp)
  Full screen triangle with a complex fragment shader.


| Demo | Description  |
|------|--------------|
| [clear_screen](01_clear_screen.cpp)            |  Simplest demo and does not draw any geometry.
| [triangle_shared](02_triangle_shared.cpp)      |  Draws a triangle using a vertex buffer that resides in shared CPU-GPU memory.
| [triangle_staged](03_triangle_staged.cpp)      |  Draws a triangle using a vertex buffer that is copied from a staging area.
| [triangle_recorded](04_triangle_recorded.cpp)  |  Draws a triangle using a recorded command buffer.
| [klein_bottle](08_klein_bottle.cpp)            |  Indexed triangles with a depth buffer and MSAA.
| [particle_system](0a_particle_system.cpp)      |   Fun with point sprites.
| [shadertoy](0b_shadertoy.cpp)                  | Full screen triangle with a complex fragment shader.
