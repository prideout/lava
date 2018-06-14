#include <jni.h>
#include <string>
#include <vulkan/vulkan.h>

extern "C" JNIEXPORT jstring

JNICALL
Java_net_prideout_lava_lavademo_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
