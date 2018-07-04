Building Lava in the usual way (as described
[here](http://github.prideout.net/lava/#buildingandrunningthedemos)) will create
executables that can run on macOS, but does not create actual app bundles that can be installed
on anybody's machine.

This folder exists solely to allow creation of macOS app bundles. Using this is optional.

```bash
> git clone --recurse-submodules https://github.com/prideout/lava.git
> cd extras/macos
> cmake -H. -B.debug -GXcode    # create Xcode project
> cmake --build .debug          # build Xcode project
> open .debug/Debug/lavamac.app # launch the app
```
