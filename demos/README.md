This folder contains a series of simple independent Lava demos that use
[GLFW](https://github.com/glfw/glfw) for windowing and
[glslang](https://github.com/KhronosGroup/glslang) for real-time SPIRV generation.

Note that the core Lava library does not have dependencies on GLFW or glslang. **AmberProgram** and
**AmberCompiler** depend on glslang but they live outside the core Lava library.

| Demo | Description  |
|------|--------------|
| [clear_screen](01_clear_screen.cpp)            | Simplest demo and does not draw any geometry.
| [triangle_shared](02_triangle_shared.cpp)      | Draws a triangle using a vertex buffer that resides in shared CPU-GPU memory.
| [triangle_staged](03_triangle_staged.cpp)      | Draws a triangle using a vertex buffer uploaded from a staging area.
| [triangle_recorded](04_triangle_recorded.cpp)  | Draws a triangle using a recorded command buffer.
| [klein_bottle](08_klein_bottle.cpp)            | Indexed triangles with a depth buffer and MSAA.
| [particle_system](0a_particle_system.cpp)      | Fun with point sprites.
| [shadertoy](0b_shadertoy.cpp)                  | Full screen triangle with a complex fragment shader.
