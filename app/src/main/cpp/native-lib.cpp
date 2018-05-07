#include <jni.h>
#include <string>
#include "AudioEngine.h"

extern "C" {

    JNIEXPORT jstring JNICALL Java_com_jbloit_androidlinkaudio_MainActivity_stringFromJNI(
            JNIEnv *env,
            jobject /* this */) {
        std::string hello = "Hello from C++";
        return env->NewStringUTF(hello.c_str());
    }

    JNIEXPORT jlong JNICALL
    Java_com_jbloit_androidlinkaudio_AudioEngine_createEngine(
            JNIEnv *env,
            jclass) {

        // We use std::nothrow so `new` returns a nullptr if the engine creation fails
        AudioEngine *engine = new(std::nothrow) AudioEngine();
        return (jlong) engine;
    }

JNIEXPORT void JNICALL Java_com_jbloit_androidlinkaudio_AudioEngine_deleteEngine(
        JNIEnv *env,
        jclass,
        jlong engineHandle) {

    // We use std::nothrow so `new` returns a nullptr if the engine creation fails
    delete (AudioEngine *) engineHandle;
}

}