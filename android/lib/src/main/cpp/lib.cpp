#include <jni.h>
#include <string>
#include "FMCore.h"
#include <android/log.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <__thread/thread.h>

static FMCore engine;

extern "C" {

void redirectStdoutToLogcat() {
    static const char* tag = "NativeSTDOUT";

    // redirect stdout
    int pipefd[2];
    pipe(pipefd);
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);

    std::thread([=]() {
        char buffer[128];
        ssize_t count;
        while ((count = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[count] = '\0';
            __android_log_write(ANDROID_LOG_INFO, tag, buffer);
        }
    }).detach();
}


// TODO debug only
JNIEXPORT void JNICALL
Java_kl_open_fmandroid_NativeBridge_jni_1setDebugSavePath(
        JNIEnv* env,
        jclass /* clazz */,
        jstring jpath)
{
    const char* cpath = env->GetStringUTFChars(jpath, nullptr);
    setDebugSavePath(std::string(cpath));
    env->ReleaseStringUTFChars(jpath, cpath);
}

// public native boolean jni_init(String configJson, String basePath);
JNIEXPORT jboolean JNICALL
Java_kl_open_fmandroid_NativeBridge_jni_1init(JNIEnv* env, jobject /* this */, jstring configJson, jstring basePath) {

    redirectStdoutToLogcat();

    const char* configStr = env->GetStringUTFChars(configJson, nullptr);
    const char* basePathStr = env->GetStringUTFChars(basePath, nullptr);

    bool result = engine.init(std::string(configStr), std::string(basePathStr));

    env->ReleaseStringUTFChars(configJson, configStr);
    env->ReleaseStringUTFChars(basePath, basePathStr);

    return result;
}

// public native ProcessResult jni_process(String imagePath);
JNIEXPORT jobject JNICALL
Java_kl_open_fmandroid_NativeBridge_jni_1process(JNIEnv* env, jobject /* this */, jstring imagePath, jboolean skipLiveness) {
    const char* pathStr = env->GetStringUTFChars(imagePath, nullptr);

    ProcessResult result;
    if(skipLiveness) {
        result = engine.process(std::string(pathStr), PipelineMode::SkipLiveness);
    } else {
        result = engine.process(std::string(pathStr), PipelineMode::WholePipeline);
    }

    env->ReleaseStringUTFChars(imagePath, pathStr);

    // Find the ProcessResult class
    jclass resultClass = env->FindClass("kl/open/fmandroid/ProcessResult");
    if (resultClass == nullptr) {
        std::cerr << "[JNI] Failed to find ProcessResult class" << std::endl;
        return nullptr;
    }

    // Get constructor ID
    jmethodID constructor = env->GetMethodID(resultClass, "<init>",
                                             "(ZZZZ[F)V");
    if (constructor == nullptr) {
        std::cerr << "[JNI] Failed to find ProcessResult constructor" << std::endl;
        return nullptr;
    }

    // Create jfloatArray for embedding
    jfloatArray embeddingArray = env->NewFloatArray(static_cast<jsize>(result.embedding.size()));
    env->SetFloatArrayRegion(embeddingArray, 0, static_cast<jsize>(result.embedding.size()), result.embedding.data());

    // Construct the result object
    jobject resultObject = env->NewObject(resultClass, constructor,
                                          static_cast<jboolean>(result.livenessChecked),
                                          static_cast<jboolean>(result.isLive),
                                          static_cast<jboolean>(result.faceDetected),
                                          static_cast<jboolean>(result.embeddingExtracted),
                                          embeddingArray
    );

    return resultObject;
}

// public native bool match(float[] embedding1, float[] embedding2);
JNIEXPORT jboolean JNICALL
Java_kl_open_fmandroid_NativeBridge_jni_1match(JNIEnv* env, jobject /* this */,
                                              jfloatArray emb1, jfloatArray emb2) {
    jsize len1 = env->GetArrayLength(emb1);
    jsize len2 = env->GetArrayLength(emb2);

    std::vector<float> vec1(len1);
    std::vector<float> vec2(len2);

    env->GetFloatArrayRegion(emb1, 0, len1, vec1.data());
    env->GetFloatArrayRegion(emb2, 0, len2, vec2.data());

    return engine.match(vec1, vec2);
}

// public native void reset();
JNIEXPORT void JNICALL
Java_kl_open_fmandroid_NativeBridge_jni_1reset(JNIEnv* env, jobject /* this */) {
    engine.reset();
}

} // extern "C"
