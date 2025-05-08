package kl.open.fmandroid

/**
 * Represents the result of the face capture and match operation.
 *
 * @property processed Indicates whether the processing job completed, independently by the result.
 * @property referenceIsValid Indicates whether the reference image was valid and usable (e.g., face detected).
 * @property capturedIsLive True if the captured face passed liveness detection.
 * @property isSameSubject True if the captured face matches the reference face.
 * @property capturedPath Path to the captured image, or null if the capture failed.
 */
data class MatchResult(
    val processed: Boolean,
    val referenceIsValid: Boolean,
    val capturedIsLive: Boolean,
    val isSameSubject: Boolean,
    val capturedPath: String?
) {
    companion object {
        fun error(): MatchResult {
            return MatchResult(
                false,
                false,
                false,
                false,
                null
            )
        }
    }
}
