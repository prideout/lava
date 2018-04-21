
1. Clone this repo with `--recursive` to get the submodules.
1. Install the LunarG Vulkan SDK for macOS.
1. Use brew to install clang, cmake, and ninja.
1. Invoke the following commands in your terminal.

```
cd <path to repo>
mkdir cmake-debug ; cd cmake-debug
rm -rf * ; cmake -DCMAKE_BUILD_TYPE=Debug .. -G Ninja
ninja && ./vtriangle
```
