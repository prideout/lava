To build:

```
cd lava/extra/android
gradle build
adb install -r ./app/build/outputs/apk/debug/app-debug.apk
```

To launch:
```
adb shell input keyevent 26 ; adb shell input keyevent 82
adb shell am start -n net.prideout.lava.lavademo/.MainActivity
```
