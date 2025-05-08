package kl.open.fmandroid

/**
 * Interface for the FaceMatch SDK.
 */
interface FaceMatchSDK {

    /**
     * Initializes the FaceMatch SDK with the provided configuration.
     *
     * You must call this method once before invoking any other SDK function.
     *
     * @param configJson A JSON string containing SDK configuration parameters.
     * @return true if the initialization was successful, false otherwise.
     */
    fun init(configJson: String): Boolean

    /**
     * Captures a photo using the front-facing camera, performs liveness detection,
     * and face matching against the provided reference image.
     *
     * The camera will remain active until a valid face is detected. This method handles
     * all camera logic internally and provides the result via the callback.
     *
     * You must call [init] before invoking this method.
     *
     * @param referenceImagePath Path to the reference image file.
     * @param onResult Callback that receives a [MatchResult] with the outcome.
     */
    fun captureAndMatch(referenceImagePath: String, onResult: (MatchResult) -> Unit)

    /**
     * Resets the internal state of the FaceMatch SDK.
     *
     * This method stops any ongoing camera session and clears any temporary data
     * or cached state held by the SDK. After calling [reset], you must re-initialize
     * the SDK using [init] before using it again.
     */
    fun reset()
}
