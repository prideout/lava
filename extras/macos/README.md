Building the Lava demos in the usual way (as described in the root-level README) will create
executables that can run on macOS, but does not create actual app bundles.

This folder exists solely to allow creation of app bundles without opening the Xcode GUI.

```bash
> git clone --recurse-submodules https://github.com/prideout/lava.git
> cd extras/macos
> cmake -H. -B.debug -GXcode    # create Xcode project
> cmake --build .debug          # build Xcode project
> open .debug/Debug/lavamac.app # launch the app
```
