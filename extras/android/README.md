First, verify that you have Vulkan libs available in your NDK:

```
find ${ANDROID_HOME}/ndk-bundle -name vulkan*.h
find ${ANDROID_HOME}/ndk-bundle -name libvulkan.so
```

To build:

```
cd lava/extras/android
./gradlew build
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
adb logcat -c ; adb shell am start -n net.prideout.lava.lavademo/android.app.NativeActivity
```

To dump the logs:
```
export PID=`adb shell ps | grep net.prideout.lava.lavademo | tr -s [:space:] ' ' | cut -d' ' -f2`
adb logcat | grep -F $PID
```

To clobber:
```
rm -rf build app/build app/.externalNativeBuild
```

To uninstall:
```
adb uninstall net.prideout.lava.lavademo
```
