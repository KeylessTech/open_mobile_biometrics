package kl.open.fmandroid

object NativeBridge {

    @JvmStatic external fun jni_init(configJson: String, basePath: String): Boolean
    @JvmStatic external fun jni_process(imagePath: String, skipLiveness: Boolean): ProcessResult
    @JvmStatic external fun jni_match(embedding1: FloatArray, embedding2: FloatArray): Boolean
    @JvmStatic external fun jni_reset()

    /** TODO debug only: tell native code where to dump debug images */
    @JvmStatic external fun jni_setDebugSavePath(path: String)
}