To verify that you have Vulkan libs available:

```
find ${ANDROID_HOME}/ndk-bundle -name vulkan*.h
find ${ANDROID_HOME}/ndk-bundle -name libvulkan.so
```

To build:

```
cd lava/extras/android
gradle build
```

To verify:
```
ls -l app/build/outputs/apk/debug/app-debug.apk
jar -tvf app/build/outputs/apk/debug/app-debug.apk | grep .so$
jar -tvf app/build/outputs/apk/release/app-release-unsigned.apk | grep .so$
```

To install, wake device, and launch:
```
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell input keyevent 26 ; adb shell input keyevent 82
adb shell am start -n net.prideout.lava.lavademo/android.app.NativeActivity
```

To clobber:
```
rm -rf build app/build app/.externalNativeBuild
```
