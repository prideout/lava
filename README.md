
#. Install the LunarG SDK for macOS.
#. Use brew to install clang, cmake, and ninja.
#. Perform the following commands to build & run.

```
mkdir cmake-debug ; cd cmake-debug ; rm -rf *
cmake -DCMAKE_BUILD_TYPE=Debug .. -G Ninja
ninja && ./vtriangle
```
